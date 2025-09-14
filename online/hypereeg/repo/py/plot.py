import pandas as pd
import matplotlib.pyplot as plt

# Reload the data
file_path = "./filtered_server.txt"
df=pd.read_csv(file_path, sep="\t", header=None, names=["raw", "filtered"])

# Downsample the data for plotting to speed things up
df_downsampled = df #df.iloc[::10, :]

# Plot the downsampled raw and filtered signals
plt.figure(figsize=(15, 5))
plt.plot(df_downsampled["raw"], label="Raw EEG", alpha=0.6)
plt.plot(df_downsampled["filtered"], label="IIR Filtered EEG", linewidth=1.5)
plt.title("Downsampled Raw vs. IIR Filtered EEG (Server Side)")
plt.xlabel("Sample Index (downsampled by 10)")
plt.ylabel("Amplitude (a.u.)")
plt.legend()
plt.grid(True)
plt.tight_layout()
plt.show()
