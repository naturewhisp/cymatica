"""Controls of analyze_loopback.py on synthetic signals: each fault must be seen, a clean signal must not raise any."""
import os
import subprocess
import sys
import tempfile

import numpy as np

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from analyze_loopback import RATE, analyze  # noqa: E402

rng = np.random.default_rng(1337)
seconds = 40
t = np.arange(seconds * RATE) / RATE
envelope = 0.5 + 0.5 * np.sign(np.sin(2 * np.pi * 2 * t))
left = 0.2 * envelope * np.sin(2 * np.pi * 220 * t) + 0.05 * rng.standard_normal(len(t))
right = 0.2 * envelope * np.sin(2 * np.pi * 331 * t) + 0.05 * rng.standard_normal(len(t))
reference = np.column_stack((left, right))
lead = np.zeros((3 * RATE // 2, 2))


def base():
    return np.vstack((lead, 0.7 * reference, np.zeros((RATE, 2))))


results = []


def check(name, condition):
    results.append(bool(condition))
    print(f"{'ok  ' if condition else 'FAIL'} {name}")


clean = analyze(base(), reference)
check('clean: measured', clean['status'] == 'MEASURED')
check('clean: lag found', clean['lag_frames'] == len(lead))
check('clean: gain found', abs(clean['gain'] - 0.7) < 1e-6)
check('clean: no fault', not clean['alignment_slips'] and clean['residual_bursts'] == 0 and clean['unexpected_silences'] == 0)

silent = base()
silent[len(lead) + 10 * RATE: len(lead) + 10 * RATE + 240] = 0.0
report = analyze(silent, reference)
check('5 ms of silence is found', report['unexpected_silences'] == 1)
check('5 ms of silence shows as a burst', report['residual_bursts'] >= 1)

lost = np.delete(base(), range(len(lead) + 20 * RATE, len(lead) + 20 * RATE + 48), axis=0)
report = analyze(lost, reference)
check('48 lost frames slip the alignment', len(report.get('alignment_slips', [])) >= 1)

click = base()
click[len(lead) + 15 * RATE] += 0.9
report = analyze(click, reference)
check('single sample click shows as a burst', report['residual_bursts'] >= 1)

# The levels are relative to the signal: the same faults must be seen at a very low endpoint volume
quiet = (base() * 0.0015).astype(np.float32).astype(np.float64)
report = analyze(quiet, reference)
check('low volume: no false fault', not report['alignment_slips'] and report['residual_bursts'] == 0)
check('low volume: rounding near float32 precision', report['residual_db'] < -120.0)
quiet[len(lead) + 15 * RATE] += 0.001
check('low volume: click still found', analyze(quiet, reference)['residual_bursts'] >= 1)

short = base()[: len(lead) + 20 * RATE]
check('truncated capture is INVALID', analyze(short, reference)['status'] == 'INVALID')

# Files: a capture of one frame, or one that is not stereo float32, never yields MEASURED
script = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'analyze_loopback.py')
with tempfile.TemporaryDirectory() as folder:
    good = os.path.join(folder, 'reference.f32')
    reference.astype('<f4').tofile(good)
    one_frame = os.path.join(folder, 'one_frame.f32')
    np.zeros(2, dtype='<f4').tofile(one_frame)
    odd = os.path.join(folder, 'odd.f32')
    with open(odd, 'wb') as f:
        f.write(b'\0' * 7)
    outcome = lambda path: subprocess.run([sys.executable, script, path, good], capture_output=True).returncode
    check('one frame capture fails', outcome(one_frame) == 1)
    check('file of the wrong shape fails', outcome(odd) == 1)
    check('missing file fails', outcome(os.path.join(folder, 'none.f32')) == 1)

print(f'{sum(results)}/{len(results)} checks passed')
sys.exit(0 if all(results) else 1)
