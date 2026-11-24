"""Shared accents of kick and bass (docs/research/M2_5_CP3_I8_Accent_Prototype_Preregistration.md).

  python -I accent_prototype.py write   --plan <csv> --dir artifacts/cacp/tempo_ramp_v1   (empty directory)
  cymatica_game --cacp-run warped_plan_render_v1
  python -I accent_prototype.py measure --plan <csv> --dir ... --cacp artifacts/cacp --out <json> [--clips <dir> --key <file>]
"""
import argparse
import json
import random
import secrets
import sys
from pathlib import Path

import numpy as np
from scipy.io import wavfile

HERE = Path(__file__).resolve().parent
for q in (HERE, HERE.parent / 'm25_cp3_listening', HERE.parent / 'm25_cp3_kick', HERE.parent / 'expert_production_analysis'):
    sys.path.insert(0, str(q))
import accent_investigation as AI  # noqa: E402
import cp3_material as CM3  # noqa: E402
import kick_presence as K  # noqa: E402
import stem_analysis as S  # noqa: E402

T, BP, B, P, FS = CM3.T, CM3.BP, CM3.B, CM3.P, 48000
MASKS = [(0, 8), (0, 8, 11), (0, 6, 8), (0, 8, 14)]
SHIFT = {2, 6, 10, 14}
NAMES = ('g0', 'g1', 'g2')
CLIP = (int((64 + 4 * 240 / 140) * FS), int((64 + 16 * 240 / 140) * FS))  # bars 36-47 (bis)


def kick_bars(ev):
    d = {}
    for e in ev:
        if e['role'] == 0:
            d.setdefault(P.slot(e['f']) // 16, {})[P.slot(e['f']) % 16] = e
    return d


def accents(ev, v):
    """bar -> accent positions, and phrase -> mask (g1)."""
    kb = kick_bars(ev)
    out, masks = {}, {}
    for p in range(24):
        bars = [b for b in range(p * 4, p * 4 + 4) if b in kb and set(kb[b]) != SHIFT]
        if bars:
            k = set(kb[bars[0]])
            masks[p] = max(MASKS, key=lambda m: (len(set(m) & k) / len(m), len(m), -MASKS.index(m)))
        for b in range(p * 4, p * 4 + 4):
            if b not in kb:
                out[b] = (0, 8)
            elif set(kb[b]) == SHIFT and v == 'g1':
                out[b] = (2, 10)
            elif v == 'g1':
                out[b] = masks[p]
            else:
                out[b] = tuple(sorted(sorted(kb[b], key=lambda x: (-AI.W[x], x))[:3]))
    return out, masks


def kick_events(ev, v):
    if v != 'g1':
        return [e for e in ev if e['role'] == 0]
    acc, _ = accents(ev, v)
    out, nid = [], 3 * 10 ** 6
    for b, q in kick_bars(ev).items():
        if set(q) == SHIFT:
            out += list(q.values())
            continue
        first = q[min(q)]
        for s in sorted(set(acc[b]) | {0, 4, 8, 12}):
            e = dict(first)
            nid += 1
            e.update(id=nid, f=P.frame(b * 16 + s), v=f"{float(first['v']) * (1.0 if s in acc[b] else 0.8):.4f}")
            out.append(e)
    return out


def bass_line(ev, v):
    """Same fields as BP.line: bar, phrase, t, dur, midi."""
    if v == 'g0':
        return BP.line(CM3.kicks_of(ev))[0]
    acc, _ = accents(ev, v)
    kb = {}
    for e in kick_events(ev, v):
        kb.setdefault(P.slot(e['f']) // 16, set()).add(P.slot(e['f']) % 16)
    out = []
    for p in range(24):
        if p in BP.SILENT:
            continue
        for k in range(4):
            bar = p * 4 + k
            s, t0 = BP.bar_time(bar)
            root, nxt = BP.fold(BP.ROOTS[s][k]), BP.fold(BP.ROOTS[s][(k + 1) % 4])
            if p in BP.PEDAL:
                out.append({'bar': bar, 'phrase': p, 't': t0, 'dur': 0.95 * BP.BAR[s], 'midi': root})
                continue
            n = {}
            for i, a in enumerate(acc[bar]):
                fifth = root + 7 if root + 7 <= 45 else root - 5
                n[a] = root if i == 0 else ((root if k % 2 == 0 else fifth) if a in (8, 10) else root + 12)
            if len(kb.get(bar, ())) <= 4 and 14 not in n:
                m = nxt - 1
                while m % 12 not in BP.SCALES[s]:
                    m -= 1
                n[14] = m
            pos = sorted(n)
            for i, a in enumerate(pos):
                gap = ((pos[i + 1] if i + 1 < len(pos) else 16) - a) * BP.BAR[s] / 16
                out.append({'bar': bar, 'phrase': p, 't': t0 + a * BP.BAR[s] / 16, 'dur': min(0.8 * gap, BP.BAR[s] / 2), 'midi': n[a]})
    return out


def bass_rows(line):
    return [{'id': 2 * 10 ** 6 + i, 'role': 1, 'f': int(round(e['t'] * FS)), 'd': int(round(e['dur'] * FS)),
             'p': f"{440 * 2 ** ((e['midi'] - 69) / 12):.6f}", 'v': '0.85', 't': '0.40', 'off': 0} for i, e in enumerate(line)]


def plan(ev, v):
    return [e for e in ev if e['role'] != 0] + kick_events(ev, v)


def write(a):
    ev = T.read(a.plan)
    d = Path(a.dir)
    d.mkdir(parents=True, exist_ok=True)
    for v in NAMES:
        CM3.rows(d, f'k3_{v}', plan(ev, v))
        CM3.rows(d, f'k3_{v}kick', kick_events(ev, v))
        CM3.rows(d, f'bass_{v}', bass_rows(bass_line(ev, v)))
    print('masks', accents(ev, 'g1')[1])


def symbolic(ev, v):
    kick, line = kick_events(ev, v), bass_line(ev, v)
    kb, bb = {}, {}
    for e in kick:
        kb.setdefault(P.slot(e['f']) // 16, set()).add(P.slot(e['f']) % 16)
    for e in line:
        s, t0 = BP.bar_time(e['bar'])
        bb.setdefault(e['bar'], set()).add(int(round((e['t'] - t0) / (BP.BAR[s] / 16))) % 16)
    keep = [b for b in sorted(bb) if b in kb and b // 4 not in BP.PEDAL | BP.SILENT]
    r = AI.metrics([(kb[b], set(), bb[b]) for b in keep])
    w = {}
    for e in line:
        s, t0 = BP.bar_time(e['bar'])
        w.setdefault(e['phrase'], []).append((e['bar'] % 4, round((e['t'] - t0) / (BP.BAR[s] / 16)), e['midi']))
    ch = [w.get(p) != w.get(p - 1) for p in range(1, 24) if p // 8 == (p - 1) // 8]
    others = [(e['f'] / FS, ((e['off'] - e['f']) if e['off'] else e['d']) / FS, P.midi(e)) for e in ev if e['role'] in (1, 2, 3)]
    semi = np.mean([any(t <= e['t'] + 1e-4 < t + du and (e['midi'] - m) % 12 in (1, 11) for t, du, m in others) for e in line])
    return {'A1_excess': r['A1_excess'], 'kicks_with_bass': r['A1_kicks_with_bass'], 'bass_on_kick': r['bass_on_kick'],
            'lhl_kick': r['lhl_per_onset_kick'], 'lhl_bass': r['lhl_per_onset_bass'], 'windows_changed': sum(ch) / len(ch),
            'semitone_share': float(semi), 'bass_notes': len(line), 'kicks': len(kick)}


def measure(a):
    ev, d, c = T.read(a.plan), Path(a.dir), Path(a.cacp)
    man = json.loads((d / 'warped_plan_render_v1_manifest.json').read_text())
    pr = c / 'bass_prototype_v1'
    st = {r: T.wav(pr / f'c2_v0_{r}.wav') for r in ('corpo', 'trama', 'vettore')}
    v1c = T.wav(pr / 'c2_v1_corpo.wav')
    n = min(*(len(x) for x in st.values()), len(v1c), *(len(T.wav(d / f'k3_{v}.wav')) for v in NAMES))
    edges = [int(x * FS) for x in (*BP.START, BP.START[2] + 64.0)]
    low = {r: B.bp(x[:n], 30, 120) for r, x in st.items()}
    v1low = B.bp(v1c[:n], 30, 120)
    out, V, clips = {'manifest': man}, {}, {}
    for v in NAMES:
        raw = np.column_stack([CM3.mono(T.wav(d / f'bass_{v}.wav'), n)] * 2)
        rl, g, gains = B.bp(raw, 30, 120), np.zeros(n), []
        for s in range(3):
            i, j = edges[s], min(edges[s + 1], n)
            want = (v1low[i:j] ** 2).sum() - (low['corpo'][i:j] ** 2).sum()
            gains.append(float(np.sqrt(max(want, 0) / (rl[i:j] ** 2).sum())))
            g[i:j] = gains[-1]
        g[edges[3]:] = gains[2]
        bass = raw * g[:, None]
        mix = T.wav(d / f'k3_{v}.wav')[:n] + bass
        kick = T.wav(d / f'k3_{v}kick.wav')
        kick = np.pad(kick, ((0, max(0, n - len(kick))), (0, 0)))[:n]
        oth = sum(x[:n] for x in st.values()) + bass
        kb, ob, bb = K.band(kick, 40, 160), K.band(oth, 40, 160), B.bp(bass, 40, 160)
        kf = np.array(sorted(e['f'] for e in kick_events(ev, v)))
        bf = np.array(sorted(e['f'] for e in bass_rows(bass_line(ev, v))))
        r = symbolic(ev, v)
        for s in range(3):
            o = kf[(kf >= edges[s]) & (kf < edges[s + 1]) & (kf < n - 9600)]
            co = [k for k in o if np.abs(bf - k).min() <= int(.06 * FS) and (bb[k:k + 2880] ** 2).sum() > 1e-9]
            r['ABC'[s]] = {'P_body_db': float(np.median(K.db(K.win_energy(kb, o, .02, .12)) - K.db(K.win_energy(ob, o, .02, .12)))),
                           'R_coincident_db': float(np.median([B.db((kb[k:k + 2880] ** 2).sum()) - B.db((bb[k:k + 2880] ** 2).sum()) for k in co])),
                           'coincident': len(co), 'kicks': int(len(o))}
        r['peak_dbfs'] = float(20 * np.log10(np.abs(mix).max()))
        r['bass_gain_db'] = [float(20 * np.log10(x)) for x in gains]
        r['lufs'] = S.integrated_lufs(mix, FS)
        V[v], clips[v] = r, mix[CLIP[0]:CLIP[1]].copy()

    def key(x):
        return sorted((e['role'], e['f'], e['d'], e['p'], e['v'], e['t'], e['off']) for e in x if e['role'] != 0)
    bars = {v: sorted({P.slot(e['f']) // 16 for e in kick_events(ev, v)}) for v in NAMES}
    lu = {v: S.integrated_lufs(clips[v], FS) for v in NAMES}
    nf = FS // 100
    fin = {}
    for v in NAMES:
        y = clips[v] * 10 ** ((min(lu.values()) - lu[v]) / 20)
        y[:nf] *= np.linspace(0, 1, nf)[:, None]
        y[-nf:] *= np.linspace(1, 0, nf)[:, None]
        fin[v] = (y * 32767).astype(np.int16)
    lf = {v: S.integrated_lufs(fin[v] / 32768.0, FS) for v in NAMES}
    out['versions'] = V
    out['outcome'] = {
        'B1': all(V[v]['A1_excess'] >= 0.15 for v in ('g1', 'g2')),
        'B2': V['g1']['lhl_kick'] <= 0.5 and all(V[v]['lhl_bass'] <= 0.6 for v in ('g1', 'g2')),
        'B3': all(V[v][s]['P_body_db'] >= 6 and V[v][s]['R_coincident_db'] >= 6 for v in ('g1', 'g2') for s in 'ABC') and all(V[v]['peak_dbfs'] <= -1 for v in NAMES),
        'B4': all(key(plan(ev, v)) == key(ev) for v in NAMES) and bars['g0'] == bars['g1'] == bars['g2'],
        'B5': all(V[v]['windows_changed'] >= 0.5 for v in ('g1', 'g2')),
        'B6': all(V[v]['semitone_share'] <= V['g0']['semitone_share'] + 0.05 for v in ('g1', 'g2')),
        'B7': man['status'] == 'MEASURED' and len(man['renders']) == 9 and
        all(r['max_gain_reduction_db'] == 0 and r['staged_at_end'] == 0 and r['backpressure_drops'] == 0 for r in man['renders']) and max(lf.values()) - min(lf.values()) <= 0.1}
    if a.clips and out['outcome']['B4'] and out['outcome']['B7']:
        seed = secrets.randbits(32)
        rng = random.Random(seed)
        names = list(NAMES) + [rng.choice(NAMES)]
        rng.shuffle(names)
        cl = Path(a.clips)
        cl.mkdir(parents=True, exist_ok=True)
        for i, k in enumerate(names, 1):
            wavfile.write(cl / f'accenti_{i}.wav', FS, fin[k])
        Path(a.key).write_text(f'CHIAVE - non aprire prima di aver risposto\nseed {seed}\n' + '\n'.join(f'accenti_{i} = {k}' for i, k in enumerate(names, 1)) + '\n', encoding='utf-8')
    Path(a.out).write_text(json.dumps(out, indent=1, default=lambda v: v.item()) + '\n', encoding='utf-8', newline='\n')
    print(json.dumps(out['outcome'], default=lambda v: v.item()))
    for v in NAMES:
        print(v, {k: (round(x, 3) if isinstance(x, float) else x) for k, x in V[v].items()})


if __name__ == '__main__':
    ap = argparse.ArgumentParser()
    ap.add_argument('mode', choices=('write', 'measure'))
    for o in ('plan', 'dir', 'cacp', 'out', 'clips', 'key'):
        ap.add_argument('--' + o)
    a = ap.parse_args()
    (write if a.mode == 'write' else measure)(a)
