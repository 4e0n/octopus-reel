from scipy.signal import iirnotch

fs=1000 # sampling frequency in Hz
f0=50.0 # notch frequency
Q=30.0  # Quality factor; adjust for width (30–50 is typical)

b,a=iirnotch(f0,Q,fs)
print("b=",b)
print("a=",a)
