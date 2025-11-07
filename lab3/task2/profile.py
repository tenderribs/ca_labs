# build programs first with build.sh

OUTFILE = "task2_out.csv"

import subprocess as sp
import re
import csv
import os

def gen_params():
    params = []
    # dtypes = [("INT32", 4),("INT64", 8),("FLOAT", 4),("DOUBLE", 8),("CHAR", 1),("SHORT", 2)]
    inp_sizes = [8]  # in MB
    dpu_cnts = [32]
    tasklets_cnts = [i + 1 for i in range(0, 24, 1)]
    dtypes = [("INT32", 4)]

    for inp_size_mb in inp_sizes:
        for n_dpus in dpu_cnts:
            for dtype in dtypes:
                for n_tasklets in tasklets_cnts:
                    # use at least 8MB per DPU
                    inp_size = int(n_dpus * inp_size_mb * 1024 * 1024 / dtype[1])

                    params.append(
                        {
                            "inp_size_mb": inp_size_mb,
                            "inp_size": inp_size,
                            "n_dpus": n_dpus,
                            "dtype": dtype[0],
                            "block": 9,
                            "n_tsklts": n_tasklets,
                        }
                    )
    return params


def run(params_list):
    results = []

    for idx, p in enumerate(params_list):
        print(f"[{idx+1}/{len(params_list)}] {p}")

        # Run the program with specified parameters
        env = {
            **os.environ,
            "NR_DPUS": str(p["n_dpus"]),
            "NR_TASKLETS": str(p["n_tsklts"]),
            "BLOCK": str(p["block"]),
            "TYPE": p["dtype"],
            "TRANSFER": "PARALLEL",
            "PERF": "INSTRUCTIONS",
        }

        sp.run(["make", "clean"], check=True, capture_output=True, text=True, timeout=120)
        sp.run(["make"], env=env, check=True, capture_output=True, text=True, timeout=120)
        cmd = ["./bin/host_code", "-w", "2", "-e", "10", "-i", str(p["inp_size"]), "-a", "20"]
        result = sp.run(cmd, capture_output=True, text=True, timeout=120)
        stdout = result.stdout

        assert "Outputs are equal" in stdout

        cpu_dpu_match = re.search(r"CPU-DPU Time \(ms\):\s+([\d.]+)", stdout)
        dpu_cpu_match = re.search(r"DPU-CPU Time \(ms\):\s+([\d.]+)", stdout)
        dpu_kernel_match = re.search(r"DPU Kernel Time \(ms\):\s+([\d.]+)", stdout)
        dpu_inst_match = re.search(r"DPU instructions\s*=\s*([0-9.+-eE]+)", stdout)

        assert cpu_dpu_match and dpu_cpu_match and dpu_kernel_match and dpu_inst_match

        perf = {
            "cpu_dpu": float(cpu_dpu_match.group(1)),
            "dpu_cpu": float(dpu_cpu_match.group(1)),
            "dpu_kernel_time": float(dpu_kernel_match.group(1)),
            "dpu_instr_cnt": float(dpu_inst_match.group(1)),
        }
        results.append({**perf, **p})

    return results


if __name__ == "__main__":
    results = run(gen_params())
    print("DONE")

    # save results to disk
    if results:
        with open(OUTFILE, "w", newline="") as f:
            writer = csv.DictWriter(f, fieldnames=results[0].keys())
            writer.writeheader()
            writer.writerows(results)
