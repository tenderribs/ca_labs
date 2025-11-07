# build programs first with build.sh
progs = {
    "serial": "./host_code_serial",
    "parallel": "./host_code_parallel",
    "broadcast": "./host_code_broadcast",
}

OUTFILE = "task1_out.csv"

import subprocess as sp
import re
import csv

def gen_params():
    input_sizes = [1, 24, 48]  # in MB
    dpu_cnts = list(range(1, 64 + 1, 1))

    res = []

    for input_size in input_sizes:
        for dpu_cnt in dpu_cnts:
            for tf_method in progs.keys():
                res.append((input_size, dpu_cnt, tf_method))
    return res


def run(params_list):
    results = []

    for idx, params in enumerate(params_list):
        # T = int64_t
        input_size = int(params[0] * 1024 * 1024 / 8)
        dpu_cnt = params[1]
        method = params[2]
        prog = progs[method]

        print(
            f"[{idx+1}/{len(params_list)}] Running: {method}, {dpu_cnt} DPUs, {params[0]}MB..."
        )

        # Run the program with specified parameters
        cmd = [prog, "-w", "2", "-e", "10", "-d", str(dpu_cnt), "-i", str(input_size)]
        # print((' '.join(cmd)))
        result = sp.run(cmd, capture_output=True, text=True, timeout=120)
        stdout = result.stdout

        if "Outputs are equal" not in stdout:
            print(" ".join(cmd))
            print(stdout)
            assert False

        cpu_dpu_match = re.search(r"CPU-DPU Time \(ms\):\s+([\d.]+)", stdout)
        dpu_cpu_match = re.search(r"DPU-CPU Time \(ms\):\s+([\d.]+)", stdout)

        if cpu_dpu_match and dpu_cpu_match:
            cpu_dpu_time = float(cpu_dpu_match.group(1))
            dpu_cpu_time = float(dpu_cpu_match.group(1))

            results.append(
                {
                    "input_size_mb": params[0],
                    "dpu_cnt": dpu_cnt,
                    "tf_method": method,
                    "cpu_dpu_ms": cpu_dpu_time,
                    "dpu_cpu_ms": dpu_cpu_time,
                }
            )

    return results


if __name__ == "__main__":
    results = run(gen_params())

    # save results to disk
    if(results):
        with open(OUTFILE, 'w', newline='') as f:
                writer = csv.DictWriter(f, fieldnames=results[0].keys())
                writer.writeheader()
                writer.writerows(results)