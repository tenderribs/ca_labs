import pandas as pd
import matplotlib.pyplot as plt
import seaborn as sns
import os

# Create plots directory if it doesn't exist
output_dir = "report/plots"
if not os.path.exists(output_dir):
    os.makedirs(output_dir)

def plot_metric(csv_file, metric_col, title, y_label, output_filename):
    # Load data
    try:
        df = pd.read_csv(csv_file)
    except FileNotFoundError:
        print(f"Error: Could not find {csv_file}")
        return

    # Create a 'Workload' column for easier plotting labels
    # Format: "3L-1H"
    df['Workload'] = df['num_l'].astype(str) + "L-" + df['num_h'].astype(str) + "H"

    # Set plot style
    sns.set_theme(style="whitegrid")

    # Initialize the figure
    plt.figure(figsize=(10, 6))

    # Create the Bar Plot
    # hue='scheduler' creates the grouping by scheduler
    chart = sns.barplot(
        data=df,
        x='Workload',
        y=metric_col,
        hue='scheduler',
        palette='viridis',
        edgecolor='black'
    )

    # Customization
    plt.title(title, fontsize=16, fontweight='bold', pad=20)
    plt.xlabel("Workload Configuration (Low-High Intensity)", fontsize=12)
    plt.ylabel(y_label, fontsize=12)
    plt.legend(title='Scheduler', title_fontsize='12')

    # Add value labels on top of bars
    for container in chart.containers:
        chart.bar_label(container, fmt='%.2f', padding=3, fontsize=9)

    # Save plot
    output_path = os.path.join(output_dir, output_filename)
    plt.tight_layout()
    plt.savefig(output_path, dpi=300)
    print(f"Generated plot: {output_path}")
    plt.close()

if __name__ == "__main__":
    # 1. Plot Instruction Throughput
    plot_metric(
        csv_file="report/res_inst_thrput.csv",
        metric_col="inst_thrput",
        title="System Instruction Throughput",
        y_label="Aggregate Instructions / Cycle",
        output_filename="inst_throughput.png"
    )

    # 2. Plot Maximum Slowdown
    plot_metric(
        csv_file="report/res_max_slowdowns.csv",
        metric_col="max_slowdown",
        title="Maximum Slowdown (Unfairness)",
        y_label="Max Slowdown",
        output_filename="max_slowdown.png"
    )