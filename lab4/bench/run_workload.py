import os, glob, argparse, multiprocessing as mp, subprocess as sp
from typing import List, Dict, Any

warmup_cycles = 10_000_000
sim_cycles = 50_000_000


def distribute_tasks(executable: str) -> List[Dict[str, Any]]:
    """distribute traces as tasks to worker pool"""
    all_traces = glob.glob("traces/*/*.gz") # run all traces by default
    tasks = [(trace, executable) for trace in all_traces]

    results = []
    with mp.Pool(processes=os.cpu_count()) as pool:
        for result in pool.imap_unordered(run, tasks):
            results.append(result)
    return results


def run(task_data) -> Dict[str, Any]:
    """run task as subprocess"""
    trace, executable = task_data

    # build args as a list (safer than splitting a string)
    args = [
        executable,
        f"--warmup-instructions={warmup_cycles}",
        f"--simulation-instructions={sim_cycles}",
        trace,
    ]
    print(f"RUNNING: {' '.join(args)}")

    result = sp.run(args, capture_output=True, text=True, timeout=30 * 60)

    return {
        "trace": trace,
        "args": args,
        "returncode": result.returncode,
        "stdout": result.stdout,
        "stderr": result.stderr,
    }


if __name__ == "__main__":


    parser = argparse.ArgumentParser()
    parser.add_argument("-e", "--executable", default="./bin/1C.fullBW.nopref")
    parser.add_argument("-o", "--output_file", default="workload_results.log")
    args = parser.parse_args()

    results: List[Dict[str, Any]] = distribute_tasks(args.executable)

    # save the outputs to a logfile
    with open(args.output_file, "w", encoding="utf-8") as f:
        for r in results:
            f.write(f"=== TRACE: {r['trace']} ===\n")
            f.write(f"COMMAND: { ' '.join(r['args']) }\n")
            f.write(f"RETURNCODE: {r['returncode']}\n\n")
            f.write("--- STDOUT ---\n")
            f.write(r.get("stdout", ""))
            f.write("\n\n--- STDERR ---\n")
            f.write(r.get("stderr", ""))
            f.write("\n\n\n")
