"""Compares a device_probe_v1 loopback capture with the golden render (M2.5.1d).

python analyze_loopback.py <capture.f32> <reference.f32> [output.json]

Both files are interleaved stereo float32 at 48 kHz. The script aligns the capture, estimates a single gain and
looks for what a listener would call a glitch: lost or repeated samples (the alignment slips), runs of digital
silence and short bursts of residual. Other sounds in the system appear as residual too, so a noisy result needs
a silent machine before it means anything. A clean result says nothing about the driver or the physical output.
Needs numpy, which is not a project dependency.
"""
import json
import os
import sys

import numpy as np

RATE = 48000
SEGMENT = 5 * RATE          # alignment checked every 5 s
SEARCH = 2 * RATE           # initial lag search: +-2 s around the start of the signal
WINDOW = RATE // 100        # residual windows of 10 ms
BURST_DB = 20.0             # a window this far above the scaled signal rounding counts as a burst
ROUNDING_DB = -130.0        # residual relative to the local signal expected from float32 rounding of a gain stage
GAP_FRAMES = RATE // 2000   # digital silence of at least 0.5 ms


def lag_of(signal, reference):
    """Lag of signal against reference by FFT cross-correlation (positive: signal starts later)."""
    n = len(signal) + len(reference)
    size = 1 << int(np.ceil(np.log2(n)))
    spectrum = np.fft.rfft(signal, size) * np.conj(np.fft.rfft(reference, size))
    correlation = np.fft.irfft(spectrum, size)
    index = int(np.argmax(np.abs(correlation)))
    return index - size if index > size // 2 else index


def active_region(reference):
    """First frame with signal, and the end of the content."""
    active = np.flatnonzero(np.abs(reference).max(axis=1) > 1e-6)
    return int(active[0]), int(active[-1]) + 1


def zero_runs(mono, minimum):
    """Runs of exact zero of at least minimum frames, as (start, length)."""
    is_zero = np.concatenate(([0], (mono == 0.0).astype(np.int8), [0]))
    edges = np.flatnonzero(np.diff(is_zero))
    runs = zip(edges[::2], edges[1::2] - edges[::2])
    return [(int(start), int(length)) for start, length in runs if length >= minimum]


def analyze(capture, reference):
    first, last = active_region(reference)
    probe_end = min(first + 30 * RATE, last)
    lag = lag_of(capture[: probe_end + SEARCH, 0], reference[: probe_end, 0])
    aligned_start = lag + first
    if aligned_start < 0:
        return {'status': 'INVALID', 'reason': 'capture starts after the signal', 'lag_frames': lag}
    length = min(last - first, len(capture) - aligned_start)
    if length < last - first:
        return {'status': 'INVALID', 'reason': 'capture ends before the signal', 'lag_frames': lag,
                'missing_frames': (last - first) - length}

    captured = capture[aligned_start: aligned_start + length]
    expected = reference[first: last]
    # Median ratio: another sound in the mix must not bias the gain, or the whole residual would rise with it
    loud = np.abs(expected) > 0.01 * np.abs(expected).max()
    gain = float(np.median(captured[loud] / expected[loud]))

    # A lost or repeated sample shifts everything after it: the local lag leaves zero
    slips = []
    for start in range(0, length - SEGMENT, SEGMENT):
        local = lag_of(captured[start: start + SEGMENT, 0], expected[start: start + SEGMENT, 0])
        if local != 0:
            slips.append({'second': round((start + first) / RATE, 2), 'lag_frames': local})

    residual = captured - gain * expected
    windows = length // WINDOW
    energy = (residual[: windows * WINDOW] ** 2).sum(axis=1).reshape(windows, WINDOW).mean(axis=1)
    # All levels are relative to the captured signal, so that the result does not depend on the volume
    scaled = (gain * expected[: windows * WINDOW]) ** 2
    signal_energy = scaled.sum(axis=1).reshape(windows, WINDOW).mean(axis=1)
    power = float(np.mean(signal_energy))
    noise_floor = max(float(np.median(energy)), power * 10 ** (ROUNDING_DB / 10))
    bursts = np.flatnonzero(energy > noise_floor * 10 ** (BURST_DB / 10))
    gaps = zero_runs(np.abs(captured).max(axis=1), GAP_FRAMES)
    silent_expected = zero_runs(np.abs(expected).max(axis=1), GAP_FRAMES)
    unexpected = [g for g in gaps if not any(s <= g[0] and g[0] + g[1] <= s + l for s, l in silent_expected)]

    return {
        'status': 'MEASURED',
        'lag_frames': lag,
        'compared_frames': length,
        'gain': round(gain, 6),
        'gain_db': round(20 * float(np.log10(abs(gain))), 3) if gain != 0 else None,
        'exact_zero_residual': bool(energy.max() == 0.0),
        'residual_db': round(10 * float(np.log10(max(float(np.mean(energy)), 1e-300) / power)), 2),
        'max_window_residual_db': round(10 * float(np.log10(max(float(energy.max()), 1e-300) / power)), 2),
        'alignment_slips': slips,
        'residual_bursts': int(len(bursts)),
        'burst_seconds': [round((int(w) * WINDOW + first) / RATE, 2) for w in bursts[:20]],
        'unexpected_silences': len(unexpected),
        'silence_seconds': [round((s + first) / RATE, 3) for s, _ in unexpected[:20]],
    }


def load(path):
    """Interleaved stereo float32; raises ValueError for any other shape."""
    data = np.fromfile(path, dtype='<f4')
    if data.size == 0 or data.size % 2 != 0 or (data.size * 4 != os.path.getsize(path)):
        raise ValueError(f'{path} is not interleaved stereo float32')
    return data.reshape(-1, 2).astype(np.float64)


if __name__ == '__main__':
    if len(sys.argv) < 3:
        print(__doc__)
        sys.exit(2)
    try:
        report = analyze(load(sys.argv[1]), load(sys.argv[2]))
    except (OSError, ValueError) as error:
        report = {'status': 'INVALID', 'reason': str(error)}
    print(json.dumps(report, indent=2))
    if len(sys.argv) > 3:
        with open(sys.argv[3], 'w', encoding='utf-8') as f:
            json.dump(report, f, indent=2)
    sys.exit(0 if report['status'] == 'MEASURED' else 1)
