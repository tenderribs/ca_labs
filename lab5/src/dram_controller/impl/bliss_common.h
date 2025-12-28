#ifndef RAMULATOR_CONTROLLER_IMPL_BLISS_COMMON_H
#define RAMULATOR_CONTROLLER_IMPL_BLISS_COMMON_H

#include <map>
#include <set>
#include "base/base.h"

namespace Ramulator {

struct BLISSState {
    int threshold = 4;
    int clearing_interval = 10000;

    int last_source_id = -1;
    int consecutive_requests = 0;
    std::set<int> blacklist;
    Clk_t last_clearing_tick = 0;
};

// Global map to store state per channel (key: channel_id)
extern std::map<int, BLISSState> bliss_states;

} // namespace Ramulator

#endif // RAMULATOR_CONTROLLER_IMPL_BLISS_COMMON_H