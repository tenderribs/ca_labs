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


def task_0():
    """Generate plots for the warmup task"""
    df = load_data("report/data/task0_1C_fullBW_nopref.csv")

    bars = plt.bar(df["trace"], df["ipc"])

    # Add IPC values on top of each bar
    for bar in bars:
        height = bar.get_height()
        plt.text(
            bar.get_x() + bar.get_width() / 2.0,
            height,
            f"{height:.3f}",
            ha="center",
            va="bottom",
        )

    plt.ylabel("IPC")
    plt.xticks(rotation=45, ha="right")
    plt.tight_layout()


def rel_speedup(with_pref: str, no_pref: str, plot: bool) -> pd.DataFrame:
    """Plots for GHB-based PC/CS prefetcher"""
    df_with_pref, df_no_pref = load_data(with_pref), load_data(no_pref)

    # Select relevant columns from with_pref
    df_w = df_with_pref[
        ["trace", "ipc", "instructions", "l2c_prefetch_issued", "l2c_prefetch_useful"]
    ]
    df_n = df_no_pref[["trace", "ipc"]]

    merged = df_w.merge(df_n, on="trace", suffixes=("_w_pref", "_no_pref"))

    # Calculate speedup
    merged["speedup"] = merged["ipc_w_pref"] / merged["ipc_no_pref"]

    # Calculate Accuracy (%)
    # Handle division by zero if issued is 0
    merged["accuracy"] = (
        merged["l2c_prefetch_useful"] / merged["l2c_prefetch_issued"]
    ).fillna(0) * 100

    # Calculate PPKI
    merged["ppki"] = (merged["l2c_prefetch_issued"] / merged["instructions"]) * 1000

    # Add row containing geomean of speedup and means of others
    geomean_speedup = np.exp(np.log(merged["speedup"]).mean())
    geomean_accuracy = np.exp(np.log(merged["accuracy"]).mean())
    geomean_ppki = np.exp(np.log(merged["ppki"]).mean())

    geomean_row = pd.DataFrame(
        {
            "trace": ["GEOMEAN"],
            "speedup": [geomean_speedup],
            "accuracy": [geomean_accuracy],
            "ppki": [geomean_ppki],
        }
    )
    merged = pd.concat([merged, geomean_row], ignore_index=True)
    # print(merged)
    print(
        f"geomean speedup: {geomean_speedup} geomean accuracy: {geomean_accuracy} geomean ppki: {geomean_ppki} "
    )

    if plot:
        fig, (ax1, ax2, ax3) = plt.subplots(3, 1, figsize=(10, 12), sharex=True)

        # 1. IPC Speedup
        bars1 = ax1.bar(merged["trace"], merged["speedup"], color="tab:blue")
        ax1.set_ylabel("IPC Speedup")
        ax1.axhline(1.0, color="black", linewidth=0.8, linestyle="--")
        ax1.grid(axis="y", linestyle="--", alpha=0.7)

        # 2. Accuracy
        bars2 = ax2.bar(merged["trace"], merged["accuracy"], color="tab:green")
        ax2.set_ylabel("Accuracy (%)")
        ax2.set_ylim(0, 100)
        ax2.grid(axis="y", linestyle="--", alpha=0.7)

        # 3. PPKI
        bars3 = ax3.bar(merged["trace"], merged["ppki"], color="tab:red")
        ax3.set_ylabel("PPKI")
        ax3.grid(axis="y", linestyle="--", alpha=0.7)

        # Add values on top of bars
        def add_labels(ax, bars, fmt="{:.2f}"):
            for bar in bars:
                height = bar.get_height()
                ax.text(
                    bar.get_x() + bar.get_width() / 2.0,
                    height,
                    fmt.format(height),
                    ha="center",
                    va="bottom",
                    fontsize=8,
                )

        add_labels(ax1, bars1, "{:.2f}")
        add_labels(ax2, bars2, "{:.1f}")
        add_labels(ax3, bars3, "{:.1f}")

        plt.xticks(rotation=45, ha="right")
        # plt.xlabel("Trace")
        plt.tight_layout()

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
    plt.ylabel("IPC Speedup")
    plt.legend()
    plt.tight_layout()

    return merged


def compare_prefetchers(df1, df2, df3, label1, label2, label3, title):
    """
    Plots grouped bars of IPC speedup for three different prefetcher configurations.
    """
    # Rename columns to avoid collision and merge
    d1 = df1[["trace", "speedup"]].rename(columns={"speedup": "speedup_1"})
    d2 = df2[["trace", "speedup"]].rename(columns={"speedup": "speedup_2"})
    d3 = df3[["trace", "speedup"]].rename(columns={"speedup": "speedup_3"})

    merged = d1.merge(d2, on="trace").merge(d3, on="trace")

    # Filter out the GEOMEAN row for plotting if desired, or keep it.
    # Usually plotting GEOMEAN alongside traces is fine.

    x = np.arange(len(merged["trace"]))
    pos_offset = 0.27
    bar_width = 0.25

    plt.figure(figsize=(14, 6))

    # Plot bars
    bars1 = plt.bar(
        x - pos_offset,
        merged["speedup_1"],
        width=bar_width,
        label=label1,
        color="tab:blue",
    )
    bars2 = plt.bar(
        x, merged["speedup_2"], width=bar_width, label=label2, color="tab:orange"
    )
    bars3 = plt.bar(
        x + pos_offset,
        merged["speedup_3"],
        width=bar_width,
        label=label3,
        color="tab:green",
    )

    # Add labels and title
    plt.ylabel("IPC Speedup")
    # plt.title(title)
    plt.xticks(x, merged["trace"], rotation=45, ha="right")
    plt.axhline(1.0, color="black", linewidth=0.8, linestyle="--")
    plt.legend()
    plt.grid(axis="y", linestyle="--", alpha=0.7)

    # Add value labels on top of bars
    def add_labels(bars):
        for bar in bars:
            height = bar.get_height()
            plt.text(
                bar.get_x() + bar.get_width() / 2.0,
                height,
                f"{height:.2f}",
                ha="center",
                va="bottom",
                fontsize=6,
                # rotation=10,
            )

    add_labels(bars1)
    add_labels(bars2)
    add_labels(bars3)

    plt.tight_layout()


if __name__ == "__main__":
    # ======= Task 1 =======
    # No prefetchers
    # task_0()

    # # ======= Task 1 =======
    # # Full BW
    # fixed_full_bw = rel_speedup(
    #     with_pref="report/data/task1_1C_fullBW_ghb_pccs_fixed_pd_20b_ip_tag.csv",
    #     no_pref="report/data/task0_1C_fullBW_nopref.csv",
    #     plot=False,
    # )

    # # # ======= Task 2 =======
    # # # Limited BW
    # fixed_limited_bw = rel_speedup(
    #     with_pref="report/data/task2_1C_limitBW_ghb_pccs_fixed_pd.csv",
    #     no_pref="report/data/task2_1C_limitBW_nopref.csv",
    #     plot=False,
    # )

    # # # Full BW vs Limited BW
    # # compare_rel_speedups(df_full=fixed_full_bw, df_limited=fixed_limited_bw)

    # ======= Task 3 =======
    ghb_adaptive_full_bw = rel_speedup(
        with_pref="report/data/task3_1C_fullBW_ghb_pccs_adaptive_pd.csv",
        no_pref="report/data/task0_1C_fullBW_nopref.csv",
        plot=False,
    )

    ghb_adaptive_limited_bw = rel_speedup(
        with_pref="report/data/task3_1C_limitBW_ghb_pccs_adaptive_pd.csv",
        no_pref="report/data/task2_1C_limitBW_nopref.csv",
        plot=False,
    )

    # ======= Task 4 =======
    bop_full_bw = rel_speedup(
        with_pref="report/data/task4_1C_fullBW_bop.csv",
        no_pref="report/data/task0_1C_fullBW_nopref.csv",
        plot=False,
    )

    bop_limited_bw = rel_speedup(
        with_pref="report/data/task4_1C_limitBW_bop.csv",
        no_pref="report/data/task2_1C_limitBW_nopref.csv",
        plot=False,
    )

    # hybrid
    hybrid_full_bw = rel_speedup(
        with_pref="report/data/task4_1C_fullBW.tournament2.csv",
        no_pref="report/data/task0_1C_fullBW_nopref.csv",
        plot=False,
    )

    hybrid_limited_bw = rel_speedup(
        with_pref="report/data/task4_1C_limitBW.tournament2.csv",
        no_pref="report/data/task2_1C_limitBW_nopref.csv",
        plot=False,
    )

    # compare_prefetchers(
    #     ghb_adaptive_full_bw,
    #     bop_full_bw,
    #     hybrid_full_bw,
    #     "GHB Adaptive (Full BW)",
    #     "BOP (Full BW)",
    #     "Hybrid GHB, BOP (Full BW)",
    #     "",
    # )
    # compare_prefetchers(
    #     ghb_adaptive_limited_bw,
    #     bop_limited_bw,
    #     hybrid_limited_bw,
    #     "GHB Adaptive (Limited BW)",
    #     "BOP (Limited BW)",
    #     "Hybrid GHB, BOP (Limited BW)",
    #     "",
    # )

    # ======= Bonus Task =======

    # pythia_full_bw = rel_speedup(
    #     with_pref="report/data/task5_1C_fullBW_pythia.csv",
    #     no_pref="report/data/task0_1C_fullBW_nopref.csv",
    #     plot=False,
    # )

    # pythia_limited_bw = rel_speedup(
    #     with_pref="report/data/task5_1C_limitBW_pythia.csv",
    #     no_pref="report/data/task2_1C_limitBW_nopref.csv",
    #     plot=False,
    # )

    # compare_prefetchers(
    #     ghb_adaptive_full_bw,
    #     hybrid_full_bw,
    #     pythia_full_bw,
    #     "GHB Adaptive (Full BW)",
    #     "BOP (Full BW)",
    #     "Pythia (Full BW)",
    #     "",
    # )
    # compare_prefetchers(
    #     ghb_adaptive_limited_bw,
    #     hybrid_limited_bw,
    #     pythia_limited_bw,
    #     "GHB Adaptive (Limited BW)",
    #     "BOP (Limited BW)",
    #     "Pythia (Limited BW)",
    #     "",
    # )

    plt.show()
