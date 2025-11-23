#include "ghb_pccs.h"

#include <limits>

void ghb_pccs::prefetcher_initialize()
{
  head_counter = 0;

  for (auto& entry : it) {
    entry.ghb_ptr = INVALID_PTR;
    entry.tag = 0;
    entry.valid = false;
  }

  for (auto& entry : ghb) {
    entry.block = 0;
    entry.full_ptr = INVALID_PTR;
    entry.prev = INVALID_PTR;
    entry.pc_tag = 0;
  }
}

uint32_t ghb_pccs::prefetcher_cache_operate(champsim::address addr, champsim::address ip, uint8_t cache_hit, bool useful_prefetch, access_type type,
                                            uint32_t metadata_in)
{
  (void)cache_hit;
  (void)useful_prefetch;
  (void)type;

  const uint64_t addr_value = addr.to<uint64_t>();
  const uint64_t ip_value = ip.to<uint64_t>();

  const uint64_t block = addr_value >> LOG2_BLOCK_SIZE;
  const uint32_t ip_index = static_cast<uint32_t>(ip_value) & (IT_SZ - 1);
  const uint64_t ip_tag = static_cast<uint64_t>(ip_value >> IT_SZ_LOG2);

  ITEntry& it_entry = it[ip_index];
  uint32_t prev_ptr = INVALID_PTR;

  if (it_entry.valid && it_entry.tag == ip_tag) {
    prev_ptr = sanitize_pointer(it_entry.ghb_ptr, ip_tag);
  }

  std::array<uint64_t, HISTORY_LENGTH> history{};
  std::size_t history_size = 0;
  uint32_t walker = prev_ptr;

  // Walk the GHB to retrieve the history of addresses for this PC
  while (history_size < HISTORY_LENGTH && pointer_valid(walker, ip_tag)) {
    const auto& ghb_entry = ghb[walker & (GHB_SZ - 1)];
    history[history_size++] = ghb_entry.block;
    walker = sanitize_pointer(ghb_entry.prev, ip_tag);
  }

  prev_ptr = sanitize_pointer(prev_ptr, ip_tag);

  // compute strides if enough data is available
  if (history_size >= 2) {
    // int64_t because strides can be negative (for example: "for (i=N; i>0; i--)")
    const int64_t stride1 = static_cast<int64_t>(block) - static_cast<int64_t>(history[0]);
    const int64_t stride2 = static_cast<int64_t>(history[0]) - static_cast<int64_t>(history[1]);

    if (stride1 == stride2 && stride1 != 0) {
      // Detected constant stride. Issue prefetches to addresses: A + l*d, A + (l+1)*d, ..., A + (l+n-1)*d
      for (std::size_t i = 0; i < PREFETCH_DEGREE; ++i) {
        const int64_t pf_block = static_cast<int64_t>(block) + stride1 * static_cast<int64_t>(PREFETCH_DISTANCE + i);
        if (pf_block < 0) { // physical addresses cannot be negative
          break;
        }

        const uint64_t pf_addr = static_cast<uint64_t>(pf_block) << LOG2_BLOCK_SIZE;
        prefetch_line(champsim::address{pf_addr}, true, 0);
      }
    }
  }

  // update the datastructures with latest miss
  const uint32_t current_ptr = head_counter;
  GHBEntry& ghb_entry = ghb[current_ptr & (GHB_SZ - 1)];
  ghb_entry.block = block;
  ghb_entry.full_ptr = current_ptr;
  ghb_entry.prev = prev_ptr;
  ghb_entry.pc_tag = ip_tag;

  it_entry.ghb_ptr = current_ptr;
  it_entry.tag = ip_tag;
  it_entry.valid = true;

  ++head_counter;

  return metadata_in;
}

bool ghb_pccs::pointer_valid(const uint32_t& pointer, const uint64_t& tag) const
{
  // 1. Future / Null check
  if (pointer == INVALID_PTR || pointer > head_counter)
    return false;

  //  2. FIFO eviction check
  if ((head_counter - pointer) > GHB_SZ)
    return false;

  // 3. Stale Data / Overwrite Check
  const GHBEntry& entry = ghb[pointer & (GHB_SZ - 1)];
  if (entry.full_ptr != pointer)
    return false;

  // 4. PC Tag Match
  return entry.pc_tag == tag;
}

uint32_t ghb_pccs::sanitize_pointer(const uint32_t& pointer, const uint64_t& tag) const { return pointer_valid(pointer, tag) ? pointer : INVALID_PTR; }
