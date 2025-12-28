#include <vector>
#include <map>

#include "base/base.h"
#include "dram_controller/controller.h"
#include "dram_controller/plugin.h"
#include "dram_controller/impl/atlas_common.h"
#include "memory_system/memory_system.h"

namespace Ramulator {

class ATLASPlugin : public IControllerPlugin, public Implementation {
    RAMULATOR_REGISTER_IMPLEMENTATION(IControllerPlugin, ATLASPlugin, "ATLAS", "Plugin for ATLAS Scheduler state tracking")

    private:
        IDRAM* m_dram = nullptr;

        // Local memory controller clock tracker
        Clk_t m_clk = 0;

        // Local Attained Service for the current quantum
        std::map<int, double> local_as;

        // Track number of active banks per thread
        // Key: Thread ID, Value: Count of active banks
        std::map<int, int> active_banks_count;

        // Map to track which thread owns which bank
        // Key: Bank Address (flattened vector), Value: Thread ID
        std::map<std::vector<int>, int> bank_ownership;

        int m_row_level = -1;

    public:
        void init() override {
            atlas_state.register_controller();
        };

        void setup(IFrontEnd* frontend, IMemorySystem* memory_system) override {
            m_dram = memory_system->get_ifce<IDRAM>();
            // Determine which level in hierarchy represents the row (hierarchy levels above are banks)
            m_row_level = m_dram->m_levels("row");
        };

        void update(bool request_found, ReqBuffer::iterator& req_it) override {
            m_clk++;
            // Update global shared clock so Scheduler can read it next cycle
            atlas_state.current_clk = m_clk;

            // 1. Update Attained Service (AS)
            // "AS for a thread is incremented every cycle by the number of banks
            // that are servicing that thread's requests."
            for (auto const& [tid, count] : active_banks_count) {
                if (count > 0) {
                    local_as[tid] += count;
                }
            }

            // 2. Track Banks Servicing status based on issued commands
            if (request_found) {
                // Determine the bank address vector (everything above row)
                std::vector<int> bank_addr_vec;
                if (req_it->addr_vec.size() >= m_row_level) {
                    bank_addr_vec.assign(req_it->addr_vec.begin(), req_it->addr_vec.begin() + m_row_level);
                }

                auto& meta = m_dram->m_command_meta(req_it->command);

                // If opening a row (ACT), thread claims the bank
                if (meta.is_opening) {
                    int tid = req_it->source_id;

                    // If bank was already active (e.g., row hit from same thread),
                    // check ownership to be safe, though usually handled by controller logic.
                    if (bank_ownership.find(bank_addr_vec) != bank_ownership.end()) {
                         int old_owner = bank_ownership[bank_addr_vec];
                         active_banks_count[old_owner]--;
                    }

                    bank_ownership[bank_addr_vec] = tid;
                    active_banks_count[tid]++;
                }

                // If closing a row (PRE), thread releases the bank
                if (meta.is_closing) {
                    if (bank_ownership.find(bank_addr_vec) != bank_ownership.end()) {
                        int owner = bank_ownership[bank_addr_vec];
                        active_banks_count[owner]--;
                        if (active_banks_count[owner] < 0) active_banks_count[owner] = 0; // Safety

                        bank_ownership.erase(bank_addr_vec);
                    }
                }
            }

            // 3. Check Quantum Expiration
            if (m_clk % atlas_state.quantum_length == 0) {
                // Report to Meta-Controller
                atlas_state.check_in_quantum(local_as);

                // Reset local AS
                local_as.clear();
            }
        }
};

} // namespace Ramulator