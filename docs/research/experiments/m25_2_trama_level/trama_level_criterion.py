"""Trama level criterion (docs/research/M2_5_2_Trama_Level_Preregistration.md): loudness increment and masking loss of
Trama, Corpo and Vettore at Trama bus offsets 0..+8 dB (MoSQITo 1.2.1 ISO 532-1 time-varying, free field; calibration
and mono as in package 4); presence, subordination and safety; choice of the smallest offset satisfying all three in
every section.
  tools/pyenv/.venv/Scripts/python.exe -I <this file> artifacts/cacp/stem_export_v3 artifacts/cacp/stem_export_v3_manifest.json <output.json>
"""
import json
import sys
from pathlib import Path

import numpy as np
from mosqito.sq_metrics import loudness_zwtv

FS = 48000
FULL_SCALE_PA = 2e-5 * 10 ** (100 / 20) * np.sqrt(2)
OFFSETS = [0, 2, 4, 6, 8]
SECTIONS = ['A', 'B', 'Aprime']


def ntv(p):
    raw = Path(p).read_bytes()
    assert raw[:4] == b'RIFF' and int.from_bytes(raw[20:22], 'little') == 3
    x = np.frombuffer(raw[44:], dtype='<f4').astype(np.float64)
    return float(np.mean(loudness_zwtv((x[0::2] + x[1::2]) / 2 * FULL_SCALE_PA, FS, field_type='free')[0]))


d = Path(sys.argv[1])
lufs = json.load(open(sys.argv[2]))['mix_excerpt_integrated_lufs']
out = {'sections': {}, 'choice_db': None}
ok_by_offset = {g: True for g in OFFSETS}
for s in SECTIONS:
    n_corpo, n_vettore = ntv(d / f'{s}_corpo.wav'), ntv(d / f'{s}_vettore.wav')
    rows = []
    for g in OFFSETS:
        b = d
        n_mix = ntv(b / f'{s}_t{g}_mix.wav')
        n_trama = ntv(b / f'{s}_t{g}_trama.wav')
        inc = {r: n_mix - ntv(b / f'{s}_t{g}_mix_without_{r}.wav') for r in ('trama', 'corpo', 'vettore')}
        alone = {'trama': n_trama, 'corpo': n_corpo, 'vettore': n_vettore}
        loss = {r: 1 - inc[r] / alone[r] for r in inc}
        presence = loss['trama'] <= max(loss['corpo'], loss['vettore'])
        subordination = inc['trama'] < inc['corpo'] and inc['trama'] < inc['vettore']
        safety = (lufs[f'{s}_t{g}'] - lufs[f'{s}_t0']) <= 0.5   # limiter reduction 0 is checked by stem_export_v3
        ok_by_offset[g] &= presence and subordination and safety
        rows.append({'offset_db': g, 'mix_sone': n_mix, 'alone_sone': alone, 'increment_sone': inc, 'loss': loss,
                     'mix_lufs_rise': lufs[f'{s}_t{g}'] - lufs[f'{s}_t0'],
                     'presence': presence, 'subordination': subordination, 'safety': safety})
    out['sections'][s] = rows
passing = [g for g in OFFSETS if ok_by_offset[g]]
out['offsets_passing_everywhere'] = passing
out['choice_db'] = passing[0] if passing else None
Path(sys.argv[3]).write_text(json.dumps(out, indent=2), encoding='utf-8')
for s, rows in out['sections'].items():
    print(s)
    for r in rows:
        print('  +%d dB' % r['offset_db'], 'loss T/C/V', [round(r['loss'][k], 3) for k in ('trama', 'corpo', 'vettore')],
              'inc T/C/V', [round(r['increment_sone'][k], 2) for k in ('trama', 'corpo', 'vettore')],
              'mix +%.2f LU' % r['mix_lufs_rise'], 'P', r['presence'], 'S', r['subordination'], 'safe', r['safety'])
print('passing everywhere:', passing, 'choice:', out['choice_db'])
