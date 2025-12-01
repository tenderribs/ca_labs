import pandas as pd
import glob

csv_files = glob.glob("*.csv")

for csv_file in csv_files:
    df = pd.read_csv(csv_file)
    df = df.sort_values(['trace']) # Sort by column: 'trace' (ascending)

    # fname = csv_file.split('/', 2)[2]
    df.to_csv(f"sorted_{csv_file}", index=False)