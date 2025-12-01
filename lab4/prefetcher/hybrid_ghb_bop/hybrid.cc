#include "hybrid.h"

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
  // Priority 1: GHB
  if (ghb_pref.is_confident) {
    ghb_pref.issue_pending_prefetches(metadata_in);
    ghb_prefetches++;
  }
  // Priority 2: BOP
  else {
    int bop_score = bop_pref.get_best_score();
    uint8_t bw = get_dram_bw();

    // Check if BOP score is high enough and bandwidth is sufficient
    if (bop_score > 1 && bw < 12) { // 1 is BAD_SCORE in bop.h
      bop_pref.issue_prefetch(addr, cache_hit, metadata_in);
      bop_prefetches++;
    }
  }

  return metadata_in;
}

uint32_t hybrid::prefetcher_cache_fill(champsim::address addr, long set, long way, uint8_t prefetch, champsim::address evicted_addr, uint32_t metadata_in)
{
  return metadata_in;
}
