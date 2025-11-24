import matplotlib.pyplot as plt
import pandas as pd
import numpy as np

# Set matplotlib to use CMU fonts (Computer Modern, LaTeX default)
plt.rcParams["font.family"] = "serif"
plt.rcParams["font.size"] = 10


def load_data(path: str) -> pd.DataFrame:
    df = pd.read_csv(path)  # C.fullBW.nopref.json
    df = df.sort_values(by="trace")

    # Clean trace names
    df["trace"] = df["trace"].str.replace(".trace.gz", "", regex=False)
    df["trace"] = df["trace"].str.replace(".champsim.gz", "", regex=False)
    df["trace"] = df["trace"].str.replace(r"_0+(\d+)", r"_\1", regex=True)

    return df


def merge_on_trace(df1: pd.DataFrame, df2: pd.DataFrame) -> pd.DataFrame:
    merged = df1[["trace", "ipc"]].merge(
        df2[["trace", "ipc"]], on="trace", suffixes=("_1", "_2")
    )
    return merged


def show_pretty_bars(bars_handle):
    # Add IPC values on top of each bar
    for bar in bars_handle:
        height = bar.get_height()
        plt.text(
            bar.get_x() + bar.get_width() / 2.0,
            height,
            f"{height:.3f}",
            ha="center",
            va="bottom",
        )

    plt.xticks(rotation=45, ha="right")
    plt.tight_layout()
    plt.show()


def task_0():
    """Generate plots for the warmup task"""
    df = load_data("report/data/task0_1C_fullBW_nopref.csv")

    bars = plt.bar(df["trace"], df["ipc"])
    show_pretty_bars(bars)


def rel_speedup(path1: str, path2: str, plot: bool) -> pd.DataFrame:
    """Plots for GHB-based PC/CS prefetcher"""
    merged: pd.DataFrame = merge_on_trace(load_data(path1), load_data(path2))

    # Calculate speedup
    merged["speedup"] = merged["ipc_1"] / merged["ipc_2"]

    # Add row containing geomean of speedup
    geomean = np.exp(np.log(merged["speedup"]).mean())
    geomean_row = pd.DataFrame({"trace": ["GEOMEAN"], "speedup": [geomean]})
    merged = pd.concat([merged, geomean_row], ignore_index=True)

    bars = plt.bar(merged["trace"], merged["speedup"])

    if plot:
        show_pretty_bars(bars)

    return merged


def compare_rel_speedups(df_full, df_limited):
    merged = df_full[["trace", "speedup"]].merge(
        df_limited[["trace", "speedup"]], on="trace", suffixes=("_full", "_limited")
    )

    print(merged)
    # bars = plt.bar(merged["trace"], merged["speedup"])


if __name__ == "__main__":
    # task_0()  # task 0  No prefetcher

    # print("FULL BW")
    full_bw = rel_speedup(  # task 1 Full BW
        "report/data/task0_1C_fullBW_nopref.csv",
        "report/data/task1_1C_fullBW_ghb_pccs_fixed_pd_20b_ip_tag.csv",
        plot=False,
    )

    # print("Reduced BW")
    limited_bw = rel_speedup(  # task 2 Limited BW
        "report/data/task2_1C_limitBW_nopref.csv",
        "report/data/task2_1C_limitBW_ghb_pccs_fixed_pd.csv",
        plot=False,
    )

    # print("Reduced BW")
    compare_rel_speedups(full_bw, limited_bw)
