"""Residual left by ONE float32 multiplication by a constant gain, relative to the scaled signal.
Synthetic signal, not the fixture. Compares with the -148.89 dB loopback residual measured on the USB output."""
import numpy as np

rng = np.random.default_rng(1)
t = np.arange(48000 * 20) / 48000
x = (0.2 * np.sin(2 * np.pi * 220 * t) + 0.1 * np.sin(2 * np.pi * 1234.5 * t)
     + 0.05 * rng.standard_normal(t.size)).astype(np.float32)
for db in (-59.5, -20.0, -6.0):
    y = (x * np.float32(10 ** (db / 20))).astype(np.float32)
    mask = np.abs(x) > 0.05
    gain = np.median(y.astype(np.float64)[mask] / x.astype(np.float64)[mask])
    r = y.astype(np.float64) - x.astype(np.float64) * gain
    rel = 20 * np.log10(np.sqrt(np.mean(r ** 2)) / np.sqrt(np.mean(y.astype(np.float64) ** 2)))
    print(f"gain {db:6.1f} dB: residual re signal {rel:.2f} dB")
