#ifndef PREFETCHER_GHB_PCCS_ADAPTIVE_H
#define PREFETCHER_GHB_PCCS_ADAPTIVE_H

#include <array>
#include <cstdint>
#include <limits>

#include "champsim.h"
#include "modules.h"

#define IT_SZ 256 // Size of the index table
#define IT_SZ_LOG2 8
#define GHB_SZ 256 // Size of the GHB
#define GHB_SZ_LOG2 8
#define GHB_PTR_BITS (GHB_SZ_LOG2 + 4)

// Paper states: "Index Table entries are relatively small; they contain a
// tag (for matching) and a single pointer into the GHB (on the order of 1-2 bytes)"
// => ghb_ptr ~ 12 bits, tag ~ 8 bits, total 20 bits = 2.5 Bytes (close enough)
#define NUM_IT_TAG_BITS 8 // Tag bits per IP table entry
#define IT_TAG_MASK ((1 << NUM_IT_TAG_BITS) - 1)

struct GHBEntry {
  uint64_t block;    // current cache-line number
  uint16_t full_ptr; // monotonically increasing “virtual pointer” (head counter value)
  uint16_t prev;     // Pointer to previous GHB entry for same PC
  uint16_t pc_tag;   // Tag of the PC
};

// only lower bits of the ptr and tag members are used to have same restrictions as in real hardware.
struct ITEntry {
  uint16_t ghb_ptr; // lower 12 bits (paper adds 4 bits to ghb size) of the last GHB slot for this PC
  uint16_t tag;     // lower 8 bit tag of the PC
  bool valid;       // Validity bit
};

class ghb_pccs_adaptive : public champsim::modules::prefetcher
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
  static constexpr uint32_t INVALID_PTR = std::numeric_limits<uint16_t>::max();

  std::array<ITEntry, IT_SZ> it{};
  std::array<GHBEntry, GHB_SZ> ghb{};
  uint16_t head_counter = 0; // 32b width allows for use of INVALID_PTR

  bool pointer_valid(const uint16_t& pointer, const uint16_t& tag) const;

  /**
   * helper to safely traverse the linked list
   */
  uint16_t sanitize_pointer(const uint16_t& pointer, const uint16_t& tag) const;
};

#endif
