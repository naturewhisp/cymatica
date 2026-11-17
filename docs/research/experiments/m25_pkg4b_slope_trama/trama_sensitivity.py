"""Package 4b, section 2: Trama loudness increment and masking loss at Trama bus offsets 0, +3, +6, +9 dB
(MoSQITo 1.2.1 ISO 532-1 time-varying, free field; calibration and mono as in package 4).
  tools/pyenv/.venv/Scripts/python.exe -I <this file> artifacts/cacp/stem_export_v2 <output.json>
"""
import json
import sys
from pathlib import Path

import numpy as np
from mosqito.sq_metrics import loudness_zwtv

FS = 48000
FULL_SCALE_PA = 2e-5 * 10 ** (100 / 20) * np.sqrt(2)


def read_mono(p):
    raw = Path(p).read_bytes()
    assert raw[:4] == b'RIFF' and int.from_bytes(raw[20:22], 'little') == 3
    x = np.frombuffer(raw[44:], dtype='<f4').astype(np.float64)
    return (x[0::2] + x[1::2]) / 2 * FULL_SCALE_PA


def ntv(x):
    return float(np.mean(loudness_zwtv(x, FS, field_type='free')[0]))


d = Path(sys.argv[1])
out = {}
for s in ['A', 'B', 'Aprime']:
    n_without = ntv(read_mono(d / f'{s}_mix_without_trama.wav'))
    rows = []
    for off in (0, 3, 6, 9):
        n_mix = ntv(read_mono(d / f'{s}_t{off}_mix.wav'))
        n_alone = ntv(read_mono(d / f'{s}_t{off}_trama.wav'))
        inc = n_mix - n_without
        rows.append({'offset_db': off, 'mix_sone': n_mix, 'trama_alone_sone': n_alone, 'increment_sone': inc,
                     'masking_loss': 1 - inc / n_alone if n_alone > 0 else None})
    incs = [r['increment_sone'] for r in rows]
    losses = [r['masking_loss'] for r in rows]
    out[s] = {'without_trama_sone': n_without, 'rows': rows,
              'K2_increment_strictly_increasing': all(b > a for a, b in zip(incs, incs[1:])),
              'K3_loss_strictly_decreasing': all(b < a for a, b in zip(losses, losses[1:])),
              'increment_per_db_0_to_9': (incs[-1] - incs[0]) / 9.0}
Path(sys.argv[2]).write_text(json.dumps(out, indent=2), encoding='utf-8')
for s, v in out.items():
    print(s, 'without', round(v['without_trama_sone'], 2), 'K2', v['K2_increment_strictly_increasing'], 'K3', v['K3_loss_strictly_decreasing'],
          'dN/dB', round(v['increment_per_db_0_to_9'], 3))
    for r in v['rows']:
        print('   ', r['offset_db'], 'alone', round(r['trama_alone_sone'], 2), 'inc', round(r['increment_sone'], 3), 'loss', round(r['masking_loss'], 3))
