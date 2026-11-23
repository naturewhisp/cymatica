"""Two-bar motifs, question/answer (docs/research/M2_5_CP3_I4_I6_Two_Bar_Prototype_Preregistration.md).

  python -I two_bar_prototype.py write   --plan <csv> --dir artifacts/cacp/tempo_ramp_v1   (directory without other CSV)
  cymatica_game --cacp-run warped_plan_render_v1
  python -I two_bar_prototype.py measure --plan <csv> --dir ... --reference <wav> --out <json> [--clips <dir> --key <file>]
"""
import argparse
import json
import random
import secrets
import sys
from collections import defaultdict
from pathlib import Path

import numpy as np
from scipy.io import wavfile

HERE = Path(__file__).resolve().parent
for q in (HERE, HERE.parent / 'm25_cp3_transitions', HERE.parent / 'expert_production_analysis'):
    sys.path.insert(0, str(q))
import melody_investigation as I6  # noqa: E402
import stem_analysis as S  # noqa: E402
import tempo_ramp as T  # noqa: E402

FS = 48000
NAMES = 'pmtb'
SCALE = [{2, 4, 5, 7, 9, 11, 0}, {7, 9, 10, 0, 2, 3, 5}]
TONIC = [2, 7]
COR = {1, 6, 11}


def midi(e):
    return int(round(69 + 12 * np.log2(float(e['p']) / 440)))


def hz(m):
    return f'{440 * 2 ** ((m - 69) / 12):.4f}'


def slot(f):
    return int(round(T.original_beat(f) * 4))


def frame(sl):
    b = sl / 4
    if b <= 128:
        return int(b * 24000)
    if b <= 256:
        return int(64 * FS + (b - 128) * 60 * FS / 140)
    return int(64 * FS + 128 * 60 * FS / 140 + (b - 256) * 24000)


def sec(sl):
    return 1 if 512 <= sl < 1024 else 0


def step(m, n, sc):
    s = sorted(x for x in range(0, 128) if x % 12 in SCALE[sc])
    return s[s.index(m) + n] if m in s else m


def degrees(a, b, sc):
    s = sorted(x for x in range(0, 128) if x % 12 in SCALE[sc])
    return s.index(b) - s.index(a)


def corpo_at(ev, f):
    return [midi(e) for e in ev if e['role'] == 1 and e['f'] <= f + 24 < (e['off'] or e['f'] + e['d'])]


def clash(ev, f, m):
    return any(abs(m - c) % 12 in COR for c in corpo_at(ev, f))


class Build:
    def __init__(self, ev):
        self.ev, self.fallbacks, self.new = ev, 0, 10 ** 6

    def melody(self):
        bars = defaultdict(list)
        for e in self.ev:
            if e['role'] == 3:
                bars[slot(e['f']) // 16].append(dict(e))
        for b in bars:
            bars[b].sort(key=lambda e: e['f'])
        src = {b: [dict(e) for e in q] for b, q in bars.items()}
        for p in range(24):
            self.prime(bars, src, p * 4 + 1, None)
            self.prime(bars, src, p * 4 + 3, 'answer' if p % 2 else 'question')
        return [e for b in sorted(bars) for e in bars[b]]

    def prime(self, bars, src, b, kind):
        q = [dict(e) for e in src[b]]
        sc = sec(b * 16)
        head = [e for e in q if slot(e['f']) % 16 < 8]
        tail = [e for e in q if slot(e['f']) % 16 >= 8]
        if not head or not tail:
            return
        axis = midi(head[-1])
        if kind == 'answer':
            e, old = tail[0], midi(tail[0])
            f = frame(b * 16 + 8)
            top = max(corpo_at(self.ev, f), default=0)
            c = sorted((x for x in range(24, 108) if x % 12 == TONIC[sc]), key=lambda x: (x <= top, abs(x - old)))
            e.update(f=f, d=frame(b * 16 + 12) - f, off=0, p=hz(next((x for x in c[:6] if not clash(self.ev, f, x)), old)))
            bars[b] = head + [e]
            return
        for e in tail:
            m = midi(e)
            n = step(axis, -degrees(axis, m, sc), sc)
            if clash(self.ev, e['f'], n):
                self.fallbacks += n != m
                n = m
            e['p'] = hz(n)
        out = head + tail
        if len(q) >= 5:
            last = out.pop()
            out[-1]['d'] = last['f'] + last['d'] - out[-1]['f']
        else:
            last = out[-1]
            nxt = src.get(b + 1)
            target = midi(nxt[0]) if nxt else midi(last)
            half = last['d'] // 2
            up = 1 if target >= midi(last) else -1
            f2 = frame(slot(last['f']) + 2) if slot(last['f']) % 16 <= 12 else last['f'] + half
            n = step(midi(last), up, sc)
            if clash(self.ev, f2, n):
                self.fallbacks += 1
                n = midi(last)
            add = dict(last)
            self.new += 1
            add.update(id=self.new, f=f2, d=max(1, last['f'] + last['d'] - f2), p=hz(n))
            if f2 - last['f'] < last['d']:
                last['d'] = f2 - last['f']
            else:
                add['d'] = last['d']
            out.append(add)
        if kind == 'question':
            e = out[-1]
            third, fifth = (TONIC[sc] + 3) % 12, (TONIC[sc] + 7) % 12
            c = sorted((x for x in range(24, 108) if x % 12 in (third, fifth)), key=lambda x: abs(x - midi(e)))
            e['p'] = hz(next((x for x in c[:6] if not clash(self.ev, e['f'], x)), midi(e)))
        bars[b] = out

    def rhythm(self, role):
        bars = defaultdict(list)
        for e in self.ev:
            if e['role'] == role:
                bars[slot(e['f']) // 16].append(dict(e))
        for b, q in bars.items():
            sl = sorted({slot(e['f']) for e in q})
            if len(sl) < 2 or b % 2 == 0:
                continue
            if b % 4 == 1:
                q[:] = [e for e in q if slot(e['f']) != sl[-1]]
            elif role == 0:
                pos = next((x for x in (14, 15) if b * 16 + x not in sl), None)
                if pos is not None:
                    for e in [e for e in q if slot(e['f']) == sl[-1]]:
                        n = dict(e)
                        self.new += 1
                        n.update(id=self.new, f=frame(b * 16 + pos), v=f"{float(e['v']) * 0.7:.4f}")
                        q.append(n)
            else:
                to = next((sl[-1] - k for k in (2, 1) if sl[-1] - k not in sl), None)
                if to is not None:
                    for e in q:
                        if slot(e['f']) == sl[-1]:
                            shift = frame(to) - e['f']
                            e['f'] += shift
                            e['off'] = e['off'] + shift if e['off'] else 0
        return [e for b in sorted(bars) for e in bars[b]]


def variant(ev, v):
    b = Build(ev)
    out = [dict(e) for e in ev]
    if v in 'mb':
        out = [e for e in out if e['role'] != 3] + b.melody()
    if v in 'tb':
        out = [e for e in out if e['role'] not in (0, 2)] + b.rhythm(0) + b.rhythm(2)
    return sorted(out, key=lambda e: (e['f'], e['id'])), b.fallbacks


def write(a):
    ev = T.read(a.plan)
    d = Path(a.dir)
    d.mkdir(parents=True, exist_ok=True)
    for v in NAMES:
        out, fb = variant(ev, v)
        with open(d / f'{v}.csv', 'w', newline='\n') as w:
            for e in out:
                w.write(f"{e['id']},{e['role']},{e['f']},{e['d']},{e['p']},{e['v']},{e['t']},{e['off'] or ''}\n")
        print(v, 'events', len(out), 'fallbacks', fb)


def bars_of(ev, role):
    d = [set() for _ in range(96)]
    for e in ev:
        if e['role'] == role:
            d[slot(e['f']) // 16].add(slot(e['f']) % 16)
    return d


def g1(ev, role):
    d = bars_of(ev, role)
    x = [d[p * 4 + k] == d[p * 4 + k - 1] for p in range(24) for k in (1, 2, 3) if d[p * 4 + k] and d[p * 4 + k - 1]]
    return sum(x) / len(x)


def symbolic(ev):
    vet = [e for e in ev if e['role'] == 3]
    m = I6.analyse([(slot(e['f']), midi(e)) for e in vet])
    conf = sum(clash(ev, e['f'], midi(e)) for e in vet)
    ph = [[] for _ in range(24)]
    for e in vet:
        c = corpo_at(ev, e['f'])
        ph[slot(e['f']) // 64].append(bool(c) and midi(e) > max(c))
    return {'M1': m['same_contour_as_bar1'], 'G1': {str(r): g1(ev, r) for r in (0, 2, 3)}, 'end_on_tonic': m['phrase_end_on_tonic'],
            'breaths': m['M6_breath_phrases'], 'ic_mean': m['ic_mean'], 'conflicts': conf,
            'above_share_per_phrase': [sum(x) / len(x) for x in ph]}


def z2(ev, o, v):
    roles = {'p': (), 'm': (3,), 't': (0, 2), 'b': (0, 2, 3)}[v]

    def key(x):
        return sorted((e['role'], e['f'], e['d'], e['p'], e['v'], e['t'], e['off']) for e in x)
    ok = key([e for e in ev if e['role'] not in roles]) == key([e for e in o if e['role'] not in roles])

    def head(x):
        return sorted((e['f'], e['d'], e['p'], e['v']) for e in x if e['role'] == 3 and slot(e['f']) % 16 < 8)
    ids = {e['id']: midi(e) for e in ev if e['role'] == 3}
    scale = all(midi(e) % 12 in SCALE[sec(slot(e['f']))] for e in o if e['role'] == 3 and ids.get(e['id']) != midi(e))
    return ok and head(ev) == head(o) and scale


def measure(a):
    ev = T.read(a.plan)
    d = Path(a.dir)
    man = json.loads((d / 'warped_plan_render_v1_manifest.json').read_text())
    out, V, clips, ref = {'manifest': man}, {}, {}, T.wav(a.reference)
    z2ok = True
    for v in NAMES:
        o, fb = variant(ev, v)
        z2ok &= z2(ev, o, v)
        x = T.wav(d / f'{v}.wav')
        if v == 'p':
            n = min(len(ref), len(x), int(182.85 * FS))
            out['Z1_max_abs_difference'] = float(np.abs(ref[:n] - x[:n]).max())
        clips[v] = x[:64 * FS].copy()
        V[v] = symbolic(o)
        V[v].update(fallbacks=fb, events=len(o), lufs=S.integrated_lufs(clips[v], FS), lufs_whole=S.integrated_lufs(x, FS))
    out['versions'], p = V, V['p']
    out['outcome'] = {
        'Z1': out['Z1_max_abs_difference'] <= 1e-3, 'Z2': z2ok,
        'Z3': all(V[v]['M1'] < 0.6 for v in 'mb'),
        'Z4': all(V[v]['G1']['3'] < 0.6 for v in 'mb') and all(V[v]['G1'][r] < 0.5 for v in 'tb' for r in '02'),
        'Z5': all(V[v]['end_on_tonic']['odd'] - V[v]['end_on_tonic']['even'] >= 0.5 for v in 'mb'),
        'Z6': all(V[v]['breaths'] >= 8 for v in 'mb'),
        'Z7': all(V[v]['conflicts'] <= p['conflicts'] for v in 'mtb') and
        all(not (x >= 0.5 and y < 0.5) for v in 'mb' for x, y in zip(p['above_share_per_phrase'], V[v]['above_share_per_phrase'])),
        'Z8': man['status'] == 'MEASURED' and len(man['renders']) == 4 and
        all(r['max_gain_reduction_db'] == 0 and r['staged_at_end'] == 0 and r['backpressure_drops'] == 0 for r in man['renders']) and
        all(abs(V[v]['lufs'] - p['lufs']) <= 0.5 and abs(V[v]['lufs_whole'] - p['lufs_whole']) <= 0.5 for v in 'mtb')}
    if a.clips and all(out['outcome'][k] for k in ('Z1', 'Z2', 'Z8')):
        seed = secrets.randbits(32)
        names = list(NAMES)
        random.Random(seed).shuffle(names)
        c = Path(a.clips)
        c.mkdir(parents=True, exist_ok=True)
        low, nf = min(V[v]['lufs'] for v in NAMES), FS // 100
        fade = np.linspace(0, 1, nf)[:, None]
        for i, k in enumerate(names, 1):
            y = clips[k] * 10 ** ((low - V[k]['lufs']) / 20)
            y[:nf] *= fade
            y[-nf:] *= fade[::-1]
            wavfile.write(c / f'frase_{i}.wav', FS, (y * 32767).astype(np.int16))
        Path(a.key).write_text('CHIAVE - non aprire prima di aver risposto\nseed ' + str(seed) + '\n' +
                               '\n'.join(f'frase_{i} = {k}' for i, k in enumerate(names, 1)) + '\n', encoding='utf-8')
    Path(a.out).write_text(json.dumps(out, indent=1, default=lambda v: v.item()) + '\n', encoding='utf-8', newline='\n')
    print(json.dumps(out['outcome']), out['Z1_max_abs_difference'])
    for v in NAMES:
        print(v, {k: x for k, x in V[v].items()})


if __name__ == '__main__':
    ap = argparse.ArgumentParser()
    ap.add_argument('mode', choices=('write', 'measure'))
    for o in ('plan', 'dir', 'reference', 'clips', 'key', 'out'):
        ap.add_argument('--' + o)
    a = ap.parse_args()
    (write if a.mode == 'write' else measure)(a)
