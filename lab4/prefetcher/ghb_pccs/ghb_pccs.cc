#include "ghb_pccs.h"

#include <limits>

namespace
{
constexpr uint32_t IT_INDEX_MASK = IT_SZ - 1;
constexpr uint32_t TAG_MASK = (1u << NUM_IP_TAG_BITS) - 1;
constexpr uint32_t TAG_SHIFT = GHB_SZ_LOG2;
} // namespace

void ghb_pccs::prefetcher_initialize()
{
  head_counter = 0;

  for (auto& entry : it) {
    entry.index = INVALID_PTR;
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
  const uint32_t index = static_cast<uint32_t>(ip_value) & IT_INDEX_MASK;
  const uint32_t tag = static_cast<uint32_t>((ip_value >> TAG_SHIFT) & TAG_MASK);

  ITEntry& it_entry = it[index];
  uint32_t prev_ptr = INVALID_PTR;

  if (it_entry.valid && it_entry.tag == tag) {
    prev_ptr = sanitize_pointer(it_entry.index, tag);
  }

  std::array<uint64_t, HISTORY_LENGTH> history{};
  std::size_t history_size = 0;
  uint32_t walker = prev_ptr;

  while (history_size < HISTORY_LENGTH && pointer_valid(walker, tag)) {
    const auto& ghb_entry = ghb[walker & (GHB_SZ - 1)];
    history[history_size++] = ghb_entry.block;
    walker = sanitize_pointer(ghb_entry.prev, tag);
  }

  prev_ptr = sanitize_pointer(prev_ptr, tag);

  if (history_size >= 2) {
    const int64_t stride1 = static_cast<int64_t>(block) - static_cast<int64_t>(history[0]);
    const int64_t stride2 = static_cast<int64_t>(history[0]) - static_cast<int64_t>(history[1]);

    if (stride1 == stride2 && stride1 != 0) {
      for (std::size_t i = 0; i < PREFETCH_DEGREE; ++i) {
        const int64_t pf_block = static_cast<int64_t>(block) + stride1 * static_cast<int64_t>(PREFETCH_DISTANCE + i);
        if (pf_block < 0) {
          break;
        }

        const uint64_t pf_addr = static_cast<uint64_t>(pf_block) << LOG2_BLOCK_SIZE;
        prefetch_line(champsim::address{pf_addr}, true, 0);
      }
    }
  }

  const uint32_t current_ptr = head_counter;
  GHBEntry& ghb_entry = ghb[current_ptr & (GHB_SZ - 1)];
  ghb_entry.block = block;
  ghb_entry.full_ptr = current_ptr;
  ghb_entry.prev = prev_ptr;
  ghb_entry.pc_tag = tag;

  it_entry.index = current_ptr;
  it_entry.tag = tag;
  it_entry.valid = true;

  ++head_counter;

  return metadata_in;
}

uint32_t ghb_pccs::prefetcher_cache_fill(champsim::address addr, long set, long way, uint8_t prefetch, champsim::address evicted_addr, uint32_t metadata_in)
{
  (void)addr;
  (void)set;
  (void)way;
  (void)prefetch;
  (void)evicted_addr;

  return metadata_in;
}

bool ghb_pccs::pointer_valid(uint32_t pointer, uint32_t tag) const
{
  if (pointer == INVALID_PTR || pointer > head_counter)
    return false;

  if ((head_counter - pointer) > GHB_SZ)
    return false;

  const GHBEntry& entry = ghb[pointer & (GHB_SZ - 1)];
  if (entry.full_ptr != pointer)
    return false;

  return entry.pc_tag == tag;
}

uint32_t ghb_pccs::sanitize_pointer(uint32_t pointer, uint32_t tag) const { return pointer_valid(pointer, tag) ? pointer : INVALID_PTR; }
