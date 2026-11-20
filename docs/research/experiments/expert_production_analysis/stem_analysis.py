"""Stem measures S1-S4 of docs/research/M2_5_Expert_Production_Analysis_Preregistration.md, and S1 on CYMATICA.

The expert's MP3 stems are decoded outside the repository (ffmpeg, approved by the owner for this analysis only) into
WAV float files in --gift. This script prints aggregate statistics only.
Usage: python -I stem_analysis.py --gift <decoded wav folder> --midi <midi folder> --cymatica <cumulative_listening_v1 dir>
"""
import argparse
import json
import sys
from pathlib import Path

import numpy as np
from scipy.io import wavfile
from scipy.signal import lfilter, welch

sys.path.insert(0, str(Path(__file__).resolve().parent))
import midi_analysis as M  # noqa: E402

BPM = 162.0
BAR_S = 4 * 60.0 / BPM
# Declared before the measure (pre-registration §1): group stems and effect returns, excluded from S1
GROUPS = {'Drums', 'Percs', 'Strings Bus', 'MIX', 'A-Reverb'}
# S4: non-bass stems (strings, pad, arp, cymbals, shakers), declared in the pre-registration
NON_BASS = ['Strings', 'Strings (Riversa)', 'DX 7 PAD', 'Arp', 'Crashes', 'Rev Crashes', 'Ride Bell', 'Reverse Ride Bell',
            'Ride Glitched', 'Hi-Shaker', 'Low-Shaker']
ACTIVE_DB = -40.0


def load(path):
    fs, x = wavfile.read(path)
    x = x.astype(np.float64)
    if x.dtype.kind == 'i' or np.abs(x).max() > 1.5:
        x = x / 32768.0
    if x.ndim == 1:
        x = x[:, None]
    return fs, x


def bar_energy(x, fs, bar_edges_s):
    e = []
    p = (x ** 2).mean(axis=1)
    for a, b in zip(bar_edges_s, bar_edges_s[1:]):
        i, j = int(round(a * fs)), int(round(b * fs))
        e.append(p[i:j].mean() if j > i else 0.0)
    return np.array(e)


def activity(e):
    m = e.max()
    if m <= 0:
        return np.zeros_like(e, dtype=bool)
    with np.errstate(divide='ignore'):
        db = 10 * np.log10(np.maximum(e, 1e-30) / m)
    return db > ACTIVE_DB


def change_stats(acts):
    """acts: dict name -> bool array per bar. A change bar is a bar where at least one stem flips state."""
    n = min(len(a) for a in acts.values())
    flips = np.zeros(n, dtype=int)
    for a in acts.values():
        a = a[:n]
        flips[1:] += (a[1:] != a[:-1]).astype(int)
    bars = np.nonzero(flips)[0]
    iv = np.diff(bars)
    return {'bars': int(n), 'change_bars': int(len(bars)), 'flips_total': int(flips.sum()),
            'S1_median_interval_bars': float(np.median(iv)) if len(iv) else None,
            'interval_quartiles_bars': [float(np.percentile(iv, 25)), float(np.percentile(iv, 75))] if len(iv) else None,
            'interval_histogram_bars': {int(k): int(v) for k, v in zip(*np.unique(iv, return_counts=True))} if len(iv) else {},
            'eight_bar_windows_with_change_share': float(np.mean([flips[w:w + 8].sum() > 0 for w in range(0, n, 8)])),
            'first_change_bars': [int(b) for b in bars[:12]]}


# BS.1770-4 K-weighting at 48 kHz (ITU-R BS.1770-4, Table 1 and 2)
K1_B = [1.53512485958697, -2.69169618940638, 1.19839281085285]
K1_A = [1.0, -1.69065929318241, 0.73248077421585]
K2_B = [1.0, -2.0, 1.0]
K2_A = [1.0, -1.99004745483398, 0.99007225036621]


def integrated_lufs(x, fs):
    if fs != 48000:
        raise ValueError('K-weighting coefficients are for 48 kHz')
    y = lfilter(K2_B, K2_A, lfilter(K1_B, K1_A, x, axis=0), axis=0)
    blk, hop = int(0.4 * fs), int(0.1 * fs)
    z = np.array([(y[i:i + blk] ** 2).mean(axis=0).sum() for i in range(0, len(y) - blk + 1, hop)])
    lk = -0.691 + 10 * np.log10(np.maximum(z, 1e-30))
    g = z[lk > -70]
    rel = -0.691 + 10 * np.log10(g.mean()) - 10
    g2 = z[(lk > -70) & (lk > rel)]
    return float(-0.691 + 10 * np.log10(g2.mean()))


def low_share(x, fs, cutoff=100.0):
    f, p = welch(x.mean(axis=1), fs=fs, nperseg=8192)
    tot = p.sum()
    return float(p[f < cutoff].sum() / tot) if tot > 0 else None


def sidechain(bass, fs, kick_times):
    p = (bass ** 2).mean(axis=1)
    m = p.max()
    d = []
    for t in kick_times:
        i0, i1 = int(t * fs), int((t + 0.030) * fs)
        j0, j1 = int((t + 0.120) * fs), int((t + 0.150) * fs)
        if j1 >= len(p):
            continue
        late = p[j0:j1].mean()
        if late <= m * 10 ** (ACTIVE_DB / 10):
            continue  # bass silent at that kick
        early = p[i0:i1].mean()
        d.append(10 * np.log10(max(late, 1e-30) / max(early, 1e-30)))
    return {'kicks_with_bass': len(d), 'S2_median_ducking_db': float(np.median(d)) if d else None,
            'ducking_quartiles_db': [float(np.percentile(d, 25)), float(np.percentile(d, 75))] if d else None}


def kick_alignment(drum, fs, kick_times):
    """Check that the MIDI time base matches the audio: energy of the drum stem right after the MIDI kicks vs right before."""
    p = (drum ** 2).mean(axis=1)
    r = []
    for t in kick_times:
        a, b, c = int((t - 0.03) * fs), int(t * fs), int((t + 0.03) * fs)
        if a < 0 or c >= len(p):
            continue
        r.append(10 * np.log10(max(p[b:c].mean(), 1e-30) / max(p[a:b].mean(), 1e-30)))
    return {'kicks': len(r), 'median_onset_rise_db': float(np.median(r)) if r else None}


def midi_offset(drum, fs, kick_times, span=8.0, step=0.001, hop=48):
    """Post-hoc correction, declared after the alignment check failed (median onset rise -7.5 dB): the offset of the MIDI
    time base that maximises the drum-stem onset energy at the MIDI kick times."""
    p = (drum ** 2).mean(axis=1)
    env = np.add.reduceat(p, np.arange(0, len(p), hop))
    on = np.maximum(np.diff(env, prepend=env[0]), 0)
    kt = np.asarray(kick_times)
    best = (0.0, -1.0)
    for off in np.arange(-span, span, step):
        idx = ((kt + off) * fs / hop).astype(int)
        idx = idx[(idx >= 0) & (idx < len(on))]
        sc = on[idx].sum()
        if sc > best[1]:
            best = (float(off), sc)
    return best[0]


def cymatica_bars(section_start_b, section_start_ap, end_s):
    edges = [k * 2.0 for k in range(32)]                       # A: 120 BPM, 2 s per bar
    edges += [section_start_b + k * (4 * 60.0 / 140) for k in range(32)]  # B: 140 BPM
    edges += [section_start_ap + k * 2.0 for k in range(33)]   # A': 120 BPM, closing edge included
    return [e for e in edges if e <= end_s]


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--gift', required=True)
    ap.add_argument('--midi', required=True)
    ap.add_argument('--cymatica', required=True)
    a = ap.parse_args()
    gift, out = Path(a.gift), {}

    stems = {p.stem: p for p in sorted(gift.glob('*.wav'))}
    fs, mix = load(stems['MIX'])
    dur = len(mix) / fs
    edges = list(np.arange(0.0, dur + 1e-9, BAR_S))
    acts = {}
    for name, p in stems.items():
        if name in GROUPS:
            continue
        _, x = load(p)
        acts[name] = activity(bar_energy(x, fs, edges))
    out['expert'] = {'duration_s': dur, 'bar_s': BAR_S, 'stems_in_S1': sorted(acts),
                     'S1': change_stats(acts),
                     'active_bars_per_stem': {k: int(v.sum()) for k, v in acts.items()}}
    out['expert']['S3_integrated_lufs'] = integrated_lufs(mix, fs)
    shares = {}
    for name in NON_BASS:
        if name in stems:
            _, x = load(stems[name])
            shares[name] = low_share(x, fs)
    vals = [v for v in shares.values() if v is not None]
    out['expert']['S4_low_share_per_stem'] = shares
    out['expert']['S4_median_low_share'] = float(np.median(vals)) if vals else None
    for name in ('808 Bass', 'Sub Bass'):
        if name in stems:
            _, x = load(stems[name])
            out['expert'][f'low_share_{name}'] = low_share(x, fs)

    ks = M.read_smf(Path(a.midi) / 'Kick & Snare.mid')
    kick_times = [n['tick'] / ks['ppq'] * 60.0 / BPM for n in ks['notes'] if n['pitch'] == 36]
    _, drum = load(stems['21-Drum Rack'])
    out['expert']['midi_audio_alignment'] = kick_alignment(drum, fs, kick_times)
    _, bass = load(stems['808 Bass'])
    out['expert']['S2_unaligned'] = sidechain(bass, fs, kick_times)
    off = midi_offset(drum, fs, kick_times)
    aligned = [t + off for t in kick_times]
    out['expert']['midi_offset_s'] = round(off, 4)
    out['expert']['midi_audio_alignment_after_offset'] = kick_alignment(drum, fs, aligned)
    out['expert']['S2'] = sidechain(bass, fs, aligned)

    cy = Path(a.cymatica)
    man = json.loads((cy / 'cumulative_listening_v1_manifest.json').read_text(encoding='utf-8'))
    sb, sap = man['section_start_s']['B'], man['section_start_s']['Aprime']
    for piece in ('riferimento', 'seed_34'):
        cacts, cfs = {}, None
        for role in ('pulso', 'corpo', 'trama', 'vettore'):
            cfs, x = load(cy / f'stem_{piece}_{role}.wav')
            e = cymatica_bars(sb, sap, len(x) / cfs)
            cacts[role] = activity(bar_energy(x, cfs, e))
        _, cm = load(cy / (man['piece_1_reference']['file'] if piece == 'riferimento' else man['piece_2_second_path']['file']))
        lows = {role: low_share(load(cy / f'stem_{piece}_{role}.wav')[1], 48000) for role in ('pulso', 'corpo', 'trama', 'vettore')}
        out[f'cymatica_{piece}'] = {'S1': change_stats(cacts), 'active_bars_per_role': {k: int(v.sum()) for k, v in cacts.items()},
                                    'integrated_lufs': integrated_lufs(cm, cfs), 'low_share_per_role': lows}
    print(json.dumps(out, indent=1))


if __name__ == '__main__':
    main()
