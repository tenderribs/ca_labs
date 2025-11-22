import time, os, glob, argparse, multiprocessing as mp, subprocess as sp
from typing import List, Dict, Any

warmup_cycles = 10_000_000
sim_cycles = 50_000_000


def distribute_tasks(executable: str) -> List[Dict[str, Any]]:
    """distribute traces as tasks to worker pool, SORTED by estimated difficulty"""
    all_traces = glob.glob("traces/*/*.gz")

    # Create task tuples
    tasks = [(trace, executable) for trace in all_traces]

    # heuristic sorting so the longest running tasks start first.
    def get_difficulty_score(task_tuple):
        trace_path = task_tuple[0]
        if "bc-" in trace_path:
            return 100  # GAP/bc (Betweenness Centrality) is heaviest (~14m)
        if "bfs-" in trace_path:
            return 80  # GAP/bfs (Breadth First Search) is heavy (~10-13m)
        if "cc-" in trace_path:
            return 60  # GAP/cc & sssp are medium (~8-9m)
        if "sssp-" in trace_path:
            return 50
        if "charlie" in trace_path:
            return 10  # charlie is fast (~5-7m)
        assert False

    tasks.sort(key=get_difficulty_score, reverse=True)

    results = []
    # Explicitly setting processes=12 to match your core count
    with mp.Pool(processes=12) as pool:
        # We use imap (ordered) or imap_unordered to dispatch the sorted list
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

    start = time.perf_counter()
    results: List[Dict[str, Any]] = distribute_tasks(args.executable)
    end = time.perf_counter()
    print(f"Completed after {end - start} s")

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
