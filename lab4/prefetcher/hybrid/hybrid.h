#ifndef PREFETCHER_HYBRID_H
#define PREFETCHER_HYBRID_H

#include <map>
#include <vector>

#include "bop.h"
#include "ghb.h"
#include "modules.h"

class hybrid : public champsim::modules::prefetcher
{
private:
  bop bop_pref;
  ghb ghb_pref;

  // TOURNAMENT STATE
  // Range: 0 (Strong BOP) to 10 (Strong GHB). Start at 5.
  int tournament_counter = 5;

  // Shadow Table: Tracks recent predictions to verify accuracy
  // Key: Cache Line Address
  struct ShadowEntry {
    int creator; // 1=GHB, 2=BOP, 3=BOTH
    uint64_t timestamp;
  };

  // key: line_addr, value: ShadowEntry
  std::map<uint64_t, ShadowEntry> shadow_table;
  uint64_t access_count = 0;

  void update_shadow_table(uint64_t line_addr, int predictor_id);

public:
  using prefetcher::prefetcher;

  hybrid(CACHE* args) : prefetcher(args), ghb_pref(args), bop_pref(args) {}

  void prefetcher_initialize();
  uint32_t prefetcher_cache_operate(champsim::address addr, champsim::address ip, uint8_t cache_hit, bool useful_prefetch, access_type type,
                                    uint32_t metadata_in);
  uint32_t prefetcher_cache_fill(champsim::address addr, long set, long way, uint8_t prefetch, champsim::address evicted_addr, uint32_t metadata_in);
  void prefetcher_cycle_operate();

  void prefetcher_final_stats();

  // Stats
  uint32_t bop_prefetches = 0;
  uint32_t ghb_prefetches = 0;
};

#endif