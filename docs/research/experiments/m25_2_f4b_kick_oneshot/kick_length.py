"""F4b: kick length and low-frequency share of the whole-piece Pulso stem (stem_export_v2, whole_pulso.wav).
Kick length: from each planned attack to the last sample above -40 dB of the note's peak, within 200 ms or the next
attack. Low-frequency share: Welch power below 100 Hz over the total.
  tools/pyenv/.venv/Scripts/python.exe -I <this file> artifacts/cacp/stem_export_v2/whole_pulso.wav \
      docs/research/experiments/cacp_methods/fixture_intents_seed1337.csv <output.json>
"""
import csv
import json
import sys
from pathlib import Path

import numpy as np
from scipy import signal

FS = 48000
raw = Path(sys.argv[1]).read_bytes()
assert raw[:4] == b'RIFF' and int.from_bytes(raw[20:22], 'little') == 3
x = np.frombuffer(raw[44:], dtype='<f4').astype(np.float64)
mono = (x[0::2] + x[1::2]) / 2
onsets = sorted(int(r['sample_frame']) for r in csv.DictReader(open(sys.argv[2])) if r['role'] == '0')
lengths = []
for i, on in enumerate(onsets):
    end = min(on + FS // 5, onsets[i + 1] if i + 1 < len(onsets) else len(mono), len(mono))
    seg = np.abs(mono[on:end])
    if seg.size == 0 or seg.max() <= 0:
        continue
    above = np.nonzero(seg >= seg.max() * 0.01)[0]
    lengths.append(1000.0 * (above[-1] + 1) / FS)
f, p = signal.welch(mono, FS, window='hann', nperseg=8192)
out = {'kicks': len(lengths), 'length_ms_median': float(np.median(lengths)),
       'length_ms_p05': float(np.percentile(lengths, 5)), 'length_ms_p95': float(np.percentile(lengths, 95)),
       'low_share_below_100hz': float(p[f < 100].sum() / p.sum())}
Path(sys.argv[3]).write_text(json.dumps(out, indent=2), encoding='utf-8')
print(out)
