import numpy as np
import matplotlib.pyplot as plt
from scipy.signal import iirnotch, freqz

# Filter parameters
f0 = 50.0  # Notch frequency in Hz
Q = 30.0   # Quality factor
fs = 1000  # Sampling frequency in Hz

# Design notch filter
b, a = iirnotch(f0, Q, fs)

# Frequency response
w, h = freqz(b, a, fs=fs)
magnitude_db = 20 * np.log10(np.abs(h))
phase_deg = np.angle(h, deg=True)

# Limit frequency range for plots (0 to 100 Hz)
mask = w <= 100

# Plot magnitude response
plt.figure()
plt.plot(w[mask], magnitude_db[mask])
plt.title('Magnitude Response of Notch Filter (0–100 Hz)')
plt.xlabel('Frequency (Hz)')
plt.ylabel('Magnitude (dB)')
plt.grid(True)
plt.tight_layout()
plt.show()

# Plot phase response
plt.figure()
plt.plot(w[mask], phase_deg[mask])
plt.title('Phase Response of Notch Filter (0–100 Hz)')
plt.xlabel('Frequency (Hz)')
plt.ylabel('Phase (degrees)')
plt.grid(True)
plt.tight_layout()
plt.show()
