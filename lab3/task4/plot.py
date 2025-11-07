"""Plot grouped bar charts of instruction counts by final reduction method."""

import csv
from collections import defaultdict
from pathlib import Path
from typing import Dict, List

import numpy as np

import matplotlib.pyplot as plt

# Set matplotlib to use CMU fonts (Computer Modern, LaTeX default)
plt.rcParams["font.family"] = "serif"
plt.rcParams["font.size"] = 10

CSV_FILENAME = "task4_out_no_barr_64dpu.csv"
TARGET_SIZES_KB = [16.0, 1024.0, 16384.0]
FINAL_METHODS = ["SINGLE", "TREE_BARRIER", "TREE_HANDSHAKE", "MUTEX"]


def load_data(csv_path: Path) -> Dict[float, Dict[str, Dict[int, float]]]:
    """Load instruction counts grouped by input size, reduction method, and tasklets."""

    data: Dict[float, Dict[str, Dict[int, float]]] = defaultdict(
        lambda: defaultdict(dict)
    )

    with csv_path.open(newline="") as csv_file:
        reader = csv.DictReader(csv_file)
        for row in reader:
            inp_size_kb = float(row["inp_size_kb"])
            if inp_size_kb not in TARGET_SIZES_KB:
                continue

            method = row["final_reduc"].strip()
            if method not in FINAL_METHODS:
                continue

            tasklets = int(row["n_tsklts"])
            instr_count = float(row["dpu_instr_cnt"])
            data[inp_size_kb][method][tasklets] = instr_count

    return data


def prepare_tasklet_axis(values: Dict[str, Dict[int, float]]) -> List[int]:
    """Collect and sort the tasklet counts present in a given input-size slice."""

    tasklet_counts = set()
    for method_values in values.values():
        tasklet_counts.update(method_values.keys())

    return sorted(tasklet_counts)


def plot_grouped_bars(data: Dict[float, Dict[str, Dict[int, float]]]) -> None:
    """Render grouped bar charts for the requested input sizes."""

    fig, axes = plt.subplots(len(TARGET_SIZES_KB), 1, figsize=(10, 12), sharex=True)

    legend_handles = legend_labels = None
    final_tasklets: List[int] = []
    final_x_pos = np.array([])

    for ax, inp_size_kb in zip(axes, TARGET_SIZES_KB):
        slice_data = data.get(inp_size_kb, {})
        if not slice_data:
            ax.set_visible(False)
            continue

        tasklets = prepare_tasklet_axis(slice_data)
        x_pos = np.arange(len(tasklets), dtype=float)
        final_tasklets = tasklets
        final_x_pos = x_pos

        bar_width = 0.18
        offsets = (
            np.arange(len(FINAL_METHODS)) - (len(FINAL_METHODS) - 1) / 2
        ) * bar_width

        for offset, method in zip(offsets, FINAL_METHODS):
            method_values = slice_data.get(method, {})
            heights = [method_values.get(t, np.nan) for t in tasklets]
            ax.bar(x_pos + offset, heights, width=bar_width, label=method)

        if legend_handles is None:
            legend_handles, legend_labels = ax.get_legend_handles_labels()

        ax.set_title(f"Input Size: {int(inp_size_kb)} KB")
        ax.set_ylabel("Instruction Count")
        ax.grid(axis="y", linestyle="--", alpha=0.4)

    if final_tasklets:
        axes[-1].set_xlabel("Number of Tasklets")
        axes[-1].set_xticks(final_x_pos)
        axes[-1].set_xticklabels([str(t) for t in final_tasklets])

    # Place a single legend for all subplots.
    if legend_handles and legend_labels:
        fig.legend(
            legend_handles, legend_labels, loc="upper center", ncol=len(FINAL_METHODS)
        )

    fig.tight_layout(rect=(0, 0, 1, 0.95))
    plt.show()


def main() -> None:
    csv_path = Path(__file__).with_name(CSV_FILENAME)
    data = load_data(csv_path)
    plot_grouped_bars(data)


if __name__ == "__main__":
    main()
