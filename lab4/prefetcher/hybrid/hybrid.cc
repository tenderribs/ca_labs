#include "hybrid.h"

#include <iostream>

#include "dpc_api.h"

void hybrid::prefetcher_initialize()
{
  ghb_pref.hybrid_mode = true;
  bop_pref.hybrid_mode = true;

  ghb_pref.prefetcher_initialize();
  bop_pref.prefetcher_initialize();
  tournament_counter = 5; // Neutral start
}

// Helper to track virtual predictions
void hybrid::update_shadow_table(uint64_t line_addr, int predictor_id)
{
  auto it = shadow_table.find(line_addr);
  if (it == shadow_table.end()) {
    // New entry
    shadow_table[line_addr] = {predictor_id, access_count};
  } else {
    // Entry exists. If creator is different, mark as BOTH (3)
    if (it->second.creator != predictor_id) {
      it->second.creator = 3;
    }
    it->second.timestamp = access_count; // Touch LRU
  }

  // Cleanup old entries (simple garbage collection)
  if (shadow_table.size() > 256) {
    shadow_table.erase(shadow_table.begin());
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

    // If BOP predicted it (2) or Both (3), push counter toward BOP (0)
    if (creator == 2 || creator == 3) {
      if (tournament_counter > 0)
        tournament_counter--;
    }

    // If GHB predicted it (1) or Both (3), push counter toward GHB (10)
    if (creator == 1 || creator == 3) {
      if (tournament_counter < 10)
        tournament_counter++;
    }
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
  if (bop.valid_prefetch) {
    update_shadow_table(line, 2);
  }

  // 4. ARBITRATION
  uint8_t bw = get_dram_bw();

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

  if (issue_bop) {
    // BOP requires a sanity check on its own score (must be > BAD_SCORE)
    // Assuming you exposed get_best_score() in bop.h
    if (bop_pref.get_best_score() > 1) {
      for (uint64_t line : bop_raw) {
        prefetch_line(champsim::address{line << LOG2_BLOCK_SIZE}, true, metadata_in);
        bop_prefetches++;
      }
    }
  }

  return metadata_in;
}

void hybrid::prefetcher_final_stats()
{
  std::cout << "GHB Prefetches: " << ghb_prefetches << std::endl;
  std::cout << "BOP Prefetches: " << bop_prefetches << std::endl;
}