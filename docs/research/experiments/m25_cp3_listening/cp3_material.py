"""Material of the CP-3 expert listening (docs/research/M2_5_CP3_Expert_Listening_Hypotheses.md).

  python -I cp3_material.py write   --plan <csv> --dir artifacts/cacp/tempo_ramp_v1   (empty directory)
  cymatica_game --cacp-run warped_plan_render_v1
  python -I cp3_material.py measure --plan <csv> --dir ... --cacp artifacts/cacp --joint <kick_bass_joint.json> --out <json>
`measure` also writes piece_z.wav in --dir.
"""
import argparse
import json
import sys
from pathlib import Path

import numpy as np
from scipy.io import wavfile

HERE = Path(__file__).resolve().parent
for q in (HERE.parent / 'm25_cp3_melody', HERE.parent / 'm25_cp3_transitions', HERE.parent / 'm25_cp3_bass', HERE.parent / 'expert_production_analysis'):
    sys.path.insert(0, str(q))
import bass_investigation as B  # noqa: E402
import bass_prototype as BP  # noqa: E402
import breath_variants as V  # noqa: E402
import register_return as RR  # noqa: E402
import stem_analysis as S  # noqa: E402
import tempo_ramp as T  # noqa: E402

FS, P = 48000, V.P


def kicks_of(ev):
    return np.array(sorted(e['f'] for e in ev if e['role'] == 0)) / FS


def z_plan(ev):
    return RR.variant(V.variant(ev, 'c'), 'x')[0]


def rows(d, name, ev):
    with open(d / f'{name}.csv', 'w', newline='\n') as w:
        for e in sorted(ev, key=lambda e: (e['f'], e['id'])):
            w.write(f"{e['id']},{e['role']},{e['f']},{e['d']},{e['p']},{e['v']},{e['t']},{e['off'] or ''}\n")


def bass_events(kicks):
    return [{'id': 2 * 10 ** 6 + i, 'role': 1, 'f': int(round(e['t'] * FS)), 'd': int(round(e['dur'] * FS)),
             'p': f"{440 * 2 ** ((e['midi'] - 69) / 12):.6f}", 'v': '0.85', 't': '0.40', 'off': 0} for i, e in enumerate(BP.line(kicks)[0])]


def write(a):
    ev = T.read(a.plan)
    d = Path(a.dir)
    d.mkdir(parents=True, exist_ok=True)
    z = z_plan(ev)
    rows(d, 'k3_today', ev)
    rows(d, 'k3_z', z)
    rows(d, 'k3_zkick', [e for e in z if e['role'] == 0])
    rows(d, 'bass_today', bass_events(kicks_of(ev)))
    rows(d, 'bass_z', bass_events(kicks_of(z)))


def mono(x, n):
    x = x.mean(axis=1)
    return np.pad(x, (0, max(0, n - len(x))))[:n]


def measure(a):
    d, c = Path(a.dir), Path(a.cacp)
    ev = T.read(a.plan)
    man = json.loads((d / 'warped_plan_render_v1_manifest.json').read_text())
    ref, today = T.wav(c / 'bass_prototype_v1/c2_v0_mix.wav'), T.wav(d / 'k3_today.wav')
    n1 = min(len(ref), len(today))
    bref, btoday = T.wav(c / 'bass_prototype_v1/bass_c2_s0.wav'), T.wav(d / 'bass_today.wav')
    n2 = min(len(bref), len(btoday))
    out = {'T1': float(np.abs(ref[:n1] - today[:n1]).max()), 'T2': float(np.abs(mono(bref, n2) - mono(btoday, n2)).max())}
    z, bz, zk = T.wav(d / 'k3_z.wav'), T.wav(d / 'bass_z.wav'), T.wav(d / 'k3_zkick.wav')
    n = len(z)
    gains = json.loads(Path(a.joint).read_text())['c2']['V2m_gain_db']
    edges = [int(x * FS) for x in (*BP.START, BP.START[2] + 64.0)]
    g = np.full(n, 10 ** (gains[2] / 20))
    for s in range(3):
        g[edges[s]:edges[s + 1]] = 10 ** (gains[s] / 20)
    bass = np.column_stack([mono(bz, n)] * 2) * g[:, None]
    mix = z + bass
    out['z_peak_dbfs'] = float(20 * np.log10(np.abs(mix).max()))
    out['z_lufs'] = S.integrated_lufs(mix, FS)
    zp, cp = z_plan(ev), V.variant(ev, 'c')

    def conflicts(x):
        return sum(P.clash(x, e['f'], P.midi(e)) for e in x if e['role'] == 3)
    out['conflicts'] = {'c': conflicts(cp), 'z': conflicts(zp)}
    kb, bb = B.bp(zk[:min(n, len(zk))], 40, 160), B.bp(bass, 40, 160)
    kf = np.array(sorted(e['f'] for e in zp if e['role'] == 0))
    bf = np.array(sorted(e['f'] for e in bass_events(kicks_of(zp))))
    out['T4'] = {}
    for s in range(3):
        rr, cnt = [], 0
        for k in kf[(kf >= edges[s]) & (kf < edges[s + 1] - 2880)]:
            if np.abs(bf - k).min() <= int(.06 * FS):
                e = (bb[k:k + 2880] ** 2).sum()
                if e > 1e-9 and k + 2880 < len(kb):
                    rr.append(B.db((kb[k:k + 2880] ** 2).sum()) - B.db(e))
        out['T4']['ABC'[s]] = {'R_coincident_db': float(np.median(rr)) if rr else None, 'n': len(rr)}
    out['renders'] = man['renders']
    zr = [r for r in man['renders'] if r['name'] in ('k3_z', 'bass_z')]
    out['outcome'] = {
        'T1': out['T1'] <= 1e-3, 'T2': out['T2'] <= 1e-3,
        'T3': bool(man['status'] == 'MEASURED' and all(r['max_gain_reduction_db'] == 0 and r['staged_at_end'] == 0 and r['backpressure_drops'] == 0 for r in zr)
                   and out['z_peak_dbfs'] <= -1 and out['conflicts']['z'] <= out['conflicts']['c']),
        'T4': all(v['R_coincident_db'] is not None and v['R_coincident_db'] >= 6 for v in out['T4'].values())}
    wavfile.write(d / 'piece_z.wav', FS, (np.clip(mix, -1, 1) * 32767).astype(np.int16))
    Path(a.out).write_text(json.dumps(out, indent=1, default=lambda v: v.item()) + '\n', encoding='utf-8', newline='\n')
    print(json.dumps({k: v for k, v in out.items() if k != 'renders'}))
    for r in man['renders']:
        print(r)


if __name__ == '__main__':
    ap = argparse.ArgumentParser()
    ap.add_argument('mode', choices=('write', 'measure'))
    for o in ('plan', 'dir', 'cacp', 'joint', 'out'):
        ap.add_argument('--' + o)
    a = ap.parse_args()
    (write if a.mode == 'write' else measure)(a)
