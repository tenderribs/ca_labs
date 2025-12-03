import pandas as pd


def load_data(path: str) -> pd.DataFrame:
    df = pd.read_csv(path)  # C.fullBW.nopref.json
    df = df.sort_values(by="trace")

    # Clean trace names
    df["trace"] = df["trace"].str.replace(".trace.gz", "", regex=False)
    df["trace"] = df["trace"].str.replace(".champsim.gz", "", regex=False)
    df["trace"] = df["trace"].str.replace(r"_0+(\d+)", r"_\1", regex=True)

    return df


df20 = load_data("report/data/task1_1C_fullBW_ghb_pccs_fixed_pd_20b_ip_tag.csv")
df56 = load_data("report/data/task1_1C_fullBW_ghb_pccs_fixed_pd_56b_ip_tag.csv")


print(df20.round(3))
print(df56.round(3))
