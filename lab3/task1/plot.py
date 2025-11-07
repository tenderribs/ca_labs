"""Plot measured transfer bandwidths for CPU<->DPU transfers."""

from __future__ import annotations

import csv
from collections import defaultdict
from pathlib import Path
from typing import Dict, Iterable, List, Tuple
import matplotlib.pyplot as plt

# Set matplotlib to use CMU fonts (Computer Modern, LaTeX default)
plt.rcParams["font.family"] = "serif"
plt.rcParams["font.size"] = 10

CSV_FILENAME = "task1_out.csv"
CPU_TO_DPU_METHODS = {"serial", "parallel", "broadcast"}
DPU_TO_CPU_METHODS = {"serial", "parallel"}
METHOD_STYLES: Dict[str, Dict[str, str]] = {
    "serial": {"linestyle": "-", "marker": "o"},
    "parallel": {"linestyle": "--", "marker": "s"},
    "broadcast": {"linestyle": ":", "marker": "x"},
}


def _bandwidth_mb_per_s(size_mb: float, time_ms: float) -> float:
    if time_ms <= 0:
        return float("nan")
    return size_mb * 1000.0 / time_ms


def load_bandwidths(
    csv_path: Path,
) -> Tuple[
    Dict[Tuple[str, float], List[Tuple[int, float]]],
    Dict[Tuple[str, float], List[Tuple[int, float]]],
]:
    cpu_to_dpu: Dict[Tuple[str, float], List[Tuple[int, float]]] = defaultdict(list)
    dpu_to_cpu: Dict[Tuple[str, float], List[Tuple[int, float]]] = defaultdict(list)

    with csv_path.open(newline="") as csv_file:
        reader = csv.DictReader(csv_file)
        for row in reader:
            method = row["tf_method"].strip()
            dpu_cnt = int(row["dpu_cnt"])
            input_size_mb = float(row["input_size_mb"])

            if method in CPU_TO_DPU_METHODS:
                bw_cpu_to_dpu = _bandwidth_mb_per_s(input_size_mb, float(row["cpu_dpu_ms"]))
                cpu_to_dpu[(method, input_size_mb)].append((dpu_cnt, bw_cpu_to_dpu))

            if method in DPU_TO_CPU_METHODS:
                bw_dpu_to_cpu = _bandwidth_mb_per_s(input_size_mb, float(row["dpu_cpu_ms"]))
                dpu_to_cpu[(method, input_size_mb)].append((dpu_cnt, bw_dpu_to_cpu))

    def _sort_entries(data: Dict[Tuple[str, float], List[Tuple[int, float]]]) -> None:
        for key in data:
            data[key].sort(key=lambda item: item[0])

    _sort_entries(cpu_to_dpu)
    _sort_entries(dpu_to_cpu)
    return cpu_to_dpu, dpu_to_cpu


def _plot_direction(
    ax,
    data: Dict[Tuple[str, float], List[Tuple[int, float]]],
    title: str,
    include_methods: Iterable[str],
    size_colors: Dict[float, Tuple[float, float, float]],
) -> None:
    for (method, size_mb), samples in data.items():
        if method not in include_methods:
            continue
        dpu_counts = [point[0] for point in samples]
        bandwidths = [point[1] for point in samples]
        label = f"{int(size_mb)} MB - {method.capitalize()}"

        style = METHOD_STYLES.get(method, {})
        linestyle = style.get("linestyle", "-")
        marker = style.get("marker", "o")
        line_color = size_colors.get(size_mb, (0.2, 0.2, 0.2))
        ax.plot(
            dpu_counts,
            bandwidths,
            marker=marker,
            linewidth=1.5,
            linestyle=linestyle,
            color=line_color,
            label=label,
        )

    ax.set_title(title)
    ax.set_xlabel("Number of DPUs")
    ax.set_ylabel("Bandwidth (MB/s)")
    ax.grid(True, linestyle="--", alpha=0.35)
    ax.legend(fontsize="small", ncol=2)


def plot_bandwidths(
    cpu_to_dpu: Dict[Tuple[str, float], List[Tuple[int, float]]],
    dpu_to_cpu: Dict[Tuple[str, float], List[Tuple[int, float]]],
) -> None:
    unique_sizes = sorted({size for _, size in cpu_to_dpu} | {size for _, size in dpu_to_cpu})
    cmap = plt.get_cmap("tab10", max(len(unique_sizes), 1))
    size_colors: Dict[float, Tuple[float, float, float]] = {}
    for index, size in enumerate(unique_sizes):
        rgba = cmap(index % cmap.N)
        size_colors[size] = (rgba[0], rgba[1], rgba[2])

    fig, axes = plt.subplots(2, 1, figsize=(10, 10), sharex=True)

    _plot_direction(
        axes[0],
        cpu_to_dpu,
        title="CPU -> DPU Transfer Bandwidth",
        include_methods=CPU_TO_DPU_METHODS,
        size_colors=size_colors,
    )

    _plot_direction(
        axes[1],
        dpu_to_cpu,
        title="DPU -> CPU Transfer Bandwidth",
        include_methods=DPU_TO_CPU_METHODS,
        size_colors=size_colors,
    )

    axes[1].set_xlabel("Number of DPUs")

    fig.tight_layout()
    plt.show()


def main() -> None:
    csv_path = Path(__file__).with_name(CSV_FILENAME)
    cpu_to_dpu, dpu_to_cpu = load_bandwidths(csv_path)
    plot_bandwidths(cpu_to_dpu, dpu_to_cpu)


if __name__ == "__main__":
    main()
