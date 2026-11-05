"""Bit-level comparison of a loopback capture with the reference, after alignment.

python compare_loopback_bits.py <capture.f32> <reference.f32> <lag_frames>

lag_frames comes from analyze_loopback.py. The active part of the reference is compared as raw float32 bits.
"""
import sys

import numpy as np

capture = np.fromfile(sys.argv[1], dtype='<f4').reshape(-1, 2)
reference = np.fromfile(sys.argv[2], dtype='<f4').reshape(-1, 2)
lag = int(sys.argv[3])

active = np.flatnonzero(np.abs(reference).max(axis=1) > 1e-6)
first, last = int(active[0]), int(active[-1]) + 1
start = lag + first
length = min(last - first, len(capture) - start)
captured = capture[start: start + length]
expected = reference[first: first + length]
differing = int((captured.view(np.uint32) != expected.view(np.uint32)).sum())
print(f'capture frames {len(capture)}, reference frames {len(reference)}, active {first}..{last}')
print(f'compared frames {length}, differing samples {differing}, of {length * 2}')
print(f'peak captured {float(np.abs(captured).max()):.6f}, peak expected {float(np.abs(expected).max()):.6f}')
if differing:
    difference = np.abs(captured.astype(np.float64) - expected.astype(np.float64))
    print(f'max abs difference {float(difference.max()):.3e}')
sys.exit(0)
