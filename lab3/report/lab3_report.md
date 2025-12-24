---
geometry:
  - margin=0.75in
  - letterpaper
papersize: a4
author: "Mark Marolf mmarolf@ethz.ch"
date: "10/10/2025"
header-includes: |
  \usepackage{fancyhdr}
  \usepackage{lastpage}
  \pagestyle{fancy}
  \fancyhead[L]{Mark Marolf (mmarolf@ethz.ch)}
  \fancyhead[C]{}
  \fancyhead[R]{\thepage/\pageref{LastPage}}
  \fancyfoot[C]{}
---

<!-- pandoc lab3_report.md --to=pdf -o lab3_report.pdf --pdf-engine=xelatex --bibliography citations.bib -F pandoc-crossref --citeproc -->

# Lab 3: Programming a Real Processing-in-Memory Architecture

## Introduction

In this lab, I performed an analysis of a real Processing-in-Memory (PIM) architecture. This involved implementing and evaluating four distinct tasks for the UPMEM PIM System: characterizing data transfer performance, scaling the AXPY vector operation, assessing performance across different data types and operations, and exploring intra-DPU synchronization primitives for parallel vector reduction. Each task was simulated using the simulator provided by the UPMEM SDK.

## Task 1 Transferring Data between Main Memory and PIM-enabled Memory

This task was concerned with the data transfer bandwidth between the host CPU and the DPUs. Aside from computation inputs and outputs being transferred between the host CPU and the DPUs, communication between DPUs has also to be routed through the host as an intermediary. Therefore having high transfer bandwidth is important for the PIM system to be effective. Documentation for runtime communication primitives is available in the [UPMEM manual](https://sdk.upmem.com/2024.1.0/033_AdvancedHostFeatures.html).

### Implementation Details

There are three different data transfer methods: `SERIAL`, `PARALLEL` and `BROADCAST`. Data transfers have to be invoked on the host, regardless of the transfer direction (CPU to DPU or DPU to CPU). In contrast to the former two methods, `BROADCAST` can only distribute data from the host to the DPUs. The SERIAL and PARALLEL methods rely on the `dpu_prepare_xfer` function. The `BROADCAST` method relies on the `dpu_broadcast_to` function. In my implementation, the data is written to and read from each DPU's `DPU_MRAM_HEAP_POINTER`. The MRAM capacity of each DPU limits the theoretical maximum transfer size per DPU to 64MB. Each transfer size has to be aligned to 8 bytes.

##### `SERIAL`
`SERIAL` passes the `DPU_XFER_DEFAULT` option, which indicates that the transfer should happen synchronously. Although the transfer between the host and the dpus occurs in parallel, each call to `dpu_push_xfer` is blocking. Later calls to `dpu_push_xfer` can only start once the first call has completed. Note there can an alternative way of implementing serial communication, depending on how the task description is understood. This involves iterating over the dpus individually and transferring the data one-by-one. Both interpretations are reasonable, so I implemented the first one based on the fact that it was less effort to program and intuitively performed better.

##### `PARALLEL`
`PARALLEL` passes the `DPU_XFER_ASYNC` option, requesting an asynchronous transfer. This also starts a parallel transfer, but it allows for subsequent transfers to start before every single parallel transfer of the first call has completed. To ensure data integrity in later computation, `dpu_sync` has to be called after a parallel transfer to wait for every asynchronous operation to have completed.

##### `BROADCAST`
`BROADCAST` calls `dpu_broadcast_to`, which copies the same buffer to all DPUS in the set.

```c
#ifdef SERIAL   // Serial transfers
    DPU_ASSERT(dpu_push_xfer(
        dpu_set,                            // the identifier of the DPU set
        DPU_XFER_TO_DPU,                    // direction of the transfer
        DPU_MRAM_HEAP_POINTER_NAME,         // location of symbol where to place data
        0,                                  // byte offset from symbol address
        input_size_dpu_8bytes * sizeof(T),  // number of bytes to copy
        DPU_XFER_DEFAULT                    // options of the transfer
    ));

#elif BROADCAST // Broadcast transfers
    DPU_ASSERT(dpu_broadcast_to(
        dpu_set,
        DPU_MRAM_HEAP_POINTER_NAME,
        0,
        bufferX,
        input_size_dpu_8bytes * sizeof(T),
        DPU_XFER_DEFAULT
    ));

#else // Parallel transfers
    DPU_ASSERT(dpu_push_xfer(
        dpu_set,
        DPU_XFER_TO_DPU,
        DPU_MRAM_HEAP_POINTER_NAME,
        0,
        input_size_dpu_8bytes * sizeof(T),
        DPU_XFER_ASYNC
    ));

    // wait for every asynchronous operation to have been performed
    DPU_ASSERT(dpu_sync(dpu_set));
#endif
```

### Evaluation

To evaluate the effectiveness of each transfer method, I transferred data between the host and between 1 and 64 DPUs at three different transfer sizes. The final result per DPU count was an average gained from ten separate runs. The transfer bandwidth is calculated as the total size of the transfer, divided by transfer duration.

#### Distributing data from the CPU to DPUs

As can be seen in figure [@fig:tf_bw], the broadcast method consistently provides the highest bandwidth in the CPU to DPU transfers, while the serial and parallel methods perform similarly overall. The performance benefit between broadcasting and the other two methods is more pronounced for larger transfer sizes. At transfer sizes of 24MB and 48MB, the measured bandwidth increases sharply till around 8 DPUs and then drops off linearly thereafter at a much slower rate.

#### Collecting data from the DPUs

In this scenario, the highest bandwidth is measured at DPU counts less than 5. Thereafter, the bandwidth remains at an almost constant level for the remaining DPU counts. There is very little difference between the serial and parallel transfer methods.

![Overall bandwidth vs. number of DPUs for three transfer sizes. Top: CPU→DPU (Serial, Parallel, Broadcast). Bottom: DPU→CPU (Serial, Parallel).](./plots/t1_transfer_bandwidth.eps){#fig:tf_bw}

### Analysis and Observations

- CPU→DPU is consistently faster than DPU→CPU.
- `SERIAL` and `PARALLEL` look similar here because the asynchrony only helps when you queue multiple outstanding transfers before waiting. In our implementation we issue one transfer and then immediately call `dpu_sync`, which collapses the overlap and makes it behave close to the synchronous version. To expose a benefit, we would need to pipeline several transfers (e.g., multiple chunks or directions) before the sync.
- `BROADCAST` delivers the highest CPU→DPU bandwidth, indicating that fixed overheads are better amortized compared to the other transfer methods.


\newpage







## Task 2 AXPY

This task is concerned with quantifying the performance scaling of PIM threads (tasklets). To this end, the AXPY operation ($y= y + alpha ×x$) is computed in a distributed fashion on the DPUs.

### Implementation Details

After placing both input vectors $x$ and $y$ in the DPUs MRAM heap, the host triggers the distributed computation. Each tasklet allocates a section of memory of fixed size in the DPUs WRAM using `mem_alloc`. Then disjoint blocks of the vectors are copied from the MRAM into the previously allocated tasklet WRAM space using `mram_read`. Special care must be taken to ensure this transfer's size is between 8 and 2048 bytes and aligned by 8 bytes. The AXPY operation is performed on the data block in WRAM and subsequently written back to MRAM using `mram_write`. To conclude, the host copies and aggregates the partial results from each DPU.

Due to hardware limitations, the maximum number of tasklets per DPU is 24. The allocated tasklet WRAM block's size has to be chosen such that each tasklet's data can fit inside the WRAM, which becomes an issue at larger numbers of tasklets. It is beneficial to maximize the block size, since this reduces the copying transfer overheads between the MRAM and WRAM. I found the highest possible block size to be 512B, using at most `2 * 24 * 512B = 24'576B` of the `64KB` of WRAM,  which left enough memory for the remaining variables. This configuration produces correct results for any number of tasklets and DPUs.

### Evaluation

In [@fig:inst_vs_tlet_cnt], the number of executed instructions per tasklet is plotted against the number of tasklets. For this experiment, two 16MB `uint32_t` input vectors were distributed among 32 DPUs and 64 DPUs. Scaling the number of tasklets reduces the instructions executed on each tasklet, but the relationship is non-linear and exhibits diminishing returns.

![The number of executed instructions per tasklet decreases non-linearly with tasklet count. Using more DPUs (64 vs 32) reduces instructions per tasklet proportionally, as the total workload is distributed across more processing elements.](./plots/t2_inst_count_per_tasklet_vs_tasklet_count.eps){#fig:inst_vs_tlet_cnt}


### Analysis and Observations

As suggested by the line plot in [@fig:inst_vs_tlet_cnt], the instruction count per tasklet doesn't decrease linearly in the number of tasklets. In fact the kernel execution time on the DPU actually increases with the number of tasklets involved in computation. This suggests that copying data between MRAM and WRAM incurs large costs, that negate the benefits of dividing the workload among tasklets.

Therefore for the AXPY kernel with 512-byte blocks, adding tasklets beyond a small count yields diminishing or negative returns due to fixed overheads and memory contention. A performance speedup can instead be achieved by leveraging parallelism in the number of DPUs allocated for computation. The instruction count per tasklet for 64 DPUs consistently is half that of the one with 32 DPUs.




\newpage





## Task 3 Operations and Data Types

The goal of this task is to quantify the computational cost of arithmetic operations on the DPUs across different data types.

### Implementation Details

Task 3 builds upon the AXPY kernel from Task 2, but generalizes the computation to support four different arithmetic operations (addition, subtraction, multiplication, and division) across six data types. The operation and data type are selected at compile time via preprocessor macros.

The implementation architecture remains similar to Task 2: input vectors $x$ and $y$ are distributed across DPUs' MRAM heaps with parallel transfers from the host. Each DPU spawns tasklets that collaboratively process their assigned vector chunks.

The core `axpy` function is modified to perform element-wise operations based on the selected macro. Special handling is required for division: for integer types, divide-by-zero is explicitly guarded to prevent `SIGFPE` signals on the DPUs by checking if the denominator is non-zero before performing the division. For floating-point types, standard IEEE 754 semantics are preserved.

```c
// Computes arithmetic operation for a cached block
static void arithm_op(T *bufferY, T *bufferX, unsigned int l_size) {
    for (unsigned int i = 0; i < l_size; i++) {
#if defined(OP_ADD)
        bufferY[i] = bufferX[i] + bufferY[i];
#elif defined(OP_SUB)
        bufferY[i] = bufferX[i] - bufferY[i];
#elif defined(OP_MULT)
        bufferY[i] = bufferX[i] * bufferY[i];
#elif defined(OP_DIV)
    /* Protect integer division from divide-by-zero which causes exception */
#if defined(FLOAT) || defined(DOUBLE)
    bufferY[i] = bufferX[i] / bufferY[i];
#else
    bufferY[i] = (bufferY[i] != 0) ? (bufferX[i] / bufferY[i]) : (T)0;
#endif
#endif
    }
}
```

### Methodology for Result Generation

To ensure reproducibility and automate the evaluation across the wide range of configurations (operations, data types, and tasklet counts), I utilized a Python script (`profile.py`) to manage the build and execution process. This script iterates through the defined parameter space and, for each unique configuration, sets the corresponding environment variables (e.g., `OP=ADD, TYPE=INT32, NR_TASKLETS=16`). It then triggers a rebuild of the project by invoking make. The slightly modified Makefile captures these environment variables and passes them as preprocessor macros (e.g., `-DOP_ADD`) to the compiler. Once compiled, the script executes the host binary, parses the output for performance metrics (such as instruction counts and execution time), and logs the results to a CSV file, which is subsequently used to generate the plots.


### Evaluation

To measure the arithmetic throughput of each combination of data type and arithmetic operation, I measured the number of DPU kernel instructions required to compute 500'000 respective arithmetic operations. The results are plotted in [@fig:inst_vs_arith_op_dtype].

![Instructions per arithmetic operation with different operations and datatypes. WRAM block size = 1024B, 16 tasklets, 8 DPUs, 500k operations. Demonstrates the high costs of software emulation.](./plots/t3_instr_per_element_by_op_n_dtype.eps){#fig:inst_vs_arith_op_dtype}

The results clearly indicate that integer addition and subtraction can be performed with very few instructions, while integer division and multiplication is considerably more expensive. Floating point operations are very expensive, especially for division and multiplication.

### Analysis and Observations

DPUs provide native hardware support for 32- and 64-bit integer addition and subtraction, leading to high throughput for these operations. DPUs don't natively support 32- and 64-bit multiplication and division and floating point operations. These operations need to be emulated by the UPMEM runtime, leading to much lower throughput. This is a clear indicator that developers must avoid costly floating point operations, and use integer division and multiplication sparingly.





\newpage



## Task 4 Vector Reduction

This task explores intra-DPU synchronization primitives by implementing parallel vector reduction across multiple tasklets. The goal is to compare four synchronization strategies and identify which approach best leverages the UPMEM architecture's threading model across different workload sizes.

### Implementation Details

For simplicity, my reduction method computes the maximum component of a vector. This means that the vector reduction kernel builds on the MRAM and WRAM transfer patterns established in previous tasks but introduces a new final step: after each tasklet independently finds the maximum of its assigned vector partition, the partial results are merged into an overall maximum value for the DPU. This final reduction step requires synchronization to prevent race conditions and ensure correctness. The final maximum value is computed on the host CPU as the maximum of all partial DPU results.

Transfers between the WRAM and MRAM  (`mram_write()`) and the DPU and host (`dpu_push_xfer`) have a minimum transfer size of 8 bytes. For smaller types (`CHAR`, `SHORT`, `INT32`) a larger chunk of 8 bytes is copied and the host extracts the valid bytes using `memcpy()`.

### Evaluation

The four synchronization strategies were evaluated across three vector sizes (16 KB, 1 MB, 16 MB) distributed among 64 DPUs, with tasklet counts ranging from 2 to 24. Figure [@fig:inst_cnt_per_meth] shows the total instruction count per DPU as a function of tasklet count for each method at the three vector sizes.

`SINGLE`, `TREE_HANDSHAKE`, and `MUTEX` show nearly identical instruction counts at most configurations. This convergence is surprising given their fundamentally different synchronization models. Across all vector sizes, though especially the smallest one, the `TREE_BARRIER` reduction incurs higher instruction counts, with the gap widening as tasklet count increases.

![Total instruction count per DPU for four synchronization methods across three vector sizes (16 KB, 1 MB, 16 MB). Particularly at larger tasklet counts the tree barrier method has outsized synchronization costs.](./plots/t4_inst_cnt_per_method_vs_nr_tasklets.eps){#fig:inst_cnt_per_meth}

### Analysis and Observations

At smaller vector sizes, synchronization overhead becomes proportionally more visible. The tree-based barrier method suffers from global synchronization overhead at each tree level. For example, at level 3 (stride=8), only 3 tasklets actively merge values, but all tasklets must synchronize. Handshakes on the other hand eliminate global synchronization by replacing barriers with point-to-point signaling. The single-tasklet method avoids synchronization complexity entirely: it does one `barrier_wait()` at the start, then tasklet 0 sequentially scans at most 24 partial results.

<!-- ```c
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

        // wait for the partial results to accumulate before combining
        barrier_wait(&my_barrier); // this line incurs large costs.
    }

    return tasklet_partials[0];
}
``` -->