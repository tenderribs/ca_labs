"""Plot instructions per tasklet as the tasklet count varies."""

from __future__ import annotations

import csv
import importlib
from pathlib import Path
from typing import List, Tuple
import matplotlib.pyplot as plt

# Set matplotlib to use CMU fonts (Computer Modern, LaTeX default)
plt.rcParams["font.family"] = "serif"
plt.rcParams["font.size"] = 10

import numpy as np


CSV_FILENAME = "task2_out_32_64_dpus.csv"


def load_instructions_per_tasklet(csv_path: Path) -> Tuple[np.ndarray, np.ndarray]:
	"""Return arrays of tasklet counts and instructions-per-tasklet values."""

	tasklets: List[int] = []
	per_tasklet_instr: List[float] = []

	with csv_path.open(newline="") as csv_file:
		reader = csv.DictReader(csv_file)
		for row in reader:
			n_tsklts = int(row["n_tsklts"])
			instr_cnt = float(row["dpu_instr_cnt"])
			if n_tsklts <= 0:
				continue
			tasklets.append(n_tsklts)
			per_tasklet_instr.append(instr_cnt / n_tsklts)

	order = np.argsort(tasklets)
	tasklet_arr = np.asarray(tasklets, dtype=float)[order]
	per_tasklet_arr = np.asarray(per_tasklet_instr, dtype=float)[order]
	return tasklet_arr, per_tasklet_arr


def plot_instructions(tasklets: np.ndarray, per_tasklet_instr: np.ndarray) -> None:
	"""Render a line plot of instructions-per-tasklet vs tasklet count."""

	fig, ax = plt.subplots(figsize=(8, 5))
	ax.plot(tasklets, per_tasklet_instr, marker="o", linestyle="-", linewidth=1.5)
	ax.set_xlabel("Number of Tasklets")
	ax.set_ylabel("Instructions per Tasklet")
	ax.set_title("Instruction Count per Tasklet vs Tasklet Count")
	ax.grid(True, linestyle="--", alpha=0.4)

	fig.tight_layout()
	plt.show()


def main() -> None:
	csv_path = Path(__file__).with_name(CSV_FILENAME)
	tasklets, per_tasklet_instr = load_instructions_per_tasklet(csv_path)
	plot_instructions(tasklets, per_tasklet_instr)


if __name__ == "__main__":
	main()
