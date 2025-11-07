"""Plot instructions per tasklet as the tasklet count varies."""

from __future__ import annotations

import csv
from pathlib import Path
from typing import Dict, List
import matplotlib.pyplot as plt

# Set matplotlib to use CMU fonts (Computer Modern, LaTeX default)
plt.rcParams["font.family"] = "serif"
plt.rcParams["font.size"] = 10

import numpy as np


CSV_FILENAME = "task2_out_32_64_dpus.csv"


def load_instructions_per_tasklet(csv_path: Path):
    """Return dict mapping DPU count to (tasklet_array, per_tasklet_instr_array)."""

    data: Dict[int, Dict[str, List]] = {}

    with csv_path.open(newline="") as csv_file:
        reader = csv.DictReader(csv_file)
        for row in reader:
            n_tsklts = int(row["n_tsklts"])
            n_dpus = int(row["n_dpus"])
            instr_cnt = float(row["dpu_instr_cnt"])

            if n_tsklts <= 0:
                continue

            if n_dpus not in data:
                data[n_dpus] = {"tasklets": [], "per_tasklet_instr": []}

            data[n_dpus]["tasklets"].append(n_tsklts)
            data[n_dpus]["per_tasklet_instr"].append(instr_cnt / n_tsklts)

    # Sort by tasklet count for each DPU count
    result = {}
    for n_dpus, values in data.items():
        order = np.argsort(values["tasklets"])
        tasklet_arr = np.asarray(values["tasklets"], dtype=float)[order]
        per_tasklet_arr = np.asarray(values["per_tasklet_instr"], dtype=float)[order]
        result[n_dpus] = (tasklet_arr, per_tasklet_arr)

    return result


def plot_instructions(data: Dict[int, tuple]) -> None:
    """Render a line plot of instructions-per-tasklet vs tasklet count for each DPU count."""

    fig, ax = plt.subplots(figsize=(8, 5))

    # Plot a line for each DPU count
    for n_dpus in sorted(data.keys()):
        tasklets, per_tasklet_instr = data[n_dpus]
        ax.plot(tasklets, per_tasklet_instr, marker="o", linestyle="-",
                linewidth=1.5, label=f"{n_dpus} DPUs")

    ax.set_xlabel("Number of Tasklets")
    ax.set_ylabel("Instructions per Tasklet")
    ax.set_title("Instruction Count per Tasklet vs Tasklet Count")
    ax.legend()
    ax.grid(True, linestyle="--", alpha=0.4)

    fig.tight_layout()
    plt.show()


def main() -> None:
    csv_path = Path(__file__).with_name(CSV_FILENAME)
    data = load_instructions_per_tasklet(csv_path)
    plot_instructions(data)


if __name__ == "__main__":
    main()