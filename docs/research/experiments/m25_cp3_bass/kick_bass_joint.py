"""Kick and bass together, V2 matched to V1 in the low band (docs/research/M2_5_CP3_I1_I3_Kick_Bass_Joint_Preregistration.md).

  python -I kick_bass_joint.py --dir artifacts/cacp/bass_prototype_v1 --plan <csv> --out <json> [--clips <dir>]
"""
import argparse
import json
import sys
from pathlib import Path

import numpy as np
from scipy.io import wavfile

HERE = Path(__file__).resolve().parent
for q in (HERE, HERE.parent / 'expert_production_analysis', HERE.parent / 'm25_cp3_kick'):
    sys.path.insert(0, str(q))
import bass_investigation as B  # noqa: E402
import bass_prototype as BP  # noqa: E402
import kick_presence as K  # noqa: E402
import stem_analysis as S  # noqa: E402

FS = 48000


def sec_edges(n):
    b = [int(x * FS) for x in (*BP.START, BP.START[2] + 64.0)]  # floor, as the planner: rounding moved the first kick of A' into B
    return [(b[i], min(b[i + 1], n)) for i in range(3)]


def main():
    ap = argparse.ArgumentParser()
    for o in ('dir', 'plan', 'out', 'clips'):
        ap.add_argument('--' + o)
    a = ap.parse_args()
    d = Path(a.dir)
    rows, kicks = BP.plan(a.plan)
    ev, _ = BP.line(kicks)
    onk = np.array(sorted(int(r['sample_frame']) for r in rows if r['role'] == '0'))
    v1on = sorted({int(r['sample_frame']) for r in rows if r['role'] == '1'})
    v2on = np.array(sorted(int(round(e['t'] * FS)) for e in ev))
    out = {}
    clips = {}
    for kick in ('now', 'c2'):
        st = {n: BP.wav(d / f'{kick}_v0_{n}.wav') for n in ('pulso', 'corpo', 'trama', 'vettore')}
        mix0, v1c, mix1 = BP.wav(d / f'{kick}_v0_mix.wav'), BP.wav(d / f'{kick}_v1_corpo.wav'), BP.wav(d / f'{kick}_v1_mix.wav')
        n = min(len(mix0), len(v1c), *(len(x) for x in st.values()))
        res = {'J1_residual': float(np.abs(sum(x[:n] for x in st.values()) - mix0[:n]).max())}
        x = BP.wav(d / f'bass_{kick}_s0.wav').mean(axis=1)
        x = np.pad(x, (0, max(0, n - len(x))))[:n]
        raw = np.column_stack([x, x])
        low = {k: B.bp(v[:n], 30, 120) for k, v in st.items()}
        v1low = B.bp(v1c[:n], 30, 120)
        rawlow = B.bp(raw, 30, 120)
        g = np.zeros(n)
        gains = []
        for i, j in sec_edges(n):
            # energies summed as in I3: stems taken separately
            want = sum((low[k][i:j] ** 2).sum() for k in ('trama', 'vettore')) + (v1low[i:j] ** 2).sum()
            have = sum((low[k][i:j] ** 2).sum() for k in ('trama', 'vettore', 'corpo'))
            bass = (rawlow[i:j] ** 2).sum()
            gain = np.sqrt(max(want - have, 0) / bass)
            gains.append(float(20 * np.log10(gain)) if gain > 0 else None)
            g[i:j] = gain
        g[sec_edges(n)[2][1]:] = g[sec_edges(n)[2][1] - 1]
        bass = raw * g[:, None]
        arms = {'V0': (mix0[:n], st['corpo'][:n], None), 'V1': (mix1[:n], v1c[:n], v1on), 'V2m': (mix0[:n] + bass, st['corpo'][:n] + bass, v2on)}
        kp = K.kweight(st['pulso'][:n])
        bass_only = {'V0': None, 'V1': v1c[:n], 'V2m': bass}
        for arm, (mix, corpo, on) in arms.items():
            others = corpo + st['trama'][:n] + st['vettore'][:n]
            bp, bo = K.band(st['pulso'][:n], 40, 160), K.band(others, 40, 160)
            cp, co = K.band(st['pulso'][:n], 2000, 5000), K.band(others, 2000, 5000)
            r = {'peak_dbfs': float(20 * np.log10(np.abs(mix).max())), 'mix_lufs': S.integrated_lufs(mix, FS)}
            bb = B.bp(bass_only[arm], 40, 160) if arm != 'V0' else None
            kb = B.bp(st['pulso'][:n], 40, 160)
            if arm == 'V1':  # the bass voice alone is not separable: Corpo is the bass-carrying stem
                bb = B.bp(v1c[:n], 40, 160)
            for s, (i, j) in enumerate(sec_edges(n)):
                o = onk[(onk >= i) & (onk < j - int(0.2 * FS))]
                q = {'P_body_db': float(np.median(K.db(K.win_energy(bp, o, .020, .120)) - K.db(K.win_energy(bo, o, .020, .120)))),
                     'P_click_db': float(np.median(K.db(K.win_energy(cp, o, 0, .015)) - K.db(K.win_energy(co, o, 0, .015))))}
                q['E_K_db'] = float(K.db(K.win_energy(kp, o, 0, .120).mean()))
                q['Q100'] = float(K.q100(st['pulso'][:n], o))
                if bb is not None:
                    on_a = np.asarray(on)
                    co_k = [k for k in o if len(on_a) and np.abs(on_a - k).min() <= int(0.060 * FS)]
                    rr = [B.db((kb[k:k + 2880] ** 2).sum()) - B.db((bb[k:k + 2880] ** 2).sum()) for k in co_k if (bb[k:k + 2880] ** 2).sum() > 1e-9]
                    q.update(J4_R_coincident_db=float(np.median(rr)) if rr else None, coincident=len(rr), kicks=int(len(o)))
                    non = sum((low[k][i:j] ** 2).sum() for k in ('trama', 'vettore')) + ((v1low[i:j] ** 2).sum() if arm == 'V1' else (low['corpo'][i:j] ** 2).sum() + (B.bp(bass, 30, 120)[i:j] ** 2).sum())
                    q['nonkick_low_db'] = float(B.db(non))
                r['ABC'[s]] = q
            res[arm] = r
        res['V2m_gain_db'] = gains
        out[kick] = res
        clips[kick] = {'V0': mix0[:n], 'V1': mix1[:n], 'V2m': mix0[:n] + bass}
    c = out['c2']
    ok = {'J1': out['c2']['J1_residual'] <= 1e-4,
          'J2': all(abs(c['V2m'][s]['nonkick_low_db'] - c['V1'][s]['nonkick_low_db']) <= 0.5 for s in 'ABC'),
          'J6': all(c[v]['peak_dbfs'] <= -1 for v in ('V1', 'V2m')) and abs(c['V1']['mix_lufs'] - c['V2m']['mix_lufs']) <= 0.5,
          'J7': all(abs(c['V0']['A']['E_K_db'] - c['V0']['B']['E_K_db']) <= 0.1 for _ in (0,)) and all(c['V0'][s]['Q100'] >= 0.75 for s in 'ABC')}
    for v in ('V1', 'V2m'):
        ok[f'J3_{v}'] = all(c[v][s]['P_body_db'] >= 6 for s in 'ABC')
        ok[f'J4_{v}'] = all(c[v][s]['J4_R_coincident_db'] is not None and c[v][s]['J4_R_coincident_db'] >= 6 for s in 'ABC')
        ok[f'J5_{v}'] = all(c[v][s]['P_click_db'] >= 0 for s in 'ABC')
        ok[f'J5_drop_{v}'] = all(c['V0'][s]['P_click_db'] - c[v][s]['P_click_db'] <= 0.5 for s in 'ABC')
    out['outcome_c2'] = {k: bool(x) for k, x in ok.items()}
    Path(a.out).write_text(json.dumps(out, indent=1) + '\n', encoding='utf-8', newline='\n')
    print(json.dumps(out['outcome_c2']))
    for kick in ('now', 'c2'):
        print(kick, 'residual', round(out[kick]['J1_residual'], 6), 'gain', out[kick]['V2m_gain_db'])
        for v in ('V0', 'V1', 'V2m'):
            print(' ', v, {k: round(x, 2) for k, x in out[kick][v].items() if not isinstance(x, dict)})
            for s in 'ABC':
                print('    ', s, {k: (round(x, 3) if isinstance(x, float) else x) for k, x in out[kick][v][s].items()})


if __name__ == '__main__':
    main()
