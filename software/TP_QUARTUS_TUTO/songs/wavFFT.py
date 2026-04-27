import numpy as np
import pandas as pd
from scipy.io import wavfile
from pathlib import Path

input_wav = "smb_mariodie.wav"
output_csv = "output/dominant_frequency_40hz.csv"

sr, data = wavfile.read(input_wav)

if data.ndim > 1:
    data = data.mean(axis=1)

data = data.astype(np.float64)
data = data - np.mean(data)

window_sec = 0.025
window_size = int(round(sr * window_sec))
step_size = window_size

rows = []

for start in range(0, len(data) - window_size + 1, step_size):
    chunk = data[start:start + window_size]
    chunk = chunk * np.hanning(len(chunk))

    spectrum = np.fft.rfft(chunk)
    freqs = np.fft.rfftfreq(len(chunk), d=1.0 / sr)
    mag = np.abs(spectrum)

    if len(mag) > 1:
        idx = np.argmax(mag[1:]) + 1
        dom_freq = float(freqs[idx])
        dom_mag = float(mag[idx])
    else:
        dom_freq = 0.0
        dom_mag = float(mag[0]) if len(mag) else 0.0

    rows.append({
        "dominant_freq_hz": round(dom_freq, 6)
    })

df = pd.DataFrame(rows)
Path(output_csv).parent.mkdir(parents=True, exist_ok=True)
df.to_csv(output_csv, index=False)

print(f"Saved {len(df)} rows to {output_csv}")