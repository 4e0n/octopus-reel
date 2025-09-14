import numpy as np
import matplotlib.pyplot as plt

# Load the data (assumes 2 columns: raw and filtered)
data = np.loadtxt("filtered_server.txt")

raw = data[:, 0]
filtered = data[:, 1]

# Create time axis (optional, for labeling)
t = np.arange(len(raw))

# Plot both signals
plt.figure(figsize=(12, 6))
plt.plot(t, raw, label="Raw signal", color="blue", alpha=0.7)
plt.plot(t, filtered, label="Filtered signal", color="orange", alpha=0.9)
plt.xlabel("Sample index")
plt.ylabel("Amplitude (V)")
plt.title("Raw vs. IIR-Filtered EEG Signal")
plt.legend()
plt.grid(True)
plt.tight_layout()
plt.show()
