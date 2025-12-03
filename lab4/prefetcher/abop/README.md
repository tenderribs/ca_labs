This folder holds my extended version of Michaud's (2016) best offset prefetcher.

The prefetcher uses the current DRAM bandwidth usage state to vary the aggressiveness of the issued prefetches. The performance is worse on average than the original paper's (which issues only one prefetch for a given miss).