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

<!-- pandoc lab3_report.md --to=pdf -o report.pdf --pdf-engine=xelatex --bibliography citations.bib -F pandoc-crossref --citeproc -->

# Lab 3: Programming a Real Processing-in-Memory Architecture

## Introduction

In this lab, I performed an analysis of a real Processing-in-Memory (PIM) architecture. This involved implementing and evaluating four distinct tasks for the UPMEM PIM System: characterizing data transfer performance, scaling the AXPY vector operation, assessing performance across different data types and operations, and exploring intra-DPU synchronization primitives for parallel vector reduction.

## Task 1 Transferring Data between Main Memory and PIM-enabled Memory

This task was concerned with the data transfer bandwidth between the host CPU and the DPUs.

### Implementation Details

### Evaluation

### Analysis and Observations




## Task 2 AXPY

### Implementation Details

### Evaluation

### Analysis and Observations




## Task 3 Operations and Data Types

### Implementation Details

### Evaluation

### Analysis and Observations




## Task 4 Vector Reduction

### Implementation Details

### Evaluation

### Analysis and Observations