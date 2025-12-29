import csv
import re
import os


# Parses the log file and returns a dictionary {core_id: cycles_recorded}.
def get_cycles(file_path):
    if not os.path.exists(file_path):
        return None

    with open(file_path, "r") as f:
        content = f.read()

    # Regex to match '  cycles_recorded_core_X: Y'
    # We use ^\s* to match start of line and indentation
    # We exclude 'memory_access_' by ensuring the key starts with cycles_recorded
    matches = re.findall(
        r"^\s*cycles_recorded_core_(\d+): (\d+)", content, re.MULTILINE
    )

    cycles_map = {}
    for core_id, cycles in matches:
        cycles_map[int(core_id)] = int(cycles)

    return cycles_map


def calculate_max_slowdown():
    schedulers = ["FCFS", "FRFCFS", "ATLAS", "BLISS"]
    # Shared configs to evaluate
    shared_configs = [(3, 1), (2, 2), (0, 4), (0, 8)]

    results = []

    for scheduler in schedulers:
        # 1. Get Baseline Cycles (Alone)

        # Low Intensity Alone
        low_alone_file = f"report/ramulator_logs/{scheduler}_1L0H.log"
        low_cycles_map = get_cycles(low_alone_file)
        assert low_cycles_map and 0 in low_cycles_map
        low_alone_cycles = low_cycles_map[0]

        # High Intensity Alone
        high_alone_file = f"report/ramulator_logs/{scheduler}_0L1H.log"
        high_cycles_map = get_cycles(high_alone_file)
        assert high_cycles_map and 0 in high_cycles_map
        high_alone_cycles = high_cycles_map[0]

        # 2. Calculate Slowdown for Shared Configs
        for num_l, num_h in shared_configs:
            config_name = f"{num_l}L{num_h}H"
            log_file = f"report/ramulator_logs/{scheduler}_{config_name}.log"

            cycles_map = get_cycles(log_file)
            assert cycles_map

            h_slowdowns, l_slowdowns = [], []

            # Cores 0 to num_l-1 are Low
            for i in range(num_l):
                if i in cycles_map:
                    l_s = cycles_map[i] / low_alone_cycles
                    l_slowdowns.append(l_s)

            # Cores num_l to num_l+num_h-1 are High
            for i in range(num_l, num_l + num_h):
                if i in cycles_map:
                    h_s = cycles_map[i] / high_alone_cycles
                    h_slowdowns.append(h_s)

            # some configs don't have any cores with low intensity traces
            if len(l_slowdowns):
                max_l_slowdown = max(l_slowdowns)
            else:
                max_l_slowdown = 0

            max_h_slowdown = max(h_slowdowns)

            results.append(
                {
                    "scheduler": scheduler,
                    "num_l": num_l,
                    "num_h": num_h,
                    "max_slowdown_l": round(max_l_slowdown, 6),
                    "max_slowdown_h": round(max_h_slowdown, 6),
                    "max_slowdown": round(max(max_h_slowdown, max_l_slowdown), 6),
                }
            )

    return results


if __name__ == "__main__":
    results = calculate_max_slowdown()

    with open("report/res_max_slowdowns.csv", "w", newline="") as f:
        writer = csv.DictWriter(f, fieldnames=results[0].keys())
        writer.writeheader()
        writer.writerows(results)
