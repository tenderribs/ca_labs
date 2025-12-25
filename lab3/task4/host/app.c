/**
 * app.c
 * Host Application Source File
 *
 */
#include <assert.h>
#include <dpu.h>
#include <dpu_log.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../support/common.h"
#include "../support/params.h"
#include "../support/timer.h"

// Define the DPU Binary path as DPU_BINARY here
#ifndef DPU_BINARY
#define DPU_BINARY "./bin/dpu_code"
#endif

// Pointer declaration
static T *X;
static T host_reduc_res = 0;
static T dpu_reduc_res = 0;

// Create input arrays
static void read_input(T *A, unsigned int nr_elements) {
    srand(0);
    printf("nr_elements\t%u\n", nr_elements);
    for (unsigned int i = 0; i < nr_elements; i++) {
        A[i] = (T)(rand());
    }
}

// Compute output in the host for verification purposes
// vec_red: Reduces a vector to its maximum element
static void vec_red(T *vec, T *res, unsigned int nr_elements) {
    for (unsigned int i = 0; i < nr_elements; i++) {
        *res += vec[i];
    }
}

// Main of the Host Application
int main(int argc, char **argv) {
    // Input parameters
    struct Params p = input_params(argc, argv);

    // Timer declaration
    Timer timer;
    double cc = 0;
    double cc_min = 0;

    // Allocate DPUs
    struct dpu_set_t dpu_set, dpu;
    uint32_t nr_of_dpus;
    DPU_ASSERT(dpu_alloc(NR_DPUS, NULL, &dpu_set));
    DPU_ASSERT(dpu_get_nr_dpus(dpu_set, &nr_of_dpus)); // Number of DPUs in the DPU set
    printf("Allocated %d DPU(s)\t", nr_of_dpus);
    printf("NR_TASKLETS\t%d\tBLOCK\t%d\n", NR_TASKLETS, BLOCK);

    // Load binary
    DPU_ASSERT(dpu_load(dpu_set, DPU_BINARY, NULL));

    // Input size
    const unsigned int input_size = p.input_size; // Total input size
    const unsigned int input_size_8bytes = ((input_size * sizeof(T)) % 8) != 0
                                               ? roundup(input_size, 8)
                                               : input_size; // Total input size, 8-byte aligned
    const unsigned int input_size_dpu =
        divceil(input_size, nr_of_dpus);       // Input size per DPU (max.)
    const unsigned int input_size_dpu_8bytes = // number of elements per DPU
        ((input_size_dpu * sizeof(T)) % 8) != 0
            ? roundup(input_size_dpu, 8)
            : input_size_dpu; // Input size per DPU (max.), 8-byte aligned

    // Input/output allocation in host main memory
    X = malloc(input_size_dpu_8bytes * nr_of_dpus * sizeof(T));
    uint64_t dpu_partials[nr_of_dpus];

    T *bufferX = X;

    unsigned int i = 0;

    // Create an input file with arbitrary data
    read_input(X, input_size);

    // Loop over main kernel
    for (int rep = 0; rep < p.n_warmup + p.n_reps; rep++) {
        // Reset accumulators for each repetition
        host_reduc_res = 0;
        dpu_reduc_res = 0;

        // Compute output on CPU (verification purposes)
        if (rep >= p.n_warmup)
            start(&timer, 0, rep - p.n_warmup);

        vec_red(X, &host_reduc_res, input_size);
        printf("Computed vec_red on host\r\n");

        if (rep >= p.n_warmup)
            stop(&timer, 0);

        printf("Load input data\n");
        // Input arguments
        unsigned int kernel = 0;
        dpu_arguments_t input_arguments[NR_DPUS];
        for (i = 0; i < nr_of_dpus - 1; i++) {
            input_arguments[i].size = input_size_dpu_8bytes * sizeof(T);
            input_arguments[i].transfer_size = input_size_dpu_8bytes * sizeof(T);
            input_arguments[i].kernel = kernel;
        }
        input_arguments[nr_of_dpus - 1].size =
            (input_size_8bytes - input_size_dpu_8bytes * (NR_DPUS - 1)) * sizeof(T);
        input_arguments[nr_of_dpus - 1].transfer_size = input_size_dpu_8bytes * sizeof(T);
        input_arguments[nr_of_dpus - 1].kernel = kernel;

        if (rep >= p.n_warmup)
            start(&timer, 1, rep - p.n_warmup); // Start timer (CPU-DPU transfers)
        i = 0;
        // Copy input arguments
        // Parallel transfers
        DPU_FOREACH(dpu_set, dpu, i) { DPU_ASSERT(dpu_prepare_xfer(dpu, &input_arguments[i])); }
        DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_TO_DPU, "DPU_INPUT_ARGUMENTS", 0,
                                 sizeof(input_arguments[0]), DPU_XFER_DEFAULT));

        // Copy input arrays

        const uint32_t x_offset = 0;

#ifdef SERIAL // Serial transfers
        //@@ INSERT SERIAL CPU-DPU TRANSFER HERE

        i = 0; // Transfer X vector chunks to DPUs @ MRAM heap end
        DPU_FOREACH(dpu_set, dpu, i) {
            DPU_ASSERT(dpu_prepare_xfer(dpu, &X[i * input_size_dpu_8bytes]));
        }
        DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_TO_DPU, DPU_MRAM_HEAP_POINTER_NAME, x_offset,
                                 input_size_dpu_8bytes * sizeof(T), DPU_XFER_DEFAULT));

#else // Parallel transfers
      //@@ INSERT PARALLEL CPU-DPU TRANSFER HERE
        i = 0; // Transfer X vector chunks to DPUs @ MRAM heap end
        DPU_FOREACH(dpu_set, dpu, i) {
            DPU_ASSERT(dpu_prepare_xfer(dpu, &X[i * input_size_dpu_8bytes]));
        }
        DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_TO_DPU, DPU_MRAM_HEAP_POINTER_NAME, x_offset,
                                 input_size_dpu_8bytes * sizeof(T), DPU_XFER_ASYNC));

        // wait for async transfers to complete
        DPU_ASSERT(dpu_sync(dpu_set));
#endif
        printf("Completed transfer from CPU to DPU\r\n");
        if (rep >= p.n_warmup)
            stop(&timer, 1); // Stop timer (CPU-DPU transfers)

        printf("Run program on DPU(s) \n");
        // Run DPU kernel
        if (rep >= p.n_warmup) {
            start(&timer, 2, rep - p.n_warmup); // Start timer (DPU kernel)
        }
        DPU_ASSERT(dpu_launch(dpu_set, DPU_SYNCHRONOUS));
        if (rep >= p.n_warmup) {
            stop(&timer, 2); // Stop timer (DPU kernel)
        }

#if PRINT
        {
            unsigned int each_dpu = 0;
            printf("Display DPU Logs\n");
            DPU_FOREACH(dpu_set, dpu) {
                printf("DPU#%d:\n", each_dpu);
                DPU_ASSERT(dpulog_read_for_dpu(dpu.dpu, stdout));
                each_dpu++;
            }
        }
#endif
        printf("Retrieve results\n");
        if (rep >= p.n_warmup)
            start(&timer, 3, rep - p.n_warmup); // Start timer (DPU-CPU transfers)

        // Transfer final reduced value onto CPU
        i = 0;
        DPU_FOREACH(dpu_set, dpu, i) { DPU_ASSERT(dpu_prepare_xfer(dpu, &dpu_partials[i])); }
        printf("Prepared targets\n");

        // Synchronously copy the data back
        // size and offset of mram read have to be at least 8 bytes. So copy the data over as an
        // uint64_t and trim it to size T
        DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_FROM_DPU, DPU_MRAM_HEAP_POINTER_NAME, x_offset,
                                 8, DPU_XFER_DEFAULT));
        printf("copying data back\n");

        // reduce the results from the DPUS to a final value
        assert(sizeof(T) <= 8);
        for (size_t i = 0; i < nr_of_dpus; i++) {
            // trim the uint64_t to size of T (sizeof(T) <= 8)
            T dpu_partial = 0;
            memcpy(&dpu_partial, &dpu_partials[i], sizeof(T));
            printf("DPU#%ld partial = 0x%08x\r\n", i, dpu_partial);

            dpu_reduc_res += dpu_partial;
            printf("updated dpu_reduc_res = 0x%08x\r\n", dpu_reduc_res);
        }

        if (rep >= p.n_warmup)
            stop(&timer, 3); // Stop timer (DPU-CPU transfers)

        dpu_results_t results[nr_of_dpus];

        // Parallel transfers
        dpu_results_t *results_retrieve[nr_of_dpus];
        DPU_FOREACH(dpu_set, dpu, i) {
            results_retrieve[i] = (dpu_results_t *)malloc(NR_TASKLETS * sizeof(dpu_results_t));
            DPU_ASSERT(dpu_prepare_xfer(dpu, results_retrieve[i]));
        }
        DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_FROM_DPU, "DPU_RESULTS", 0,
                                 NR_TASKLETS * sizeof(dpu_results_t), DPU_XFER_DEFAULT));
        DPU_FOREACH(dpu_set, dpu, i) {
            results[i].count = 0;
            // Retrieve tasklet count
            for (unsigned int each_tasklet = 0; each_tasklet < NR_TASKLETS; each_tasklet++) {
                if (results_retrieve[i][each_tasklet].count > results[i].count) {
                    results[i].count = results_retrieve[i][each_tasklet].count;
                }
            }
            free(results_retrieve[i]);
        }

        uint64_t max_count = 0;
        uint64_t min_count = 0xFFFFFFFFFFFFFFFF;
        // Print performance results
        if (rep >= p.n_warmup) {
            i = 0;
            DPU_FOREACH(dpu_set, dpu) {
                if (results[i].count > max_count)
                    max_count = results[i].count;
                if (results[i].count < min_count)
                    min_count = results[i].count;
                i++;
            }
            cc += (double)max_count;
            cc_min += (double)min_count;
        }
    }
    printf("DPU cycles  = %g\n", cc / p.n_reps);
    printf("DPU instructions  = %g\n", cc / p.n_reps);

    // Print timing results
    printf("CPU ");
    print(&timer, 0, p.n_reps);
    printf("CPU-DPU ");
    print(&timer, 1, p.n_reps);
    printf("DPU Kernel ");
    print(&timer, 2, p.n_reps);
    printf("DPU-CPU ");
    print(&timer, 3, p.n_reps);
    // Check output
    bool status = (dpu_reduc_res == host_reduc_res);
    if (status) {
        printf("[" ANSI_COLOR_GREEN "OK" ANSI_COLOR_RESET "] Outputs are equal\n");
    } else {
        printf("[" ANSI_COLOR_RED "ERROR" ANSI_COLOR_RESET "] Outputs differ!\n");
    }

    printf("DPU: 0x%08x CPU 0x%08x\r\n", dpu_reduc_res, host_reduc_res);

    // Deallocation
    free(X);
    DPU_ASSERT(dpu_free(dpu_set)); // Deallocate DPUs

    return status ? 0 : -1;
}
