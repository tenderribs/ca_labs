import re, csv

# Shared configs to evaluate
shared_configs = [(3, 1), (2, 2), (0, 4), (0, 8)]  # (num_l, num_h)
schedulers = ["FCFS", "FRFCFS", "ATLAS", "BLISS"]

results = []

for scheduler in schedulers:
    for config in shared_configs:
        num_l, num_h = config
        file_path = f"report/ramulator_logs/{scheduler}_{num_l}L{num_h}H.log"

        with open(file_path, "r") as f:
            content = f.read()

        insts = re.findall(r"insts_retired_core_\d+: (\d+)", content)
        # Use multiline mode and anchor to start of line to avoid matching memory_access_cycles...
        cycles = re.findall(r"^\s*cycles_recorded_core_\d+: (\d+)", content, re.MULTILINE)

        assert insts and cycles

        total_insts = sum(int(i) for i in insts)
        total_cycles = max(int(c) for c in cycles)

        throughput = total_insts / total_cycles

        results.append(
            {
                "scheduler": scheduler,
                "num_l": num_l,
                "num_h": num_h,
                "inst_thrput": throughput,
            }
        )

with open("report/res_inst_thrput.csv", "w", newline="") as f:
    writer = csv.DictWriter(f, fieldnames=results[0].keys())
    writer.writeheader()
    writer.writerows(results)
