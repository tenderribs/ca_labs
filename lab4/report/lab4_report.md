---
geometry:
  - margin=0.75in
  - letterpaper
papersize: a4
author: "Mark Marolf mmarolf@ethz.ch"
header-includes: |
  \usepackage{fancyhdr}
  \usepackage{lastpage}
  \pagestyle{fancy}
  \fancyhead[L]{Mark Marolf (mmarolf@ethz.ch)}
  \fancyhead[C]{}
  \fancyhead[R]{\thepage/\pageref{LastPage}}
  \fancyfoot[C]{}
---

<!-- pandoc lab4_report.md --to=pdf -o lab4_report.pdf --pdf-engine=xelatex --bibliography citations.bib -F pandoc-crossref --citeproc -->

# Lab 4: Hardware Data Prefetcher Design and Analysis

## Introduction

In this lab, I implement a GHB-based stride hardware data prefetcher. In a later step I improve the design by making it aware of system memory bandwidth constraints. Finally I create my own hybrid prefetcher, which chooses between a best-offset-prefetcher [@michaud] and the previous strided GHB prefetcher.

## Warm Up: Running the No-prefetching Baseline
![Baseline performance of the system without an L2C prefetcher on real-world CPU traces](./img/task0_nopref_ipc.eps){#fig:task0_nopref_ipc}

The hardware prefetcher designs are evaluated on CPU traces collected from fourteen workloads sourced from graph analytics benchmarks and data-center workloads. Each workload was run for 50 million cycles following 10 million cycles of initial warmup using the Champsim microarchitecural simulator [@champsim]. The simulated system has a single out-of-order core and a single channel of DRAM running at a data rate of 4800 MT/s ("full BW").

In [@fig:task0_nopref_ipc] is a plot of the instructions-per-cycle (IPC) for each workload.
The `charlie_1` trace resulted in the highest IPC value, while the `bfs-14` trace got the lowest IPC value.

## Task 1/4: Implementing GHB-based Stride Prefetcher

### Implementation

In Task 1 I implemented a GHB-based stride prefetcher [@ghb] for the L2C as closely as possible to the original specification. I took special care to accurately replicate the bit-width of tags in the Index Table (IT) and pointers in the Global History Buffer (GHB). However in my own testing, I found that the IPC results for the exact replication are identical to those with a more relaxed replication of the paper with wider bit-widths ([@tbl:ghb_pccs_it_width]). First, this shows that the hardware budget proposed in the paper provides performance equal to an expensive prefetcher, but with a fraction of the resources. Second, this means that my replication of the original prefetcher isn't sensitive to the specific sizings of the table entries.

\begingroup\footnotesize
| trace   | 1     | 2     | 3     | 4     | 5     | 6     | 7     | 8     | 9     | 10    | 11    | 12    | 13    | 14    |
| ------- | ----- | ----- | ----- | ----- | ----- | ----- | ----- | ----- | ----- | ----- | ----- | ----- | ----- | ----- |
| 20b IPC | 0.229 | 0.221 | 0.362 | 0.293 | 0.415 | 0.476 | 0.532 | 0.864 | 1.014 | 0.582 | 0.809 | 0.704 | 0.548 | 0.554 |
| 56b IPC | 0.229 | 0.221 | 0.362 | 0.293 | 0.415 | 0.476 | 0.532 | 0.864 | 1.014 | 0.582 | 0.809 | 0.704 | 0.548 | 0.554 |
: Per-trace IPC for GHB PC/CS with 20b and 56b wide IT tags (1 cpu, full BW) {#tbl:ghb_pccs_it_width}
\endgroup

![Task 1: IPC speedup, prefetch accuracy, and prefetches per thousand instructions (PPKI) of strided GHB prefetcher relative to system without a prefetcher (1 CPU, full BW)](./img/task1_ghb_pccs_fixed_fullbw.eps){#fig:t1_ghbpccs_fx_fullbw}

The per-workload IPC speedups, prefetching accuracies and prefetches per thousand instructions (PPKI) caused by the strided GHB prefetcher in the L2C are plotted in [@fig:t1_ghbpccs_fx_fullbw]. The IPC speedup is simply the ratio of the IPC measured with the prefetcher and the IPC without the prefetcher. Next, the accuracy is defined as the ratio of prefetches reported by Champsim logs as being "useful" (`l2c_prefetch_useful`) and the total number of issued prefetches (`l2c_prefetch_issued`).

### Results

The utility of the prefetcher is highly heterogenous for the different types of workloads. As explained in the following paragraphs, this indicates that the strided GHB prefetcher is only useful for workloads with access patterns that play to its strengths.

##### GAP Workloads
`bfs-14` was the workload with the largest IPC speedup. It was also the workload with the largest amount of prefetches issued, at the highest accuracy. The large number of prefetches issued indicate that the GAP workloads (graph analytics) can benefit a lot from a strided GHB prefetcher, considering they are most likely traversing neighboring nodes lists.

##### Charlie Workloads
`charlie_2` got the lowest IPC speedup of all workloads. With the exception of `charlie_3`, very few prefetches were issued for this group of workloads, leading to very low IPC speedups overall. For `charlie_3`, the number of issued prefetches was much higher, but with very poor accuracy. This indicates that the charlie workloads (sourced from Google data centers) feature cache accesses that don't suit a constant-stride prefetcher's profile. Possibly these workloads resemble pointer-chasing or random accesses.

## Task 2/4: Prefetching with Limited Main Memory Bandwidth

In this task, I investigate the effects of limited main memory bandwidth on the strided GHB prefetcher. The system with limited main memory bandwidth ("limited BW") is identical to the system with full BW except that the main memory bandwidth is 800 MT/s, a sixth of the full BW.

### Results

![Task 2: IPC speedup, prefetch accuracy, and prefetches per thousand instructions (PPKI) of strided GHB prefetcher relative to system without a prefetcher (1 CPU, limited BW)](./img/task2_ghb_pccs_fixed_limitbw.eps){#fig:t2_ghbpccs_fx_limbw}



<!-- ## Task 2 AXPY

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
 -->


\newpage
## Citations