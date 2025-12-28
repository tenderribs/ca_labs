#include <vector>
#include <map>
#include <set>
#include <algorithm>

#include "base/base.h"
#include "dram_controller/controller.h"
#include "dram_controller/scheduler.h"
#include "dram_controller/impl/bliss_common.h"

namespace Ramulator {

// Define the global map here
// key: channel id, value: bliss state
std::map<int, BLISSState> bliss_states;

class BLISSScheduler : public IScheduler, public Implementation {
    RAMULATOR_REGISTER_IMPLEMENTATION(IScheduler, BLISSScheduler, "BLISS", "BLISS Scheduler.")

    private:
        IDRAM* m_dram;
        int m_channel_id = -1;

    public:
        void init() override { };
        void setup(IFrontEnd* frontend, IMemorySystem* memory_system) override {
            m_dram = cast_parent<IDRAMController>()->m_dram;
            m_channel_id = cast_parent<IDRAMController>()->m_channel_id;
        }

        ReqBuffer::iterator compare(ReqBuffer::iterator req1, ReqBuffer::iterator req2) override {
            auto& state = bliss_states[m_channel_id];

            bool blacklisted1 = state.blacklist.count(req1->source_id);
            bool blacklisted2 = state.blacklist.count(req2->source_id);

            // 1) Non-blacklisted applications' requests are prioritized
            if (blacklisted1 != blacklisted2) {
                // If req1 is blacklisted and req2 is not, return req2.
                // If req1 is not and req2 is, return req1.
                return blacklisted1 ? req2 : req1;
            }

            // 2) Row-buffer hit requests
            bool ready1 = m_dram->check_ready(req1->command, req1->addr_vec);
            bool ready2 = m_dram->check_ready(req2->command, req2->addr_vec);
            if (ready1 != ready2) {
                return ready1 ? req1 : req2;
            }

            // 3) Older requests (FCFS)
            if (req1->arrive != req2->arrive) {
                return (req1->arrive < req2->arrive) ? req1 : req2;
            }

            return req1;
        }

        ReqBuffer::iterator get_best_request(ReqBuffer& buffer) override {
             if (buffer.size() == 0) {
                return buffer.end();
            }
            // Determine the preq command for all requests to check readiness
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