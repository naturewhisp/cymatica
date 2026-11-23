"""Bass line V2 and measures P1-P7 of docs/research/M2_5_CP3_I3_Bass_Prototype_Preregistration.md.

  python -I bass_prototype.py write   --plan <fixture_intents_seed1337.csv> --dir artifacts/cacp/bass_prototype_v1
  cymatica_game --cacp-run bass_prototype_v1
  python -I bass_prototype.py measure --plan ... --dir ... [--clips <out dir>]
"""
import argparse
import csv
import json
import sys
from pathlib import Path

import numpy as np
from scipy.io import wavfile

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE.parent / 'expert_production_analysis'))
sys.path.insert(0, str(HERE))
import bass_investigation as B  # noqa: E402
import stem_analysis as S  # noqa: E402

FS = 48000
CELLS = ['R--R--O-', 'R-R--F-O', 'R--F-O-N', '-R--R-O-', 'R------N', '-R-F--RN']
PEDAL = {1, 7, 8, 15, 16, 23}
SILENT = {0}
ROOTS = [[2, 7, 9, 2], [7, 3, 5, 7], [2, 7, 9, 2]]
SCALES = [{2, 4, 5, 7, 9, 11, 0}, {7, 9, 10, 0, 2, 3, 5}, {2, 4, 5, 7, 9, 11, 0}]
BAR = [2.0, 4 * 60 / 140, 2.0]
START = [0.0, 64.0, 64.0 + 32 * 4 * 60 / 140]
LEVELS = (-6, -3, 0)


def fold(pc):
    return 28 + (pc - 28) % 12


def bar_time(bar):
    s = bar // 32
    return s, START[s] + (bar % 32) * BAR[s]


def coincident(t, s, kicks):
    return len(kicks) > 0 and np.abs(kicks - t).min() <= BAR[s] / 32


def cell_hits(cell, bar, kicks):
    s, t0 = bar_time(bar)
    on = [t0 + q * BAR[s] / 8 for q, c in enumerate(cell) if c != '-']
    return sum(coincident(t, s, kicks) for t in on), len(on)


def choose_pairs(kicks):
    pairs = {}
    for p in range(24):
        if p in SILENT or p in PEDAL:
            continue
        if p >= 16 and (p - 16) in pairs:
            pairs[p] = pairs[p - 16]
            continue
        prev = pairs.get(p - 1)
        best = None
        for a in range(6):
            for b in range(6):
                if a == b or (a, b) == prev:
                    continue
                h = n = 0
                for k, c in enumerate(phrase_cells(a, b)):
                    x, y = cell_hits(CELLS[c], p * 4 + k, kicks)
                    h, n = h + x, n + y
                d = abs(h / n - 0.36)
                if best is None or d < best[0] - 1e-12:
                    best = (d, (a, b))
        pairs[p] = best[1]
    return pairs


def phrase_cells(a, b):
    bs = (b + 1) % 6
    if bs == a:
        bs = (bs + 1) % 6
    return [a, b, a, bs]


def line(kicks):
    """Events of V2: dicts bar, phrase, t, dur, midi (S0)."""
    pairs = choose_pairs(kicks)
    ev = []
    for p in range(24):
        for k in range(4):
            bar = p * 4 + k
            s, t0 = bar_time(bar)
            root = fold(ROOTS[s][k])
            if p in SILENT:
                continue
            if p in PEDAL:
                ev.append({'bar': bar, 'phrase': p, 't': t0, 'dur': 0.95 * BAR[s], 'midi': root})
                continue
            cell = CELLS[phrase_cells(*pairs[p])[k]]
            if p % 2 == 1 and k == 0:
                cell = cell.translate(str.maketrans('OF', 'FO'))
            on = [q for q, c in enumerate(cell) if c != '-']
            nxt = fold(ROOTS[s][(k + 1) % 4])
            for i, q in enumerate(on):
                gap = ((on[i + 1] if i + 1 < len(on) else 8) - q) * BAR[s] / 8
                midi = {'R': root, 'O': root + 12, 'F': root + 7}.get(cell[q])
                if midi is None:
                    midi = nxt - 1
                    while midi % 12 not in SCALES[s]:
                        midi -= 1
                if cell[q] == 'F' and midi > 45:
                    midi -= 12
                ev.append({'bar': bar, 'phrase': p, 't': t0 + q * BAR[s] / 8, 'dur': min(0.8 * gap, BAR[s] / 2), 'midi': midi})
    return ev, pairs


def plan(path):
    rows = [r for r in csv.DictReader(open(path)) if r['sample_frame'] and r['pitch_hz']]  # drop the NoteOff trailer
    kicks = np.array(sorted(int(r['sample_frame']) for r in rows if r['role'] == '0')) / FS
    return rows, kicks


def write(a):
    _, kicks = plan(a.plan)
    ev, _ = line(kicks)
    d = Path(a.dir)
    d.mkdir(parents=True, exist_ok=True)
    for kick in ('now', 'c2'):
        for sep in ('s0', 's1'):
            with open(d / f'bass_{kick}_{sep}.csv', 'w', newline='\n') as f:
                for e in ev:
                    t, dur = e['t'], e['dur']
                    if sep == 's1' and coincident(t, e['bar'] // 32, kicks) and dur > 0.1:
                        t, dur = t + 0.060, dur - 0.060
                    f.write(f'{t:.9f},{440 * 2 ** ((e["midi"] - 69) / 12):.6f},0.85,{dur:.9f},0.40\n')
    print('notes', len(ev))


def wav(p):
    return wavfile.read(p)[1].astype(np.float64) / 32768.0


def measure(a):
    rows, kicks = plan(a.plan)
    ev, pairs = line(kicks)
    d = Path(a.dir)
    man = json.loads((d / 'bass_prototype_v1_manifest.json').read_text())
    bounds = [int(round(x * FS)) for x in (*START, START[2] + 64.0)]
    out = {'manifest_status': man['status'], 'renders': man['renders'], 'pairs': {str(k): v for k, v in pairs.items()}}
    edges = [bar_time(b)[1] for b in range(96)] + [START[2] + 64.0]
    # Plan notes of the pitched roles; the bass voice of Corpo is the lower note of each simultaneous pair
    notes = [(int(r['role']), int(r['sample_frame']) / FS, int(r['duration_frames']) / FS,
              int(round(69 + 12 * np.log2(float(r['pitch_hz']) / 440)))) for r in rows if r['role'] != '0']
    corpo = {}
    for r, t, dur, m in notes:
        if r == 1:
            corpo.setdefault(round(t, 4), []).append((m, dur))
    low = [(t, v[0][1], min(x[0] for x in v)) for t, v in sorted(corpo.items())]
    others = [(t, dur, m) for r, t, dur, m in notes if r in (2, 3)]
    for t, v in corpo.items():
        hi = max(v)
        others.append((t, hi[1] if hi[1] > 0 else 1.0, hi[0]))

    def semitone_share(on):
        c = 0
        for t, m in on:
            hit = False
            for t2, d2, m2 in others:
                if t2 <= t + 1e-4 < t2 + d2:
                    ic = (m - m2) % 12
                    hit = hit or ic in (1, 11)
            c += hit
        return c / len(on)
    v1_on = [(t, m - 12) for t, _, m in low]
    v2_on = [(e['t'], e['midi']) for e in ev]
    out['P4'] = {'V1': semitone_share(v1_on), 'V2': semitone_share(v2_on)}

    def windows(events):
        w = []
        for p in range(24):
            b0 = edges[p * 4]
            w.append(tuple(sorted((round((t - b0) / (BAR[p // 8] / 16)), m) for t, m in events if edges[p * 4] - 1e-3 <= t < edges[p * 4 + 4] - 1e-3)))
        ch = [w[p] != w[p - 1] for p in range(1, 24) if p // 8 == (p - 1) // 8]
        return sum(ch) / len(ch)
    out['P5'] = {'V1': windows(v1_on), 'V2': windows(v2_on)}
    ln = [e for e in ev if e['phrase'] not in PEDAL]
    out['P7_coincidence_line'] = sum(coincident(e['t'], e['bar'] // 32, kicks) for e in ln) / len(ln)
    out['V2_notes'] = len(ev)
    onf = (kicks * FS).round().astype(int)
    for kick in ('now', 'c2'):
        st = {n: wav(d / f'{kick}_v0_{n}.wav') for n in ('pulso', 'corpo', 'trama', 'vettore')}
        mix0 = wav(d / f'{kick}_v0_mix.wav')
        v1c, mix1 = wav(d / f'{kick}_v1_corpo.wav'), wav(d / f'{kick}_v1_mix.wav')
        n = min(len(mix0), *(len(x) for x in st.values()))
        res = {'master_residual_max': float(np.abs(sum(x[:n] for x in st.values()) - mix0[:n]).max())}
        lowb = {k: B.bp(x[:n], 30, 120) for k, x in st.items()}
        v1low = B.bp(v1c[:n], 30, 120)
        kb = B.bp(st['pulso'][:n], 40, 160)
        raw = {}
        for sep in ('s0', 's1'):
            x = wav(d / f'bass_{kick}_{sep}.wav').mean(axis=1)
            x = np.pad(x, (0, max(0, n - len(x))))[:n]
            raw[sep] = np.column_stack([x, x])
        # section gain from S0 at each level, kept for S1
        gains = []
        for s in range(3):
            i, j = bounds[s], min(bounds[s + 1], n)
            c = (B.kw(st['corpo'][i:j]) ** 2).sum(axis=1).mean()
            b = (B.kw(raw['s0'][i:j]) ** 2).sum(axis=1).mean()
            gains.append(np.sqrt(c / b))
        for lev in LEVELS:
            for sep in ('s0', 's1'):
                g = np.ones(n)
                for s in range(3):
                    g[bounds[s]:bounds[s + 1]] = gains[s] * 10 ** (lev / 20)
                g[bounds[3]:] = gains[2] * 10 ** (lev / 20)
                bass = raw[sep] * g[:, None]
                bl, bb = B.bp(bass, 30, 120), B.bp(bass, 40, 160)
                r = {}
                for s in range(3):
                    i, j = bounds[s], min(bounds[s + 1], n)
                    e = {k: (x[i:j] ** 2).sum() for k, x in lowb.items()}
                    tot0 = sum(e.values())
                    non0 = tot0 - e['pulso']
                    non1 = non0 - e['corpo'] + (v1low[i:j] ** 2).sum()
                    non2 = non0 + (bl[i:j] ** 2).sum()
                    rr = []
                    for k in onf[(onf >= i) & (onf < j - 2880)]:
                        eb = (bb[k:k + 2880] ** 2).sum()
                        if eb > 1e-9:
                            rr.append(B.db((kb[k:k + 2880] ** 2).sum()) - B.db(eb))
                    r['ABC'[s]] = {'P2_V0': float(non0 / tot0), 'P2_V1': float(non1 / (non1 + e['pulso'])),
                                   'P2_V2': float(non2 / (non2 + e['pulso'])), 'R_median_db': float(np.median(rr)), 'kicks_with_bass': len(rr)}
                m = mix0[:n] + bass
                acts = {k: S.activity(S.bar_energy(x[:n], FS, edges)) for k, x in st.items()}
                base = S.change_stats(acts)
                acts['bass'] = S.activity(S.bar_energy(bass, FS, edges))
                r['peak_dbfs'] = float(20 * np.log10(np.abs(m).max()))
                r['mix_lufs'] = S.integrated_lufs(m, FS)
                r['P6_V0'] = base['eight_bar_windows_with_change_share']
                r['P6_V2'] = S.change_stats(acts)['eight_bar_windows_with_change_share']
                r['R_section_gain_db'] = [float(20 * np.log10(x)) for x in gains]
                res[f'{lev:+d}_{sep}'] = r
                if a.clips and lev == -3 and sep == 's0':
                    Path(a.clips).mkdir(parents=True, exist_ok=True)
                    lim = np.clip(m, -1, 1)
                    for tag, x in (('V0', mix0[:n]), ('V1', mix1[:n]), ('V2', lim)):
                        # every clip attenuated to the loudness of the quietest arm, so that level does not bias the comparison
                        ref = min(S.integrated_lufs(y, FS) for y in (mix0[:n], mix1[:n], lim))
                        x = x * 10 ** ((ref - S.integrated_lufs(x, FS)) / 20)
                        wavfile.write(Path(a.clips) / f'{kick}_{tag}.wav', FS, (x * 32767).astype(np.int16))
        res['mix_lufs_V0'], res['mix_lufs_V1'] = S.integrated_lufs(mix0, FS), S.integrated_lufs(mix1, FS)
        out[kick] = res
    ok = {}
    for kick in ('now', 'c2'):
        r = out[kick]['-3_s0']
        ren = {x['tag']: x for x in man['renders']}
        v0, v1 = ren[f'{kick}_v0'], ren[f'{kick}_v1']
        ok[kick] = {
            'P1': bool(man['status'] == 'MEASURED' and ren['now_v0']['golden_pcm_status'] == 'PASS' and v1['same_pulso'] and v1['same_trama']
                       and v1['same_vettore'] and v0['max_gain_reduction_db'] == 0 and v1['max_gain_reduction_db'] == 0 and r['peak_dbfs'] <= -1),
            'P2': all(r[s]['P2_V2'] > r[s]['P2_V0'] and r[s]['P2_V1'] > r[s]['P2_V0'] for s in 'ABC'),
            'P3': all(r[s]['R_median_db'] >= 6 for s in 'ABC'),
            'P4': out['P4']['V2'] <= out['P4']['V1'] + 0.05,
            'P5': out['P5']['V1'] <= 0.10 and out['P5']['V2'] >= 0.80,
            'P6': r['P6_V2'] >= r['P6_V0'], 'P7': 0.25 <= out['P7_coincidence_line'] <= 0.50}
        ok[kick]['all'] = all(ok[kick].values())
    out['verdict'] = ok
    print(json.dumps(out, indent=1, default=lambda x: x.item()))


if __name__ == '__main__':
    ap = argparse.ArgumentParser()
    ap.add_argument('mode', choices=('write', 'measure'))
    ap.add_argument('--plan', required=True)
    ap.add_argument('--dir', required=True)
    ap.add_argument('--clips')
    a = ap.parse_args()
    (write if a.mode == 'write' else measure)(a)
