#include <vector>
#include <map>

#include "base/base.h"
#include "dram_controller/controller.h"
#include "dram_controller/scheduler.h"
#include "dram_controller/impl/atlas_common.h"

namespace Ramulator {

// Define the global shared state here
AtlasSharedState atlas_state;

class ATLAS : public IScheduler, public Implementation {
    RAMULATOR_REGISTER_IMPLEMENTATION(IScheduler, ATLAS, "ATLAS", "Adaptive per-Thread Least-Attained-Service Scheduler")

    private:
        IDRAM* m_dram;

    public:
        void init() override { };

        void setup(IFrontEnd* frontend, IMemorySystem* memory_system) override {
            m_dram = cast_parent<IDRAMController>()->m_dram;
        };

        // Comparison function implementing Rule 1 from the paper
        ReqBuffer::iterator compare(ReqBuffer::iterator req1, ReqBuffer::iterator req2) override {
            // Use the shared clock updated by the plugin
            Clk_t current_clk = atlas_state.current_clk;

            bool req1_over = (current_clk - req1->arrive) > atlas_state.threshold;
            bool req2_over = (current_clk - req2->arrive) > atlas_state.threshold;

            // 1. TH-Over-threshold-requests-first
            if (req1_over ^ req2_over) {
                return req1_over ? req1 : req2;
            }

            // 2. LAS-Higher-LAS-rank-thread-first
            // Lower rank index means higher priority (Least Attained Service)
            int rank1 = atlas_state.get_rank(req1->source_id);
            int rank2 = atlas_state.get_rank(req2->source_id);

            if (rank1 != rank2) {
                return (rank1 < rank2) ? req1 : req2;
            }

            // 3. RH-Row-hit-first
            bool ready1 = m_dram->check_ready(req1->command, req1->addr_vec);
            bool ready2 = m_dram->check_ready(req2->command, req2->addr_vec);

            if (ready1 ^ ready2) {
                return ready1 ? req1 : req2;
            }

            // 4. FCFS-Oldest-first
            if (req1->arrive != req2->arrive) {
                return (req1->arrive < req2->arrive) ? req1 : req2;
            }

            return req1;
        }

        ReqBuffer::iterator get_best_request(ReqBuffer& buffer) override {
            if (buffer.size() == 0) {
                return buffer.end();
            }

            for (auto& req : buffer) {
                req.command = m_dram->get_preq_command(req.final_command, req.addr_vec);
            }

            auto candidate = buffer.begin();
            for (auto next = std::next(buffer.begin(), 1); next != buffer.end(); next++) {
                candidate = compare(candidate, next);
            }
            return candidate;
        }
};

} // namespace Ramulator