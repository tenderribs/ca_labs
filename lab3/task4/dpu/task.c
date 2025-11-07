/*
 * AXPY with multiple tasklets
 *
 */
#include "mutex.h"
#include <alloc.h>
#include <assert.h>
#include <barrier.h>
#include <defs.h>
#include <handshake.h>
#include <mram.h>
#include <perfcounter.h>
#include <stdint.h>
#include <stdio.h>

#include "../support/common.h"
#include "../support/cyclecount.h"

#if (defined(FINAL_SINGLE) + defined(FINAL_TREE_BARRIER) + defined(FINAL_TREE_HANDSHAKE) +         \
     defined(FINAL_MUTEX)) != 1
#error                                                                                             \
    "Define exactly one final reduction strategy (FINAL=SINGLE|TREE_BARRIER|TREE_HANDSHAKE|MUTEX)."
#endif

// Input and output arguments
__host dpu_arguments_t DPU_INPUT_ARGUMENTS;
__host dpu_results_t DPU_RESULTS[NR_TASKLETS];

// array where each tasklet places its partially reduced value
static T tasklet_partials[NR_TASKLETS];

#if defined(FINAL_MUTEX)
MUTEX_INIT(my_mutex);
static T mutex_accum; // store the intermediate accumulated value
static T accum_initialized;
#endif

// Barrier
BARRIER_INIT(my_barrier, NR_TASKLETS);

extern int main_kernel1(void);
int (*kernels[nr_kernels])(void) = {main_kernel1};
int main(void) {
    // Kernel
    return kernels[DPU_INPUT_ARGUMENTS.kernel]();
}

// vec_red: Reduces a vector to its resimum element
// force_init: whether initialize the result as the first element in the vector
static void vec_red(T *vec, T *res, unsigned int l_size, uint8_t force_init) {
    if (force_init) {
        *res = vec[0];
    }

    for (unsigned int i = 0; i < l_size; i++) {
        if (vec[i] > *res)
            *res = vec[i];
    }
}

static inline T max_val(T a, T b) { return (a > b) ? a : b; }

#if defined(FINAL_SINGLE)
static T reduce_single_tasklet(unsigned int tasklet_id) {
    if (tasklet_id == 0) {
        T acc = tasklet_partials[0];
        for (unsigned int other = 1; other < NR_TASKLETS; ++other) {
            acc = max_val(acc, tasklet_partials[other]);
        }
        tasklet_partials[0] = acc;
        return acc;
    }

    return (T)0; // we never use this return value further
}
#elif defined(FINAL_TREE_BARRIER)
static T reduce_tree_barrier(unsigned int tasklet_id) {
    T local = tasklet_partials[tasklet_id];

    // wait for all tasklets to put their results in tasklet_partials[tasklet_id]
    barrier_wait(&my_barrier);

    // walk through levels of tasklet_partials at increasing strides, coalescing
    // subresults at given level into single value, till only one value remains
    for (unsigned int stride = 1; stride < NR_TASKLETS; stride <<= 1) {
        unsigned int group = stride << 1;
        if ((tasklet_id % group) == 0) {
            unsigned int partner = tasklet_id + stride;
            if (partner < NR_TASKLETS) {
                // merge local node with partner at same level
                T other = tasklet_partials[partner];
                local = max_val(local, other);
                tasklet_partials[tasklet_id] = local;
            }
        }
    }

    return tasklet_partials[0];
}
#elif defined(FINAL_TREE_HANDSHAKE)
static T reduce_tree_handshake(unsigned int tasklet_id) {
    T local = tasklet_partials[tasklet_id];

    // tasklet_partials represents a flattened binary heap
    for (unsigned int stride = 1; stride < NR_TASKLETS; stride <<= 1) {
        unsigned int group = stride << 1;
        if ((tasklet_id % group) == 0) {
            unsigned int partner = tasklet_id + stride;
            if (partner < NR_TASKLETS) {
                handshake_wait_for(partner);

                // read and merge partner
                T other = tasklet_partials[partner];
                local = max_val(local, other);
                tasklet_partials[tasklet_id] = local;
            }
        } else {
            tasklet_partials[tasklet_id] = local;
            handshake_notify();
            return local;
        }
    }

    return local;
}
#elif defined(FINAL_MUTEX)
static T reduce_mutex(unsigned int tasklet_id) {
    T local = tasklet_partials[tasklet_id];

    mutex_lock(my_mutex);
    if (!accum_initialized) {
        mutex_accum = local;
        accum_initialized = 1;
    } else {
        mutex_accum = max_val(mutex_accum, local);
    }
    mutex_unlock(my_mutex);

    // wait for all tasklets to have acquired the mutex
    barrier_wait(&my_barrier);
    return mutex_accum;
}
#endif

static T finalize_reduction(unsigned int tasklet_id) {
#if defined(FINAL_SINGLE)
    barrier_wait(&my_barrier);
    return reduce_single_tasklet(tasklet_id);
#elif defined(FINAL_TREE_BARRIER)
    return reduce_tree_barrier(tasklet_id);
#elif defined(FINAL_TREE_HANDSHAKE)
    return reduce_tree_handshake(tasklet_id);
#elif defined(FINAL_MUTEX)
    return reduce_mutex(tasklet_id);
#else
    return (T)0;
#endif
}

// main_kernel1
int main_kernel1() {
    unsigned int tasklet_id = me();
#if PRINT
    printf("tasklet_id = %u\n", tasklet_id);
#endif
    if (tasklet_id == 0) {
        mem_reset();                                  // Reset the heap
        perfcounter_config(COUNT_CYCLES, true);       // Initialize once the cycle counter
        perfcounter_config(COUNT_INSTRUCTIONS, true); // Initialize once the instruction counter

#if defined(FINAL_MUTEX)
        mutex_accum = 0;
        accum_initialized = 0;
#endif
    }
    // Barrier
    barrier_wait(&my_barrier);

    perfcounter_count count;
    dpu_results_t *result = &DPU_RESULTS[tasklet_id];
    result->count = 0;
    counter_start(&count); // START TIMER

    uint32_t input_size_dpu_bytes = DPU_INPUT_ARGUMENTS.size; // Input size per DPU in bytes
    // uint32_t input_size_dpu_bytes_transfer =
    //     DPU_INPUT_ARGUMENTS.transfer_size; // Transfer input size per DPU in bytes

    // Address of the current processing block in MRAM
    uint32_t base_tasklet = tasklet_id << BLOCK_SIZE_LOG2;
    uint32_t mram_base_addr_X = (uint32_t)DPU_MRAM_HEAP_POINTER;

    // Initialize a local cache in WRAM to store the MRAM block
    //@@ INSERT WRAM ALLOCATION HERE
    // base addresses where this tasklet will store each transferred block
    T *wram_base_addr_X = mem_alloc(BLOCK_SIZE);
    assert(wram_base_addr_X != NULL);

    // result calculated by each tasklet onboard a DPU

    uint8_t tasklet_res_uninitialized = 1;

    for (unsigned int byte_index = base_tasklet; byte_index < input_size_dpu_bytes;
         byte_index += BLOCK_SIZE * NR_TASKLETS) {
        __mram_ptr T *mram_blk_start_X = (__mram_ptr T *)(mram_base_addr_X + byte_index);
        uint32_t act_tf_size = BLOCK_SIZE;

        // Bound checking
        //@@ INSERT BOUND CHECKING HERE
        // last block might be smaller than BLOCK_SIZE if input_size_dpu_bytes isn't BLOCK_SIZE
        // aligned
        if (byte_index + BLOCK_SIZE > input_size_dpu_bytes) { // if this block extends past the end
            act_tf_size =
                input_size_dpu_bytes - byte_index; // only transfer remaining bytes for this tasklet
        }

        // check TF constraints
        // https://sdk.upmem.com/2024.1.0/031_DPURuntimeService_Memory.html#direct-access-to-the-mram
        assert(act_tf_size % 8 == 0);
        assert(act_tf_size >= 8);
        assert(act_tf_size <= 2048);

        // Load cache with current MRAM block
        //@@ INSERT MRAM-WRAM TRANSFERS HERE
        mram_read(mram_blk_start_X, wram_base_addr_X, act_tf_size);

        // Computer vector reduction
        unsigned int num_elems = act_tf_size / sizeof(T);

        vec_red(wram_base_addr_X, &tasklet_partials[tasklet_id], num_elems,
                tasklet_res_uninitialized);
        tasklet_res_uninitialized = 0;
    }

    // compute final reduction
    T final_value = finalize_reduction(tasklet_id);

    if (tasklet_id == 0) {
        // Transfer 8 bytes (instead of sizeof(T)) because transfer has to be 8 bytes at a minimum.
        // T is at most 8 bytes (64 bits) for doubles or int64_t.
        mram_write(&final_value, (__mram_ptr T *)mram_base_addr_X, 8);
    }

    result->count += counter_stop(&count); // STOP TIMER

    return 0;
}
