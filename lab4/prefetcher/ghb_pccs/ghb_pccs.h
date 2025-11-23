#ifndef PREFETCHER_GHB_PCCS_H
#define PREFETCHER_GHB_PCCS_H

#include <array>
#include <cstdint>
#include <limits>

#include "champsim.h"
#include "modules.h"

#define IT_SZ 256 // Size of the index table
#define IT_SZ_LOG2 8
#define GHB_SZ 256 // Size of the GHB
#define GHB_SZ_LOG2 8

struct GHBEntry {
  uint64_t block;    // current cache-line number
  uint32_t full_ptr; // monotonically increasing “virtual pointer” (head counter value)
  uint32_t prev;     // Pointer to previous GHB entry for same PC
  uint64_t pc_tag;   // Tag of the PC
};

struct ITEntry {
  uint32_t ghb_ptr; // low bits of the last GHB slot for this PC
  uint64_t tag;     // Tag of the PC
  bool valid;       // Validity bit
};

class ghb_pccs : public champsim::modules::prefetcher
{
public:
  using prefetcher::prefetcher;

  void prefetcher_initialize();
  // void prefetcher_branch_operate(champsim::address ip, uint8_t branch_type, champsim::address branch_target) {}
  uint32_t prefetcher_cache_operate(champsim::address addr, champsim::address ip, uint8_t cache_hit, bool useful_prefetch, access_type type,
                                    uint32_t metadata_in);
  // uint32_t prefetcher_cache_fill(champsim::address addr, long set, long way, uint8_t prefetch, champsim::address evicted_addr, uint32_t metadata_in);
  // void prefetcher_cycle_operate() {}
  // void prefetcher_final_stats() {}

private:
  static constexpr uint32_t PREFETCH_DISTANCE = 4; // ensure the first prefetched block arrives in time
  static constexpr uint32_t PREFETCH_DEGREE = 6;
  static constexpr std::size_t HISTORY_LENGTH = 2;
  static constexpr uint32_t INVALID_PTR = std::numeric_limits<uint32_t>::max();

  std::array<ITEntry, IT_SZ> it{};
  std::array<GHBEntry, GHB_SZ> ghb{};
  uint32_t head_counter = 0;

  bool pointer_valid(const uint32_t& pointer, const uint64_t& tag) const;

  /**
   * helper to safely traverse the linked list
   */
  uint32_t sanitize_pointer(const uint32_t& pointer, const uint64_t& tag) const;
};

#endif
