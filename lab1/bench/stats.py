import pandas as pd
import matplotlib.pyplot as plt

# Set matplotlib to use CMU fonts (Computer Modern, LaTeX default)
plt.rcParams["font.family"] = "serif"
plt.rcParams["font.size"] = 10

csv_path = "bench/inputs_custom_*.x.csv"

config_cols = [
    "policy",
    "icache_cap",
    "icache_ways",
    "icache_blsz",
    "dcache_cap",
    "dcache_ways",
    "dcache_blsz",
]

# Create a nice name mapping for tests
test_names = {
    "inputs/custom/looped_random_1k.x": "Looped Random 1.5K",
    "inputs/custom/pointer_chase.x": "Pointer Chase",
    "inputs/custom/primes.x": "Primes (Sieve)",
    "inputs/custom/random1.x": "Random1",
    "inputs/custom/repmovs.x": "Repmovs",
    "inputs/custom/strided_access.x": "Strided Access",
    "inputs/custom/test1.x": "Sequential-8",
    "inputs/custom/stream_reuse.x": "Stream-Reuse",
    "inputs/custom/levenshtein.x": "Levenshtein",
    "inputs/custom/matmul.x": "Mat-Mul",
}

colors = [
    "#D1322F",
    "#F17901",
    "#E0D018",
    "#67C71D",
    "#0D8825",
    "#1DB6C7",
    "#1D4AC7",
    "#6C1DC7",
    "#AB1DC7",
    "#C71D83",
]  # Different colors for each test


def find_best_for_test(df, test_name):
    """Find the best IPC configurations for a specific test"""
    # Filter rows based on column: 'input'
    filtered_df = df[
        df["input"].str.contains(test_name, regex=False, na=False, case=False)
    ]

    if filtered_df.empty:
        print(f"No data found for test: {test_name}")
        return None

    # Sort by columns: 'ipc' (descending), 'icache_cap' (ascending), 'dcache_cap' (ascending)
    sorted_df = filtered_df.sort_values(
        ["cycles", "icache_cap", "dcache_cap"], ascending=[True, True, True]
    )

    # Get the least cycles value
    least_cycles = sorted_df["cycles"].iloc[0]

    # Get all configurations with least cycles
    best_configs = sorted_df[sorted_df["cycles"] == least_cycles]

    # print(f"\n=== Best configurations for {test_name} (IPC = {least_cycles}) ===")
    # print(best_configs.head())
    return best_configs


def find_common_best_configs(df, input_tests):
    """Find configurations that work well across all tests"""
    print(f"Checking {df.shape} configurations")
    # Get the best configurations for each test
    best_configs_by_test = {}
    for test in input_tests:
        best_configs = find_best_for_test(df, test)
        if best_configs is not None:
            best_configs_by_test[test] = best_configs

    # Start with the best configs from the first test
    first_test = list(best_configs_by_test.keys())[0]
    common_configs = best_configs_by_test[first_test][config_cols].drop_duplicates()

    # Merge with best configs from each subsequent test
    for test, configs in best_configs_by_test.items():
        common_configs = pd.merge(
            common_configs,
            configs[config_cols].drop_duplicates(),
            how="inner",
            on=config_cols,
        )

    return common_configs


# find the parameter set that achieves the max score on all benchmarks
def best_overall():
    input_tests = [
        "pointer_chase",
        "strided_access",
        "primes",
        "random1",
        "repmovs",
        "test1",
    ]

    df = pd.read_csv(csv_path)

    # best configs are those that got the best IPC score on all tests
    best = find_common_best_configs(df, input_tests)

    # find smallest caches that permit achieved best score
    # sort by what is most interesting
    best = best.sort_values(
        ["icache_cap", "dcache_cap", "dcache_blsz", "policy"],
        ascending=[True, True, False, False],
    )

    best.to_csv("bench/best_overall.csv", index=False)


# Effect of Cache Size on Performance (LRU, 32B blocks)
def analyze_cache_size_effect():
    print("\n\n\n============ analyze_cache_size_effect ============")
    df = pd.read_csv(csv_path)

    # Filter for baseline configuration
    df = df[
        (df["policy"] == "lru")
        & (df["icache_ways"] == 4)
        & (df["dcache_ways"] == 8)
        & (df["icache_blsz"] == 32)
        & (df["dcache_blsz"] == 32)
    ]

    # here everything has same results, so just show random25k
    icache_test = "inputs/custom/looped_random_1k.x"

    # tests with interesting results, all others are equal
    # dcache_tests = [
    #     "inputs/custom/stream_reuse.x",
    #     "inputs/custom/strided_access.x",
    #     "inputs/custom/primes.x",
    #     "inputs/custom/pointer_chase.x",
    # ]

    dcache_tests = df["input"].unique()

    # Create figure with two subplots
    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(14, 5))

    # ===== Plot 1: Instruction Cache Size Effect =====
    test_data = df[(df["input"] == icache_test) & (df["dcache_cap"] == 64 * 1024)]
    test_data = test_data.sort_values(by=["icache_cap"])

    # Convert to KB for better readability
    test_data_plot = test_data.copy()
    test_data_plot["icache_kb"] = test_data_plot["icache_cap"] / 1024

    ax1.plot(
        test_data_plot["icache_kb"],
        test_data_plot["ipc"],
        marker="o",
        linewidth=2,
        markersize=8,
        color="#2E86AB",
    )
    ax1.set_xlabel("Instruction Cache Size (KB)", fontsize=11)
    ax1.set_ylabel("IPC", fontsize=11)
    ax1.set_title(
        "Instruction Cache Size Effect\n(Looped Random 1.5K, 64KB D-Cache)", fontsize=11
    )
    ax1.grid(True, alpha=0.3, linestyle="--")
    ax1.set_xscale("log", base=2)

    # Set custom x-axis ticks at actual data points
    ax1.set_xticks(test_data_plot["icache_kb"])
    ax1.set_xticklabels([f"{int(x)}" for x in test_data_plot["icache_kb"]])

    # Print data for reference
    print(f"\nInstruction Cache Size Effect - {icache_test}")
    print(test_data[["icache_cap", "ipc", "cycles"]].to_string(index=False))

    # ===== Plot 2: Data Cache Size Effect =====

    # Get all unique dcache sizes across all tests for consistent x-axis
    all_dcache_sizes = set()

    for idx, test in enumerate(dcache_tests):
        if test == "inputs/custom/looped_random_1k.x":
            continue
        # with 2KB icache
        test_data = df[(df["input"] == test) & (df["icache_cap"] == 2 * 1024)]
        test_data = test_data.sort_values(by=["dcache_cap"])

        # Convert to KB for better readability
        test_data_plot = test_data.copy()
        test_data_plot["dcache_kb"] = test_data_plot["dcache_cap"] / 1024

        all_dcache_sizes.update(test_data_plot["dcache_kb"])

        ax2.plot(
            test_data_plot["dcache_kb"],
            test_data_plot["ipc"],
            marker="o",
            linewidth=2,
            markersize=8,
            label=test_names[test],
            color=colors[idx],
        )

        # Print data for reference
        print(f"\nData Cache Size Effect - {test}")
        print(test_data[["dcache_cap", "ipc", "cycles"]].to_string(index=False))

    ax2.set_xlabel("Data Cache Size (KB)", fontsize=11)
    ax2.set_ylabel("IPC", fontsize=11)
    ax2.set_title("Data Cache Size Effect\n(2KB I-Cache)", fontsize=11)
    ax2.legend(fontsize=10, loc="best")
    ax2.grid(True, alpha=0.3, linestyle="--")
    ax2.set_xscale("log", base=2)

    # Set custom x-axis ticks at actual data points
    sorted_sizes = sorted(all_dcache_sizes)
    ax2.set_xticks(sorted_sizes)
    ax2.set_xticklabels([f"{int(x)}" for x in sorted_sizes])

    plt.tight_layout()
    plt.show()


# Effect of Block Size on Performance (LRU, 32B blocks)
def analyze_block_size_effect():
    print("\n\n\n============ analyze_block_size_effect ============")
    df = df = pd.read_csv(csv_path)

    # Filter for baseline configuration
    df = df[
        (df["policy"] == "lru")
        & (df["icache_ways"] == 4)
        & (df["dcache_ways"] == 8)
        & (df["icache_cap"] == 2 * 1024)
        & (df["dcache_cap"] == 64 * 1024)
    ]

    # all tests are interesting here
    tests = df["input"].unique()

    # Create figure
    fig, ax = plt.subplots(figsize=(10, 6))
    # fig.suptitle(
    #     "Effect of Block Size on Performance (LRU, 2KB I-Cache 4-way, 64KB D-Cache 8-way)",
    #     fontsize=14,
    # )

    # Get all unique block sizes for consistent x-axis
    all_block_sizes = set()

    for idx, test in enumerate(tests):
        test_data = df[df["input"] == test]
        test_data = test_data.sort_values(by=["icache_blsz"])

        all_block_sizes.update(test_data["icache_blsz"])

        ax.plot(
            test_data["icache_blsz"],
            test_data["ipc"],
            marker="o",
            linewidth=2,
            markersize=8,
            label=test_names[test],
            color=colors[idx],
        )

        # Print data for reference
        print(f"\nBlock Size Effect - {test}")
        print(
            test_data[["icache_blsz", "dcache_blsz", "ipc", "cycles"]].to_string(
                index=False
            )
        )

    ax.set_xlabel("Block Size (bytes)", fontsize=11)
    ax.set_ylabel("IPC", fontsize=11)
    ax.set_title("Block Size Effect", fontsize=11)
    ax.legend(fontsize=10, loc="best")
    ax.grid(True, alpha=0.3, linestyle="--")
    ax.set_xscale("log", base=2)

    # Set custom x-axis ticks at actual data points
    sorted_sizes = sorted(all_block_sizes)
    ax.set_xticks(sorted_sizes)
    ax.set_xticklabels([f"{int(x)}" for x in sorted_sizes])

    plt.tight_layout()
    plt.show()


# Effect of Block Size on Performance (LRU, 32B blocks)
def analyze_associativity_effect():
    print("\n\n\n============ analyze_associativity_effect ============")
    df = df = pd.read_csv(csv_path)

    # Filter for baseline configuration
    df = df[
        (df["policy"] == "lru")
        & (df["icache_ways"] == 4)
        & (df["icache_blsz"] == 32)
        & (df["dcache_blsz"] == 32)
        & (df["icache_cap"] == 2 * 1024)
        & (df["dcache_cap"] == 64 * 1024)
    ]

    # all tests are interesting here
    tests = df["input"].unique()
    # here actually look at
    # tests = ["inputs/custom/strided_access.x", "inputs/custom/stream_reuse.x"]

    for idx, test in enumerate(tests):
        test_data = df[(df["input"] == test)]
        print(f"\ninst cache {test}")
        print(test_data)


def analyze_policy_effect():
    print("\n\n\n============ analyze_policy_effect ============")
    df = df = pd.read_csv(csv_path)

    # Filter for baseline configuration
    df = df[
        (df["icache_blsz"] == 32)
        & (df["icache_ways"] == 4)
        & (df["dcache_ways"] == 8)
        & (df["icache_cap"] == 2 * 1024)
        & (df["dcache_cap"] == 64 * 1024)
    ]

    # all tests are interesting here
    # tests = ["inputs/custom/strided_access.x", "inputs/custom/stream_reuse.x"]
    tests = df["input"].unique()
    for idx, test in enumerate(tests):
        test_data = df[(df["input"] == test)]
        print(f"\ninst cache {test}")
        print(test_data)


if __name__ == "__main__":
    # best_overall()
    # analyze_cache_size_effect()
    analyze_block_size_effect()
    # analyze_associativity_effect()
    # analyze_policy_effect()
