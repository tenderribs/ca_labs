#include <vector>
#include <map>
#include <set>

#include "base/base.h"
#include "dram_controller/controller.h"
#include "dram_controller/plugin.h"
#include "dram_controller/impl/bliss_common.h"

namespace Ramulator {

class BLISSPlugin : public IControllerPlugin, public Implementation {
    RAMULATOR_REGISTER_IMPLEMENTATION(IControllerPlugin, BLISSPlugin, "BLISS", "BLISS Plugin.")

    private:
        int m_channel_id = -1;
        int m_threshold = 4;
        int m_clearing_interval = 10000;

        // Local clock tracker since we cannot access the controller's protected m_clk
        Clk_t m_clk = 0;

    public:
        void init() override { };

        void setup(IFrontEnd* frontend, IMemorySystem* memory_system) override {
            m_ctrl = cast_parent<IDRAMController>();
            m_channel_id = m_ctrl->m_channel_id;

            // Initialize state parameters in the shared map
            auto& state = bliss_states[m_channel_id];
            state.threshold = m_threshold;
            state.clearing_interval = m_clearing_interval;
        }

        void update(bool request_found, ReqBuffer::iterator& req_it) override {
            m_clk++; // Increment local clock every cycle

            auto& state = bliss_states[m_channel_id];

            // Periodically clear blacklist using local clock
            if (m_clk - state.last_clearing_tick >= state.clearing_interval) {
                state.blacklist.clear();
                state.last_clearing_tick = m_clk;
            }

            if (request_found) {
                int source_id = req_it->source_id;
                // Count if this command completes the request (Read or Write)
                // If we are just opening a row (ACT), we haven't "serviced" the request yet.
                if (req_it->command == req_it->final_command) {
                    // Track consecutive requests from the same source
                    if (source_id == state.last_source_id) {
                        state.consecutive_requests++;
                    } else {
                        state.last_source_id = source_id;
                        state.consecutive_requests = 1;
                    }
                }

                // Blacklist if threshold exceeded
                if (state.consecutive_requests > state.threshold) {
                    state.blacklist.insert(source_id);
                    // Reset consecutive requests so we don't re-insert repeatedly (optional optimization)
                    // state.consecutive_requests = 0;
                    // Note: The paper implies once blacklisted, it stays until cleared.
                    // Keeping the counter effectively keeps it blacklisted.
                }
            }
        }
};

} // namespace Ramulator