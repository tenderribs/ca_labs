#ifndef PREFETCHER_HYBRID_H
#define PREFETCHER_HYBRID_H

#include <cstdint>

#include "bop.h"
#include "champsim.h"
#include "ghb_pccs.h"
#include "modules.h"

class hybrid : public champsim::modules::prefetcher
{
#define ADAPTIVE_PF_DEG // ensure GHB PC/CS has system feedback
private:
  bop bop_pref;
  ghb_pccs ghb_pref;

public:
  using prefetcher::prefetcher;

  // Initialize sub-prefetchers
  hybrid(CACHE* args) : prefetcher(args), ghb_pref(args), bop_pref(args) {};

  void prefetcher_initialize()
  {
    ghb_pref.prefetcher_initialize();
    bop_pref.prefetcher_initialize();
  }
  uint32_t bop_prefetches = 0;
  uint32_t ghb_prefetches = 0;
  // void prefetcher_branch_operate(champsim::address ip, uint8_t branch_type, champsim::address branch_target) {}
  uint32_t prefetcher_cache_operate(champsim::address addr, champsim::address ip, uint8_t cache_hit, bool useful_prefetch, access_type type,
                                    uint32_t metadata_in);
  uint32_t prefetcher_cache_fill(champsim::address addr, long set, long way, uint8_t prefetch, champsim::address evicted_addr, uint32_t metadata_in);
  // void prefetcher_cycle_operate() {}
  // void prefetcher_final_stats() {}
};

#endif
