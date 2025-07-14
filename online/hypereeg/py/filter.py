from scipy.signal import butter

fs = 1000  # Hz
lowcut = 0.1
highcut = 100
order = 2  # per direction → 4th order zero-phase

nyq = 0.5 * fs
low = lowcut / nyq
high = highcut / nyq

b, a = butter(order, [low, high], btype='band')
print('b =', b)
print('a =', a)
