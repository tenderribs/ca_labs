#include "bop.h"

#include <algorithm> // For std::sort
#include <utility>
#include <vector>

// Define candidates (offsets.py)
const std::array<int, bop::SCORE_TABLE_SIZE> bop::candidates = {1,   2,   3,   4,   5,   6,   8,   9,   10,  12,  15,  16,  18,  20,  24,  25,  27,  30,
                                                                32,  36,  40,  45,  48,  50,  54,  60,  64,  72,  75,  80,  81,  90,  96,  100, 108, 120,
                                                                125, 128, 135, 144, 150, 160, 162, 180, 192, 200, 216, 225, 240, 243, 250, 256};

int bop::hash(const uint64_t& line_addr) const
{
  // For a 256-entry RR table:
  // XOR the 8 least significant line address bits with the next 8 bits
  return (line_addr & 0xFF) ^ ((line_addr >> 8) & 0xFF);
}

void bop::prefetcher_initialize()
{
  rr_table.resize(RR_TABLE_SIZE, 0);
  scores.resize(SCORE_TABLE_SIZE, 0);

  current_degree = 1;
  best_offset = 1; // Default to next line

  round_counter = 0;
  access_counter = 0;
}

uint32_t bop::prefetcher_cache_fill(champsim::address addr, long set, long way, uint8_t prefetch, champsim::address evicted_addr, uint32_t metadata_in)
{
  // Normalize to Cache Line Address
  uint64_t fill_line = addr.to<uint64_t>() >> LOG2_BLOCK_SIZE;

  // Determine Trigger
  uint64_t trigger_line = fill_line;
  if (prefetch) {
    trigger_line = fill_line - best_offset;
  }

  // If the trigger and fill are on different pages, DO NOT update the RR table.
  // We check if the Page Frame Numbers (Line >> 6) are different.
  const uint64_t lines_per_page_log2 = LOG2_PAGE_SIZE - LOG2_BLOCK_SIZE;
  if ((trigger_line >> lines_per_page_log2) != (fill_line >> lines_per_page_log2)) {
    return metadata_in;
  }

  // Update the RR table
  int idx = hash(trigger_line);
  int tag_val = (trigger_line >> 8) & 0xFFF; // Paper states: "Skip the 8 least significant line address bits"
  rr_table[idx] = tag_val;

  return metadata_in;
}

uint32_t bop::prefetcher_cache_operate(champsim::address addr, champsim::address ip, uint8_t cache_hit, bool useful_prefetch, access_type type,
                                       uint32_t metadata_in)
{
  // only operate on cache misses or useful prefetch fills
  if (cache_hit && !useful_prefetch) {
    return metadata_in;
  }

  clear_candidates(); // RESET for this cycle

  // Normalize
  uint64_t current_line = addr.to<uint64_t>() >> LOG2_BLOCK_SIZE;

  // Part 1: The Learning Step
  const int candidate_idx = access_counter % SCORE_TABLE_SIZE;

  // Test Hypothesis
  const uint64_t test_line = current_line - candidates[candidate_idx];

  const int idx = hash(test_line);
  int tag_val = (test_line >> 8) & 0xFFF;

  if (rr_table[idx] == tag_val) {
    scores[candidate_idx]++;
    if (scores[candidate_idx] == MAX_SCORE) {
      // Trigger End of Round
      round_counter = ROUND_MAX;
    }
  }

  round_counter++;
  access_counter++;

  // Part 2: End of Round (Update State)
  if (round_counter >= ROUND_MAX) {
    // Sort: Identify the indices of the top 3 highest scores
    std::vector<std::pair<int, int>> score_indices(SCORE_TABLE_SIZE);
    for (int i = 0; i < SCORE_TABLE_SIZE; ++i) {
      score_indices[i] = {scores[i], i};
    }

    // Sort descending by score
    std::sort(score_indices.rbegin(), score_indices.rend());

    // Update Offsets
    best_offset = candidates[score_indices[0].second];

    // Check Threshold
    if (score_indices[0].first <= BAD_SCORE) {
      current_degree = 0; // Disable prefetch temporarily
    } else {
      // If score is good, ensure we are enabled (so Part 3 can adjust degree)
      current_degree = 1;
    }

    // Reset
    std::fill(scores.begin(), scores.end(), 0);
    round_counter = 0;
  }

  // Part 3: Prefetch Issue
  if (prefetch_enabled && current_degree > 0) {
    issue_prefetch(addr, cache_hit ? true : false, metadata_in);
  }

  return metadata_in;
}

int bop::get_best_score() const
{
  // Find max score
  int max_score = 0;
  for (int s : scores) {
    if (s > max_score)
      max_score = s;
  }
  return max_score;
}

void bop::issue_prefetch(champsim::address addr, bool cache_hit, uint32_t metadata_in)
{
  uint64_t current_line = addr.to<uint64_t>() >> LOG2_BLOCK_SIZE;
  uint64_t pf_line = current_line + best_offset;

  // Page Boundary Check
  if ((pf_line >> (LOG2_PAGE_SIZE - LOG2_BLOCK_SIZE)) == (current_line >> (LOG2_PAGE_SIZE - LOG2_BLOCK_SIZE))) {

    // If we are in "Hybrid Mode" (prefetch_enabled is false),
    // we just store the candidate for the controller.
    if (!prefetch_enabled) {
      generated_candidates.push_back(pf_line);
    } else {
      // Standalone mode
      prefetch_line(pf_line << LOG2_BLOCK_SIZE, cache_hit, metadata_in);
    }
  }
}
