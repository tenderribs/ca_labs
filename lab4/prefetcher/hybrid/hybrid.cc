#include "hybrid.h"

#include <iostream>

#include "dpc_api.h"

void hybrid::prefetcher_initialize()
{
  ghb_pref.prefetcher_initialize();
  bop_pref.prefetcher_initialize();

  tournament_counter = 5; // Neutral start

  // override subprefetcher defaults set by prefetcher_initialize()
  ghb_pref.hybrid_mode = true;
  bop_pref.hybrid_mode = true;
}

// Helper to track virtual predictions
void hybrid::update_shadow_table(uint64_t line_addr, int predictor_id)
{
  auto line_addr_it = shadow_table.find(line_addr);
  if (line_addr_it == shadow_table.end()) {
    // New entry
    shadow_table[line_addr] = {predictor_id, access_count};
  } else {
    // Entry exists. If creator is different, mark as BOTH (3)
    if (line_addr_it->second.creator != predictor_id) {
      line_addr_it->second.creator = 3;
    }
    line_addr_it->second.timestamp = access_count; // Touch LRU
  }

  // Cleanup old entries (simple garbage collection)
  if (shadow_table.size() > 256) {
    // Find LRU
    auto lru_it = shadow_table.begin();
    for (auto it = shadow_table.begin(); it != shadow_table.end(); ++it) {
      if (it->second.timestamp < lru_it->second.timestamp) {
        lru_it = it;
      }
    }
    shadow_table.erase(lru_it);
  }
}

uint32_t hybrid::prefetcher_cache_operate(champsim::address addr, champsim::address ip, uint8_t cache_hit, bool useful_prefetch, access_type type,
                                          uint32_t metadata_in)
{
  access_count++;
  uint64_t current_line = addr.to<uint64_t>() >> LOG2_BLOCK_SIZE;

  // 1. SCORING
  // Did anyone predict this current access recently?
  auto it = shadow_table.find(current_line);
  if (it != shadow_table.end()) {
    int creator = it->second.creator;

    // BOP only
    if (creator == 2 && tournament_counter > 0) {
      tournament_counter--;

    } // GHB only
    else if (creator == 1 && tournament_counter < 10) {
      tournament_counter++;
    }
    // If creator == 3 (Both), do nothing (neutral).

    shadow_table.erase(it); // Consumed
  }

  // 2. RUN SUB-PREFETCHERS (Without actually issuing prefetches)
  ghb_pref.prefetcher_cache_operate(addr, ip, cache_hit, useful_prefetch, type, metadata_in);
  bop_pref.prefetcher_cache_operate(addr, ip, cache_hit, useful_prefetch, type, metadata_in);

  // 3. GATHER CANDIDATES
  const std::vector<champsim::address>& ghb_raw = ghb_pref.pending_prefetches;
  const uint64_t bop_best_line = bop_pref.best_line;

  // Populate Shadow Table with what they would have done
  // GHB:
  for (const auto& addr_obj : ghb_raw) {
    update_shadow_table(addr_obj.to<uint64_t>() >> LOG2_BLOCK_SIZE, 1);
  }
  // BOP:
  if (bop_pref.best_line_valid) {
    update_shadow_table(bop_best_line, 2);
  }

  // 4. ARBITRATION
  bool issue_ghb = false;
  bool issue_bop = false;

  // Contended BW: Pick the Winner only
  if (tournament_counter >= 6)
    issue_ghb = true; // Leaning GHB
  else if (tournament_counter <= 4)
    issue_bop = true; // Leaning BOP
  else {              // Neutral -> Both
    issue_ghb = true;
    issue_bop = true;
  }

  // Final Issue
  if (issue_ghb) {
    for (const auto& addr_obj : ghb_raw) {
      prefetch_line(addr_obj, true, metadata_in);
      ghb_prefetches++;
    }
  }

  if (issue_bop && bop_pref.best_line_valid) {
    prefetch_line(champsim::address{bop_best_line << LOG2_BLOCK_SIZE}, true, metadata_in);
    bop_prefetches++;
  }

  return metadata_in;
}

void hybrid::prefetcher_cycle_operate()
{
  // allow ghb prefetcher to update accuracy measurements
  ghb_pref.prefetcher_cycle_operate();
}

uint32_t hybrid::prefetcher_cache_fill(champsim::address addr, long set, long way, uint8_t prefetch, champsim::address evicted_addr, uint32_t metadata_in)
{
  // allow the bop prefetcher to update its RR table
  return bop_pref.prefetcher_cache_fill(addr, set, way, prefetch, evicted_addr, metadata_in);
}

void hybrid::prefetcher_final_stats()
{
  std::cout << "GHB Prefetches: " << ghb_prefetches << std::endl;
  std::cout << "BOP Prefetches: " << bop_prefetches << std::endl;
}