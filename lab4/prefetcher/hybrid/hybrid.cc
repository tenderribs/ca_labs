#include "hybrid.h"

#include <iostream>

#include "dpc_api.h" // For get_dram_bw()

uint32_t hybrid::prefetcher_cache_operate(champsim::address addr, champsim::address ip, uint8_t cache_hit, bool useful_prefetch, access_type type,
                                          uint32_t metadata_in)
{
  // Disable automatic prefetching for sub-prefetchers
  ghb_pref.prefetch_enabled = false;
  bop_pref.prefetch_enabled = false;

  // Update state of both prefetchers
  ghb_pref.prefetcher_cache_operate(addr, ip, cache_hit, useful_prefetch, type, metadata_in);
  bop_pref.prefetcher_cache_operate(addr, ip, cache_hit, useful_prefetch, type, metadata_in);

  // Tournament Logic

  // 1. Gather Metrics
  bool ghb_confident = ghb_pref.is_confident;
  int bop_score = bop_pref.get_best_score();
  uint8_t bw = get_dram_bw(); // 0 (empty) to 16 (full)

  // 2. GHB is high precision, so we prioritize it.
  if (ghb_confident) {
    ghb_pref.issue_pending_prefetches(metadata_in);
    ghb_prefetches++;
  }

  // 3. BOP Decision (The Global Observer)
  // BOP is allowed if it has a high enough score relative to system congestion.

  int bop_threshold = 1; // Base threshold (must be > BAD_SCORE=1)
  if (bw >= 13) {
      bop_threshold = 20;
  } else if (bw >= 9) {
      bop_threshold = 5;
  } else {
      bop_threshold = 1;
  }

  // If GHB is also firing, we should be more careful with BOP to avoid flooding
  if (ghb_confident) {
      bop_threshold += 10; // Significant penalty if GHB is already using BW
  }

  // Issue BOP if score meets threshold
  if (bop_score > bop_threshold) {
    bop_pref.issue_prefetch(addr, cache_hit, metadata_in);
    bop_prefetches++;
  }

  return metadata_in;
}

uint32_t hybrid::prefetcher_cache_fill(champsim::address addr, long set, long way, uint8_t prefetch, champsim::address evicted_addr, uint32_t metadata_in)
{
  return metadata_in;
}

void hybrid::prefetcher_final_stats()
{
  std::cout << "GHB Prefetches: " << ghb_prefetches << std::endl;
  std::cout << "BOP Prefetches: " << bop_prefetches << std::endl;
}
