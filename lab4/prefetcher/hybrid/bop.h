#ifndef PREFETCHER_BOP_H
#define PREFETCHER_BOP_H

#include <array>
#include <cstdint>
#include <vector>

#include "champsim.h"
#include "modules.h"

class bop : public champsim::modules::prefetcher
{
  // Constants
  static constexpr int RR_TABLE_SIZE = 256;
  static constexpr int SCORE_TABLE_SIZE = 52;
  static constexpr int MAX_SCORE = 31;
  static constexpr int BAD_SCORE = 1;
  static constexpr int ROUND_MAX = 100;
  static constexpr int MAX_DEGREE = 3;
  static constexpr int PAGE_SHIFT = 12;

  // Data Structures
  // Candidates list (ROM)
  static const std::array<int, SCORE_TABLE_SIZE> candidates;

  // Recent Requests (RR) Table
  std::vector<int> rr_table;

  // Scoreboard
  std::vector<int> scores;

  // Global State Registers
  int best_offset;
  int current_degree;
  int round_counter;
  int access_counter;

  int hash(const uint64_t& line_addr) const;

public:
  using prefetcher::prefetcher;

  // hybrid sub-prefetcher related members
  bool hybrid_mode = false;     // select if GHB pref. is being used as part of hybrid pref. or standalone
  uint64_t best_line;           // best candidate line found by BOP to be prefetched
  bool best_line_valid = false; // does best_line meet conditions for hybrid prefetcher to issue prefetch?

  bool prefetch_enabled = true;

  void prefetcher_initialize();
  // void prefetcher_branch_operate(champsim::address ip, uint8_t branch_type, champsim::address branch_target) {}
  uint32_t prefetcher_cache_operate(champsim::address addr, champsim::address ip, uint8_t cache_hit, bool useful_prefetch, access_type type,
                                    uint32_t metadata_in);
  uint32_t prefetcher_cache_fill(champsim::address addr, long set, long way, uint8_t prefetch, champsim::address evicted_addr, uint32_t metadata_in);
  // void prefetcher_cycle_operate() {}
  // void prefetcher_final_stats() {}
};

#endif
