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


def rel_speedup(with_pref: str, no_pref: str, plot: bool) -> pd.DataFrame:
    """Plots for GHB-based PC/CS prefetcher"""
    df_with_pref, df_no_pref = load_data(with_pref), load_data(no_pref)

    merged = df_with_pref[["trace", "ipc"]].merge(
        df_no_pref[["trace", "ipc"]], on="trace", suffixes=("_w_pref", "_no_pref")
    )

    # Calculate speedup
    merged["speedup"] = merged["ipc_w_pref"] / merged["ipc_no_pref"]

    # Add row containing geomean of speedup
    geomean = np.exp(np.log(merged["speedup"]).mean())
    geomean_row = pd.DataFrame({"trace": ["GEOMEAN"], "speedup": [geomean]})
    merged = pd.concat([merged, geomean_row], ignore_index=True)

    print(f"geomean {geomean}")

    if plot:
        bars = plt.bar(merged["trace"], merged["speedup"])
        show_pretty_bars(bars)

    return merged


def compare_rel_speedups(df_full, df_limited):
    merged = df_full[["trace", "speedup"]].merge(
        df_limited[["trace", "speedup"]], on="trace", suffixes=("_full", "_limited")
    )

    x = np.arange(len(merged["trace"]))
    width = 0.35

    plt.figure(figsize=(10, 4))
    bars_full = plt.bar(x - width / 2, merged["speedup_full"], width, label="Full BW")
    bars_limited = plt.bar(
        x + width / 2, merged["speedup_limited"], width, label="Limited BW"
    )

    plt.xticks(x, merged["trace"], rotation=45, ha="right")
    plt.ylabel("Relative speedup")
    plt.legend()
    plt.tight_layout()

    return merged


if __name__ == "__main__":
    # ======= Task 1 =======
    # No prefetchers
    # task_0()

    # ======= Task 1 =======
    # Full BW
    fixed_full_bw = rel_speedup(
        with_pref="report/data/task1_1C_fullBW_ghb_pccs_fixed_pd_20b_ip_tag.csv",
        no_pref="report/data/task0_1C_fullBW_nopref.csv",
        plot=False,
    )

    # ======= Task 2 =======
    # Limited BW
    fixed_limited_bw = rel_speedup(
        with_pref="report/data/task2_1C_limitBW_ghb_pccs_fixed_pd.csv",
        no_pref="report/data/task2_1C_limitBW_nopref.csv",
        plot=False,
    )

    # # Full BW vs Limited BW
    # compare_rel_speedups(df_full=fixed_full_bw, df_limited=fixed_limited_bw)

    # ======= Task 3 =======
    adaptive_full_bw = rel_speedup(
        with_pref="report/data/task3_1C_fullBW_ghb_pccs_adaptive_pd.csv",
        no_pref="report/data/task0_1C_fullBW_nopref.csv",
        plot=False,
    )

    adaptive_limited_bw = rel_speedup(
        with_pref="report/data/task3_1C_limitBW_ghb_pccs_adaptive_pd.csv",
        no_pref="report/data/task2_1C_limitBW_nopref.csv",
        plot=False,
    )

    plt.show()
