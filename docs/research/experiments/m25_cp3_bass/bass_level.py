"""Bass level rule (docs/research/M2_5_CP3_I3b_Bass_Level_Preregistration.md).

  python -I bass_level.py --plan <csv> --dir artifacts/cacp/tempo_ramp_v1 --cacp artifacts/cacp --out <json>
"""
import argparse
import json
import sys
from pathlib import Path

import numpy as np

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE.parent / 'm25_cp3_accents'))
import accent_prototype as A  # noqa: E402

T, BP, B, K, S, CM3, FS = A.T, A.BP, A.B, A.K, A.S, A.CM3, 48000
ap = argparse.ArgumentParser()
for o in ('plan', 'dir', 'cacp', 'out'):
    ap.add_argument('--' + o, required=True)
a = ap.parse_args()
ev, d, pr = T.read(a.plan), Path(a.dir), Path(a.cacp) / 'bass_prototype_v1'
st = {r: T.wav(pr / f'c2_v0_{r}.wav') for r in ('corpo', 'trama', 'vettore')}
n = min(*(len(x) for x in st.values()), *(len(T.wav(d / f'k3_{v}.wav')) for v in ('g0', 'g1')))
st = {r: x[:n] for r, x in st.items()}
edges = [int(x * FS) for x in (*BP.START, BP.START[2] + 64.0)]
sec = [(edges[s], min(edges[s + 1], n)) for s in range(3)]
ck, cm = B.kw(st['corpo']), B.bp(st['corpo'], 120, 250)
out = {'levels': {}}
base = {}
data = {}
for v in ('g0', 'g1'):
    raw = np.column_stack([CM3.mono(T.wav(d / f'bass_{v}.wav'), n)] * 2)
    rk = B.kw(raw)
    g0 = [float(np.sqrt((ck[i:j] ** 2).sum() / (rk[i:j] ** 2).sum())) for i, j in sec]
    kick = T.wav(d / f'k3_{v}kick.wav')
    kick = np.pad(kick, ((0, max(0, n - len(kick))), (0, 0)))[:n]
    mix0 = T.wav(d / f'k3_{v}.wav')[:n]
    data[v] = (raw, g0, kick, mix0, B.bp(raw, 120, 250), K.band(raw, 40, 160), B.bp(raw, 40, 160), S.integrated_lufs(mix0, FS),
               np.array(sorted(e['f'] for e in A.kick_events(ev, v))), np.array(sorted(e['f'] for e in A.bass_rows(A.bass_line(ev, v)))))
oth0 = sum(st.values())
ob0 = K.band(oth0, 40, 160)
out['control_gain_db_g0_at_-3'] = [float(20 * np.log10(x) - 3) for x in data['g0'][1]]
for lev in range(-6, 4):
    res = {}
    for v, (raw, g0, kick, mix0, rm, rband, rbp, lu0, kf, bf) in data.items():
        g = np.zeros(n)
        for (i, j), x in zip(sec, g0):
            g[i:j] = x * 10 ** (lev / 20)
        g[sec[2][1]:] = g0[2] * 10 ** (lev / 20)
        G = g[:, None]
        kb, ob, kbp = K.band(kick, 40, 160), ob0 + rband * G, B.bp(kick, 40, 160)
        r = {}
        for s, (i, j) in enumerate(sec):
            o = kf[(kf >= i) & (kf < j) & (kf < n - 9600)]
            co = [k for k in o if np.abs(bf - k).min() <= int(.06 * FS)]
            rr = []
            for k in co:
                e = ((rbp[k:k + 2880] * G[k:k + 2880]) ** 2).sum()
                if e > 1e-9:
                    rr.append(B.db((kbp[k:k + 2880] ** 2).sum()) - B.db(e))
            r['ABC'[s]] = {'P_body_db': float(np.median(K.db(K.win_energy(kb, o, .02, .12)) - K.db(K.win_energy(ob, o, .02, .12)))),
                           'R_db': float(np.median(rr)), 'mid_bass_minus_corpo_db': float(B.db(((rm[i:j] * G[i:j]) ** 2).sum()) - B.db((cm[i:j] ** 2).sum()))}
        m = mix0 + raw * G
        r['peak_dbfs'] = float(20 * np.log10(np.abs(m).max()))
        r['mix_delta_lu'] = S.integrated_lufs(m, FS) - lu0
        r['N'] = {'N1': all(r[s]['P_body_db'] >= 6 for s in 'ABC'), 'N2': all(r[s]['R_db'] >= 6 for s in 'ABC'), 'N3': r['peak_dbfs'] <= -1,
                  'N4': r['mix_delta_lu'] <= 1.5, 'N5': all(r[s]['mid_bass_minus_corpo_db'] <= 0 for s in 'ABC')}
        res[v] = r
    out['levels'][lev] = res
ok = [lev for lev, r in out['levels'].items() if all(all(r[v]['N'].values()) for v in r)]
out['selected'] = max(ok) if ok else None
Path(a.out).write_text(json.dumps(out, indent=1, default=lambda v: v.item()) + '\n', encoding='utf-8', newline='\n')
print('control', out['control_gain_db_g0_at_-3'], 'selected', out['selected'])
for lev, r in out['levels'].items():
    for v in r:
        print(lev, v, 'P', [round(r[v][s]['P_body_db'], 1) for s in 'ABC'], 'R', [round(r[v][s]['R_db'], 1) for s in 'ABC'],
              'mid', [round(r[v][s]['mid_bass_minus_corpo_db'], 1) for s in 'ABC'], round(r[v]['peak_dbfs'], 1), round(r[v]['mix_delta_lu'], 2), [k for k, x in r[v]['N'].items() if not x])
