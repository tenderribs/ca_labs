#ifndef RAMULATOR_CONTROLLER_IMPL_ATLAS_COMMON_H
#define RAMULATOR_CONTROLLER_IMPL_ATLAS_COMMON_H

#include <vector>
#include <map>
#include <algorithm>
#include <iostream>

#include "base/base.h"

namespace Ramulator {

// -----------------------------------------------------------------------------
// Shared Global State (The "Meta-Controller")
// -----------------------------------------------------------------------------
struct AtlasSharedState {
    // Configuration
    Clk_t quantum_length = 10000000; // Default 10M cycles
    double alpha = 0.875;            // Default alpha
    Clk_t threshold = 100000;        // Default threshold 100K cycles

    // Shared Clock (Updated by Plugin, Read by Scheduler)
    Clk_t current_clk = 0;

    // State
    // Thread ID -> Total Attained Service (Historical)
    std::map<int, double> total_as;

    // Thread ID -> Rank (0 is highest priority/least attained service)
    std::map<int, int> ranking;

    // Buffer to accumulate Local AS from all controllers at quantum end
    std::map<int, double> pending_global_as;

    // Synchronization for quantum updates
    int num_controllers = 0;
    int controllers_checked_in = 0;

    // Helper to get rank (default to lowest priority if unknown)
    int get_rank(int source_id) {
        if (ranking.find(source_id) != ranking.end()) {
            return ranking[source_id];
        }
        return 999999;
    }

    void register_controller() {
        num_controllers++;
    }

    // Called by each controller at the end of a quantum
    void check_in_quantum(const std::map<int, double>& local_as) {
        // Accumulate local AS into the pending global sum
        for (auto const& [tid, as] : local_as) {
            pending_global_as[tid] += as;
        }

        controllers_checked_in++;

        // If all controllers have reported, update TotalAS and Ranks
        if (controllers_checked_in == num_controllers) {
            update_rankings();

            // Reset for next quantum
            pending_global_as.clear();
            controllers_checked_in = 0;
        }
    }

    void update_rankings() {
        // Rule 2: Coordination at the end of a quantum
        // Equation 1: TotalAS = alpha * TotalAS + (1 - alpha) * AS_current
        for (auto const& [tid, current_as] : pending_global_as) {
            if (total_as.find(tid) == total_as.end()) {
                total_as[tid] = 0;
            }
            total_as[tid] = (alpha * total_as[tid]) + ((1.0 - alpha) * current_as);
        }

        // Create a vector of thread IDs to sort
        std::vector<int> threads;
        for (auto const& [tid, val] : total_as) {
            threads.push_back(tid);
        }

        // Sort based on TotalAS (Least Attained Service = Higher Priority)
        std::sort(threads.begin(), threads.end(), [&](int a, int b) {
            // Tie-breaking with thread ID for stability
            if (total_as[a] != total_as[b])
                return total_as[a] < total_as[b];
            return a < b;
        });

        // Assign ranks
        ranking.clear();
        for (size_t i = 0; i < threads.size(); ++i) {
            ranking[threads[i]] = i;
        }
    }
};

// Global instance declaration (defined in atlas_scheduler.cpp)
extern AtlasSharedState atlas_state;

} // namespace Ramulator

#endif // RAMULATOR_CONTROLLER_IMPL_ATLAS_COMMON_H