"""Plot instruction counts per element by operation and data type."""

from __future__ import annotations

import csv
import importlib
from collections import defaultdict
from pathlib import Path
from typing import Dict, List

import numpy as np
import matplotlib.pyplot as plt

# Set matplotlib to use CMU fonts (Computer Modern, LaTeX default)
plt.rcParams["font.family"] = "serif"
plt.rcParams["font.size"] = 10

CSV_FILENAME = "task3_out.csv"
OPERATIONS = ["OP_ADD", "OP_SUB", "OP_MULT", "OP_DIV"]


def load_data(csv_path: Path) -> Dict[str, Dict[str, float]]:
	"""Return per-element instruction counts indexed by operation and data type."""

	per_op_dtype: Dict[str, Dict[str, float]] = defaultdict(dict)

	with csv_path.open(newline="") as csv_file:
		reader = csv.DictReader(csv_file)
		for row in reader:
			op = row["op"].strip()
			dtype = row["dtype"].strip()
			if op not in OPERATIONS:
				continue

			instr_cnt = float(row["dpu_instr_cnt"])
			input_size = float(row["inp_size"])
			if input_size <= 0:
				continue

			per_element = instr_cnt / input_size
			per_op_dtype[op][dtype] = per_element

	return per_op_dtype


def plot_grouped_bars(per_op_dtype: Dict[str, Dict[str, float]]) -> None:
	"""Render a grouped bar chart with operations on the x-axis."""

	dtypes = sorted({dtype for values in per_op_dtype.values() for dtype in values})
	ops = [op for op in OPERATIONS if op in per_op_dtype]

	group_spacing = 0.2
	x_pos = np.arange(len(ops), dtype=float) * (1 + group_spacing)
	bar_width = 0.15
	offsets = ((np.arange(len(dtypes)) - (len(dtypes) - 1) / 2) * bar_width)

	fig, ax = plt.subplots(figsize=(10, 6))

	for offset, dtype in zip(offsets, dtypes):
		heights = [per_op_dtype[op].get(dtype, np.nan) for op in ops]
		ax.bar(x_pos + offset, heights, width=bar_width, label=dtype)

	ax.set_xticks(x_pos)
	ax.set_xticklabels([op.replace("OP_", "") for op in ops])
	ax.set_ylabel("Instructions per Arithmetic Operation")
	ax.set_xlabel("Operation")
	# ax.set_title("Instruction Cost per Arithmetic Operation")
	ax.grid(axis="y", linestyle="--", alpha=0.4)
	ax.legend(title="Data Type")

	fig.tight_layout()
	plt.show()


def main() -> None:
	csv_path = Path(__file__).with_name(CSV_FILENAME)
	per_op_dtype = load_data(csv_path)
	plot_grouped_bars(per_op_dtype)


if __name__ == "__main__":
	main()
