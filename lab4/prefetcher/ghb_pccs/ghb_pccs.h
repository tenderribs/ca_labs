#ifndef PREFETCHER_GHB_PCCS_H
#define PREFETCHER_GHB_PCCS_H

#include <cstdint>

#include "champsim.h"
#include "modules.h"

#define IT_SIZE 256  // Size of the index table
#define GHB_SIZE 256 // Size of the GHB

#define NUM_IP_INDEX_BITS 10 // Bits to index into the IP table
#define NUM_IP_TAG_BITS 6    // Tag bits per IP table entry

// class idx_table
// {
// public:
//   uint64_t link_ptr;
// };

class ghb_pccs : public champsim::modules::prefetcher
{
public:
  using prefetcher::prefetcher;

  // void prefetcher_initialize() {}
  // void prefetcher_branch_operate(champsim::address ip, uint8_t branch_type, champsim::address branch_target) {}
  uint32_t prefetcher_cache_operate(champsim::address addr, champsim::address ip, uint8_t cache_hit, bool useful_prefetch, access_type type,
                                    uint32_t metadata_in);
  uint32_t prefetcher_cache_fill(champsim::address addr, long set, long way, uint8_t prefetch, champsim::address evicted_addr, uint32_t metadata_in);
  // void prefetcher_cycle_operate() {}
  // void prefetcher_final_stats() {}

private:
  uint64_t it[IT_SIZE];
  uint64_t ghb[GHB_SIZE];
};

#endif
