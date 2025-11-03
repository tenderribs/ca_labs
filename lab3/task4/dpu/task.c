/*
 * AXPY with multiple tasklets
 *
 */
#include <alloc.h>
#include <assert.h>
#include <barrier.h>
#include <defs.h>
#include <mram.h>
#include <perfcounter.h>
#include <stdint.h>
#include <stdio.h>

#include "../support/common.h"
#include "../support/cyclecount.h"

#if (defined(OP_ADD) + defined(OP_SUB) + defined(OP_MULT) + defined(OP_DIV)) != 1
#error "Define exactly one vector operation (e.g. OP=ADD make)."
#endif

// Input and output arguments
__host dpu_arguments_t DPU_INPUT_ARGUMENTS;
__host dpu_results_t DPU_RESULTS[NR_TASKLETS];

// Barrier
BARRIER_INIT(my_barrier, NR_TASKLETS);

extern int main_kernel1(void);
int (*kernels[nr_kernels])(void) = {main_kernel1};
int main(void) {
    // Kernel
    return kernels[DPU_INPUT_ARGUMENTS.kernel]();
}

// AXPY: Computes AXPY for a cached block
static void axpy(T *bufferY, T *bufferX, unsigned int l_size) {
    //@@ INSERT AXPY CODE
    for (unsigned int i = 0; i < l_size; i++) {
#if defined(OP_ADD)
        bufferY[i] = bufferX[i] + bufferY[i];
#elif defined(OP_SUB)
        bufferY[i] = bufferX[i] - bufferY[i];
#elif defined(OP_MULT)
        bufferY[i] = bufferX[i] * bufferY[i];
#elif defined(OP_DIV)
    /* Protect integer division from divide-by-zero which causes SIGFPE on DPUs. */
#if defined(FLOAT) || defined(DOUBLE)
    bufferY[i] = bufferX[i] / bufferY[i];
#else
    bufferY[i] = (bufferY[i] != 0) ? (bufferX[i] / bufferY[i]) : (T)0;
#endif
#endif
    }
}

// main_kernel1
int main_kernel1() {
    unsigned int tasklet_id = me();
#if PRINT
    printf("tasklet_id = %u\n", tasklet_id);
#endif
    if (tasklet_id == 0) {
        mem_reset(); // Reset the heap
#ifdef CYCLES
        perfcounter_config(COUNT_CYCLES, true); // Initialize once the cycle counter
#elif INSTRUCTIONS
        perfcounter_config(COUNT_INSTRUCTIONS, true); // Initialize once the instruction counter
#endif
    }
    // Barrier
    barrier_wait(&my_barrier);
#if defined(CYCLES) || defined(INSTRUCTIONS)
    perfcounter_count count;
    dpu_results_t *result = &DPU_RESULTS[tasklet_id];
    result->count = 0;
    counter_start(&count); // START TIMER
#endif

    uint32_t input_size_dpu_bytes = DPU_INPUT_ARGUMENTS.size; // Input size per DPU in bytes
    uint32_t input_size_dpu_bytes_transfer =
        DPU_INPUT_ARGUMENTS.transfer_size; // Transfer input size per DPU in bytes

    // Address of the current processing block in MRAM
    uint32_t base_tasklet = tasklet_id << BLOCK_SIZE_LOG2;
    uint32_t mram_base_addr_X = (uint32_t)DPU_MRAM_HEAP_POINTER;
    uint32_t mram_base_addr_Y = (uint32_t)(DPU_MRAM_HEAP_POINTER + input_size_dpu_bytes_transfer);

    // Initialize a local cache in WRAM to store the MRAM block
    //@@ INSERT WRAM ALLOCATION HERE
    // base addresses where this tasklet will store each transferred block
    T *wram_base_addr_X = mem_alloc(BLOCK_SIZE);
    T *wram_base_addr_Y = mem_alloc(BLOCK_SIZE);
    assert(wram_base_addr_X != NULL && wram_base_addr_Y != NULL);

    for (unsigned int byte_index = base_tasklet; byte_index < input_size_dpu_bytes;
         byte_index += BLOCK_SIZE * NR_TASKLETS) {

        __mram_ptr T *mram_blk_start_X = (__mram_ptr T *)(mram_base_addr_X + byte_index);
        __mram_ptr T *mram_blk_start_Y = (__mram_ptr T *)(mram_base_addr_Y + byte_index);
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
        mram_read(mram_blk_start_Y, wram_base_addr_Y, act_tf_size);

        // Computer vector addition
        //@@ INSERT CALL TO AXPY FUNCTION HERE
        unsigned int num_elems = act_tf_size / sizeof(T);
        axpy(wram_base_addr_Y, wram_base_addr_X, num_elems);

        // Write cache to current MRAM block
        //@@ INSERT WRAM-MRAM TRANSFER HERE
        mram_write(wram_base_addr_Y, mram_blk_start_Y, act_tf_size);
    }

#if defined(CYCLES) || defined(INSTRUCTIONS)
    result->count += counter_stop(&count); // STOP TIMER
#endif

    return 0;
}
