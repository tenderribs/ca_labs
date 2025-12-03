#include "ghb.h"

#include <limits>

#include "cache.h" // Needed for intern_->sim_stats

void ghb::prefetcher_initialize()
{
  head_counter = 0;

  for (auto& entry : it) {
    entry.ghb_ptr = INVALID_PTR;
    entry.tag = 0;
    entry.valid = false;
    entry.confidence = 0;
  }

  for (auto& entry : ghb) {
    entry.block = 0;
    entry.full_ptr = INVALID_PTR;
    entry.prev = INVALID_PTR;
    entry.pc_tag = 0;
  }

  // Initialize adaptive state
  epoch_cycle_count = 0;
  last_pf_issued = 0;
  last_pf_useful = 0;
  current_prefetch_degree = MAX_DEGREE; // Start with max degree
}

uint32_t ghb::prefetcher_cache_operate(champsim::address addr, champsim::address ip, uint8_t cache_hit, bool useful_prefetch, access_type type,
                                       uint32_t metadata_in)
{
  (void)cache_hit;
  (void)useful_prefetch;
  (void)type;

  const uint64_t addr_value = addr.to<uint64_t>();
  const uint64_t ip_value = ip.to<uint64_t>();

  const uint64_t block = addr_value >> LOG2_BLOCK_SIZE;
  const uint16_t ip_index = static_cast<uint16_t>(ip_value) & (IT_SZ - 1);
  const uint16_t ip_tag = static_cast<uint16_t>((ip_value >> IT_SZ_LOG2) & IT_TAG_MASK);

  ITEntry& it_entry = it[ip_index];
  uint16_t prev_ptr = INVALID_PTR;

  // Reset state
  pending_prefetches.clear();

  if (it_entry.valid && it_entry.tag == ip_tag) {
    prev_ptr = sanitize_pointer(it_entry.ghb_ptr, ip_tag);
  }

  std::array<uint64_t, HISTORY_LENGTH> history{};
  std::size_t history_size = 0;
  uint16_t walker = prev_ptr;

  // Walk the GHB to retrieve the history of addresses for this PC
  while (history_size < HISTORY_LENGTH && pointer_valid(walker, ip_tag)) {
    const auto& ghb_entry = ghb[walker & (GHB_SZ - 1)];
    history[history_size++] = ghb_entry.block;
    walker = sanitize_pointer(ghb_entry.prev, ip_tag);
  }

  prev_ptr = sanitize_pointer(prev_ptr, ip_tag);

  // compute strides if enough data is available
  if (history_size >= 2) {
    // int64_t, as strides can be negative ("for (i=N; i>0; i--)")
    const int64_t stride1 = static_cast<int64_t>(block) - static_cast<int64_t>(history[0]);
    const int64_t stride2 = static_cast<int64_t>(history[0]) - static_cast<int64_t>(history[1]);

    if (stride1 == stride2 && stride1 != 0) {
      for (std::size_t i = 0; i < current_prefetch_degree; ++i) {
        // Issue prefetches to addresses: A + l*d, A + (l+1)*d, ..., A + (l+n-1)*d
        const int64_t pf_block = static_cast<int64_t>(block) + stride1 * static_cast<int64_t>(PREFETCH_DISTANCE + i);
        if (pf_block < 0) { // physical addresses cannot be negative
          break;
        }

        const uint64_t pf_addr = static_cast<uint64_t>(pf_block) << LOG2_BLOCK_SIZE;

        if (prefetch_enabled) {
          prefetch_line(champsim::address{pf_addr}, true, 0);
        } else {
          pending_prefetches.push_back(champsim::address{pf_addr});
        }
      }
    }
  }

  // update the IT and GHB data structures with latest miss
  GHBEntry& ghb_entry = ghb[head_counter & (GHB_SZ - 1)];
  ghb_entry.block = block;
  ghb_entry.full_ptr = head_counter;
  ghb_entry.prev = prev_ptr;
  ghb_entry.pc_tag = ip_tag;

  it_entry.ghb_ptr = head_counter;
  it_entry.tag = ip_tag;
  it_entry.valid = true;

  // ensure restriction of head_counter to (8 + 4) bits, as in paper
  head_counter = (head_counter + 1) & ((1 << GHB_PTR_BITS) - 1);

  return metadata_in;
}

void ghb::issue_pending_prefetches(uint32_t metadata_in)
{
  for (const auto& addr : pending_prefetches) {
    prefetch_line(addr, true, metadata_in);
  }
}

void ghb::prefetcher_cycle_operate()
{
  epoch_cycle_count++;

  if (epoch_cycle_count >= EPOCH_LENGTH) {
    // Update Stats
    uint64_t current_pf_issued = intern_->sim_stats.pf_issued;
    uint64_t current_pf_useful = intern_->sim_stats.pf_useful;

    uint64_t epoch_issued = current_pf_issued - last_pf_issued;
    uint64_t epoch_useful = current_pf_useful - last_pf_useful;

    last_pf_issued = current_pf_issued;
    last_pf_useful = current_pf_useful;

    if (epoch_issued == 0) // no updates to make
      return;

    // Compute accuracy
    double accuracy = (double)epoch_useful / (double)epoch_issued;

    // Get Bandwidth
    uint8_t bw_quantized = get_dram_bw();

    // Adjust degree
    int adjustment = 0;
    if (bw_quantized >= 12) { // >= 75%
      if (accuracy >= 0.90) {
        adjustment = 0;
      } else if (accuracy >= 0.50) {
        adjustment = -1;
      } else {
        adjustment = -2;
      }
    } else if (bw_quantized >= 4) { // 25% <= bw < 75%
      if (accuracy >= 0.90) {
        adjustment = 1;
      } else if (accuracy >= 0.50) {
        adjustment = 0;
      } else {
        adjustment = -1;
      }
    } else { // < 25%
      if (accuracy >= 0.90) {
        adjustment = 2;
      } else if (accuracy >= 0.50) {
        adjustment = 1;
      } else {
        adjustment = 0;
      }
    }

    // Apply adjustment with clamping
    int new_degree = (int)current_prefetch_degree + adjustment;
    if (new_degree > (int)MAX_DEGREE)
      new_degree = MAX_DEGREE;
    if (new_degree < (int)MIN_DEGREE)
      new_degree = MIN_DEGREE;

    current_prefetch_degree = (uint32_t)new_degree;

    // Reset cycle count
    epoch_cycle_count = 0;
  }
}

bool ghb::pointer_valid(const uint16_t& pointer, const uint16_t& tag) const
{
  // 1. Null check
  if (pointer == INVALID_PTR)
    return false;

  // 2. FIFO eviction check (Modular arithmetic for circular buffer)
  // Calculate distance: (head - pointer) modulo 2^GHB_PTR_BITS
  const uint32_t diff = (head_counter - pointer) & ((1 << GHB_PTR_BITS) - 1);
  if (diff > GHB_SZ)
    return false;

  // 3. Stale Data / Overwrite Check
  const GHBEntry& entry = ghb[pointer & (GHB_SZ - 1)];
  if (entry.full_ptr != pointer)
    return false;

  // 4. PC Tag Match
  return entry.pc_tag == tag;
}

uint16_t ghb::sanitize_pointer(const uint16_t& pointer, const uint16_t& tag) const { return pointer_valid(pointer, tag) ? pointer : INVALID_PTR; }
