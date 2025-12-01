import pandas as pd
import glob

csv_files = glob.glob("report/data/*.csv")

for csv_file in csv_files:
    df = pd.read_csv(csv_file)
    df = df.sort_values(['trace']) # Sort by column: 'trace' (ascending)

    fname = csv_file.split('/', 2)[2]
    df.to_csv(f"report/sorted/{fname}", index=False)