"""Tempo ramp prototype (docs/research/M2_5_CP3_I5_Tempo_Ramp_Prototype_Preregistration.md).

  python -I tempo_ramp.py write   --plan <fixture_intents csv> --dir artifacts/cacp/tempo_ramp_v1
  cymatica_game --cacp-run warped_plan_render_v1
  python -I tempo_ramp.py measure --dir ... --reference <base mix wav> --clips <listener dir> --key <key file, never opened by the listener>
"""
import argparse
import csv
import json
import secrets
import random
import sys
from pathlib import Path

import numpy as np
from scipy.io import wavfile

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE.parent / 'expert_production_analysis'))
import stem_analysis as S  # noqa: E402

FS = 48000
NB = 96 * 4  # beats


def beat_lengths(version):
    """Seconds per beat for every beat of the (possibly longer) piece, and the index of the inserted bar's first beat."""
    a, b = 0.5, 60 / 140
    if version == 'a':
        return [a] * 128 + [b] * 128 + [a] * 128, None
    if version == 'b':
        return [a] * NB, None
    if version == 'c':
        return [a] * 132 + [b] * 128 + [a] * 128, 128
    if version == 'd':
        # tempo linear in beats from 120 to 140 over the 8 beats of bars 30-31: exact integral of 60 / T(b)
        r = [60 * np.log((120 + 2.5 * (k + 1)) / (120 + 2.5 * k)) / 2.5 for k in range(8)]
        return [a] * 120 + r + [b] * 128 + [a] * 128, None
    raise ValueError(version)


def ramp_time(version):
    """Monotone map: beat position of the original plan -> seconds in the version."""
    ln, ins = beat_lengths(version)
    cum = np.concatenate([[0.0], np.cumsum(ln)])

    def f(beat):
        if ins is not None and beat >= ins - 1e-9:
            beat = beat + 4
        k = min(int(np.floor(beat + 1e-9)), len(ln) - 1)
        x = beat - k
        if version == 'd' and 120 <= k < 128:
            q = k - 120
            return cum[k] + 60 * np.log((120 + 2.5 * (q + x)) / (120 + 2.5 * q)) / 2.5
        return cum[k] + x * ln[k]
    return f, cum


def original_beat(frame):
    b1 = 64.0 * FS
    b2 = b1 + 128 * 60 * FS / 140
    if frame < b1:
        return frame / (0.5 * FS)
    if frame < b2:
        return 128 + (frame - b1) / (60 * FS / 140)
    return 256 + (frame - b2) / (0.5 * FS)


def read(plan):
    rows = list(csv.reader(open(plan)))[1:]
    off = {r[1]: int(r[3]) for r in rows if r[0] == '#noteoff'}
    ev = []
    for r in rows:
        if r[0] == '#noteoff':
            continue
        ev.append({'id': int(r[0]), 'role': int(r[3]), 'f': int(r[4]), 'd': int(r[5]), 'p': r[6], 'v': r[7], 't': r[8],
                   'off': off.get(r[0], 0)})
    return ev


def write(a):
    ev = read(a.plan)
    d = Path(a.dir)
    d.mkdir(parents=True, exist_ok=True)
    for ver in 'abcd':
        f, _ = ramp_time(ver)

        def fr(frame):
            return frame if ver == 'a' else int(round(f(original_beat(frame)) * FS))
        out = []
        for e in ev:
            s = fr(e['f'])
            n = fr(e['f'] + e['d']) - s if e['d'] else 0
            o = fr(e['off']) if e['off'] else 0
            out.append((s, e['id'], e['role'], max(n, 1) if e['d'] else 0, e['p'], e['v'], e['t'], o or ''))
        if ver == 'c':  # bridge: Pulso and Trama of bar 31 repeated one bar later, at 120
            nid = 100000
            for e in ev:
                if e['role'] in (0, 2) and 62.0 * FS <= e['f'] < 64.0 * FS:
                    out.append((e['f'] + 2 * FS, nid, e['role'], e['d'], e['p'], e['v'], e['t'], ''))
                    nid += 1
        out.sort(key=lambda x: (x[0], x[1]))
        with open(d / f'{ver}.csv', 'w', newline='\n') as w:
            for s, i, r, n, p, v, t, o in out:
                w.write(f'{i},{r},{s},{n},{p},{v},{t},{o}\n')
    print('written')


def wav(p):
    return wavfile.read(p)[1].astype(np.float64) / 32768.0


def measure(a):
    d = Path(a.dir)
    man = json.loads((d / 'warped_plan_render_v1_manifest.json').read_text())
    out = {'manifest': man}
    ref, ra = wav(a.reference), wav(d / 'a.wav')
    n = min(len(ref), len(ra), int(182.85 * FS))
    out['V1_max_abs_difference'] = float(np.abs(ref[:n] - ra[:n]).max())
    clips = {}
    jumps = {}
    for ver in 'abcd':
        ln, ins = beat_lengths(ver)
        jumps[ver] = float(max(abs(y / x - 1) for x, y in zip(ln[:200], ln[1:200])))
        _, cum = ramp_time(ver)
        c0 = cum[128 if ins is None else 132]
        x = wav(d / f'{ver}.wav')
        # same length for every version (so that duration does not reveal b): boundary at 16 s, 8 bars of B at 140 after it
        clips[ver] = x[int((c0 - 16) * FS):int((c0 - 16) * FS) + int((16 + 32 * 60 / 140) * FS)]
        out[f'boundary_s_{ver}'] = float(c0)
    ap0 = 64 + 128 * 60 / 140
    x = ra
    clips['e'] = x[int((ap0 - 32 * 60 / 140) * FS):int((ap0 + 16) * FS)]
    out['V2_max_beat_length_jump'] = jumps
    lu = {k: S.integrated_lufs(v, FS) for k, v in clips.items()}
    out['V4_lufs'] = lu
    out['clip_seconds'] = {k: len(v) / FS for k, v in clips.items()}
    ok = {'V1': out['V1_max_abs_difference'] <= 1e-3,
          'V2': abs(jumps['a'] - 1 / 7) < 0.01 and abs(jumps['c'] - 1 / 7) < 0.01 and jumps['b'] < 1e-9 and jumps['d'] <= 0.025,
          'V3': man['status'] == 'MEASURED' and all(r['max_gain_reduction_db'] == 0 and r['staged_at_end'] == 0 and r['backpressure_drops'] == 0
                                                     for r in man['renders']),
          'V4': max(lu[k] for k in 'abcd') - min(lu[k] for k in 'abcd') <= 1.0 and all(abs(lu[k] - lu['a']) <= 0.5 for k in 'abcd')}
    out['outcome'] = ok
    if a.clips and ok['V1']:
        seed = secrets.randbits(32)
        names = list('abcd')
        random.Random(seed).shuffle(names)
        c = Path(a.clips)
        c.mkdir(parents=True, exist_ok=True)
        low = min(lu.values())
        nf = FS // 100
        fade = np.linspace(0, 1, nf)
        for i, k in enumerate(names, 1):
            y = clips[k] * 10 ** ((low - lu[k]) / 20)
            y[:nf] *= fade[:, None]
            y[-nf:] *= fade[::-1, None]
            wavfile.write(c / f'estratto_{i}.wav', FS, (y * 32767).astype(np.int16))
        y = clips['e'] * 10 ** ((low - lu['e']) / 20)
        wavfile.write(c / 'riferimento_B_Aprime.wav', FS, (y * 32767).astype(np.int16))
        Path(a.key).write_text('CHIAVE - non aprire prima di aver risposto\nseed ' + str(seed) + '\n' +
                               '\n'.join(f'estratto_{i} = {k}' for i, k in enumerate(names, 1)) + '\n', encoding='utf-8')
    print(json.dumps(out, indent=1, default=lambda v: v.item()))


if __name__ == '__main__':
    ap = argparse.ArgumentParser()
    ap.add_argument('mode', choices=('write', 'measure'))
    ap.add_argument('--plan')
    ap.add_argument('--dir', required=True)
    ap.add_argument('--reference')
    ap.add_argument('--clips')
    ap.add_argument('--key')
    a = ap.parse_args()
    (write if a.mode == 'write' else measure)(a)
