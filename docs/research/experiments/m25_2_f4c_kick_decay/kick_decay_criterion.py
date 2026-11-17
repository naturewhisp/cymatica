"""F4c criterion (docs/research/M2_5_2_F4c_Kick_Decay_Preregistration.md): overlap N, kick length and low share from
the whole Pulso stem; Corpo masking loss C with MoSQITo on the excerpts; safety S from the scenario manifest.
  tools/pyenv/.venv/Scripts/python.exe -I <this file> artifacts/cacp/kick_decay_ab_v1 artifacts/cacp/kick_decay_ab_v1_manifest.json \
      docs/research/experiments/cacp_methods/fixture_intents_seed1337.csv <output.json>
"""
import csv
import json
import sys
from pathlib import Path

import numpy as np
from mosqito.sq_metrics import loudness_zwtv
from scipy import signal

FS = 48000
FULL_SCALE_PA = 2e-5 * 10 ** (100 / 20) * np.sqrt(2)
KS = ['k100', 'k125', 'k150', 'k175', 'k200']


def read(p):
    raw = Path(p).read_bytes()
    assert raw[:4] == b'RIFF' and int.from_bytes(raw[20:22], 'little') == 3
    x = np.frombuffer(raw[44:], dtype='<f4').astype(np.float64)
    return (x[0::2] + x[1::2]) / 2


def ntv(p):
    return float(np.mean(loudness_zwtv(read(p) * FULL_SCALE_PA, FS, field_type='free')[0]))


d = Path(sys.argv[1])
man = json.load(open(sys.argv[2]))['per_k']
onsets = sorted(int(r['sample_frame']) for r in csv.DictReader(open(sys.argv[3])) if r['role'] == '0')
out = {}
for k in KS:
    x = read(d / f'whole_pulso_{k}.wav')
    overlaps, lengths = 0, []
    for i, on in enumerate(onsets):
        nxt = onsets[i + 1] if i + 1 < len(onsets) else len(x)
        peak = np.abs(x[on:nxt]).max()
        if i + 1 < len(onsets) and np.abs(x[nxt - 48:nxt]).max() > 1e-3 * peak:
            overlaps += 1
        seg = np.abs(x[on:min(nxt, on + FS // 2)])
        above = np.nonzero(seg >= seg.max() * 0.01)[0]
        lengths.append(1000.0 * (above[-1] + 1) / FS)
    f, p = signal.welch(x, FS, window='hann', nperseg=8192)
    corpo_loss = {}
    for s in ['A', 'B', 'Aprime']:
        n_mix = ntv(d / f'{s}_{k}_mix.wav')
        n_without = ntv(d / f'{s}_{k}_mix_without_corpo.wav')
        n_corpo = ntv(d / f'{s}_{k}_corpo.wav')
        corpo_loss[s] = 1 - (n_mix - n_without) / n_corpo
    out[k] = {'overlaps': overlaps, 'length_ms_median': float(np.median(lengths)), 'low_share_below_100hz': float(p[f < 100].sum() / p.sum()),
              'corpo_loss': corpo_loss, 'mix_lufs_rise': man[k]['mix_integrated_lufs'] - man['k100']['mix_integrated_lufs'],
              'pulso_lufs_rise': man[k]['pulso_integrated_lufs'] - man['k100']['pulso_integrated_lufs'],
              'limiter_db': man[k]['max_gain_reduction_db'], 'pulso_steals': man[k]['pulso_steals_or_cuts']}
for k in KS:
    v = out[k]
    v['N'] = v['overlaps'] == 0
    v['C'] = all(v['corpo_loss'][s] - out['k100']['corpo_loss'][s] <= 0.02 for s in v['corpo_loss'])
    v['S'] = v['limiter_db'] == 0 and v['mix_lufs_rise'] <= 0.5 and v['pulso_steals'] == 0
passing = [k for k in KS if out[k]['N'] and out[k]['C'] and out[k]['S']]
out['choice'] = passing[-1] if passing else None
Path(sys.argv[4]).write_text(json.dumps(out, indent=2), encoding='utf-8')
for k in KS:
    v = out[k]
    print(k, 'overlaps', v['overlaps'], 'len', round(v['length_ms_median'], 1), 'low', round(v['low_share_below_100hz'], 4),
          'corpo loss', {s: round(x, 3) for s, x in v['corpo_loss'].items()}, 'mix +%.3f' % v['mix_lufs_rise'],
          'pulso +%.3f' % v['pulso_lufs_rise'], 'N', v['N'], 'C', v['C'], 'S', v['S'])
print('choice:', out['choice'])
