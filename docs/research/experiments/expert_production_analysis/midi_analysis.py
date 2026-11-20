"""MIDI measures D1-D6 of docs/research/M2_5_Expert_Production_Analysis_Preregistration.md.

The material stays outside the repository; this script reads it from --midi and prints aggregate statistics only.
Minimal Standard MIDI File reader (format 0/1, running status, meta and sysex skipped), no dependency.
Usage: python -I midi_analysis.py --midi <folder with the .mid files> [--dump]
"""
import argparse
import json
from collections import Counter
from pathlib import Path


def varlen(b, i):
    v = 0
    while True:
        c = b[i]
        i += 1
        v = (v << 7) | (c & 0x7F)
        if not c & 0x80:
            return v, i


def read_smf(path):
    b = path.read_bytes()
    if b[:4] != b'MThd':
        raise ValueError(f'{path.name}: not a MIDI file')
    hlen = int.from_bytes(b[4:8], 'big')
    fmt, ntrk, div = int.from_bytes(b[8:10], 'big'), int.from_bytes(b[10:12], 'big'), int.from_bytes(b[12:14], 'big')
    if div & 0x8000:
        raise ValueError('SMPTE division not supported')
    i = 8 + hlen
    notes, timesig, tempo = [], None, None
    for _ in range(ntrk):
        if b[i:i + 4] != b'MTrk':
            raise ValueError('bad track chunk')
        tlen = int.from_bytes(b[i + 4:i + 8], 'big')
        j, end, t, status = i + 8, i + 8 + tlen, 0, None
        open_notes = {}
        while j < end:
            d, j = varlen(b, j)
            t += d
            c = b[j]
            if c == 0xFF:
                typ = b[j + 1]
                ln, k = varlen(b, j + 2)
                data = b[k:k + ln]
                if typ == 0x58 and timesig is None:
                    timesig = (data[0], 2 ** data[1])
                if typ == 0x51 and tempo is None:
                    tempo = int.from_bytes(data, 'big')
                j = k + ln
                continue
            if c in (0xF0, 0xF7):
                ln, k = varlen(b, j + 1)
                j = k + ln
                continue
            if c & 0x80:
                status = c
                j += 1
            hi = status & 0xF0
            nbytes = 1 if hi in (0xC0, 0xD0) else 2
            data = b[j:j + nbytes]
            j += nbytes
            if hi == 0x90 and data[1] > 0:
                open_notes.setdefault(data[0], []).append((t, data[1]))
            elif hi == 0x80 or (hi == 0x90 and data[1] == 0):
                if open_notes.get(data[0]):
                    t0, vel = open_notes[data[0]].pop(0)
                    notes.append({'tick': t0, 'dur': t - t0, 'pitch': data[0], 'vel': vel})
        i = end
    notes.sort(key=lambda n: (n['tick'], n['pitch']))
    return {'format': fmt, 'tracks': ntrk, 'ppq': div, 'timesig': timesig or (4, 4), 'tempo_us': tempo, 'notes': notes}


PHRYGIAN_F = {5, 6, 8, 10, 0, 1, 3}  # F Gb Ab Bb C Db Eb


def bar_beat(n, ppq, num=4):
    bar_ticks = ppq * num
    return n['tick'] // bar_ticks, (n['tick'] % bar_ticks) / ppq


def summary(name, m):
    ppq, notes = m['ppq'], m['notes']
    pitches = Counter(n['pitch'] for n in notes)
    bars = (max((n['tick'] + n['dur']) for n in notes) // (ppq * 4) + 1) if notes else 0
    return {'file': name, 'ppq': ppq, 'timesig': m['timesig'], 'notes': len(notes), 'bars_spanned': bars,
            'pitch_histogram': dict(sorted(pitches.items()))}


def drums(m, kick, snare):
    ppq = m['ppq']
    k = [n for n in m['notes'] if n['pitch'] in kick]
    s = [n for n in m['notes'] if n['pitch'] in snare]
    tol = ppq // 8  # a 32nd-note tolerance around the beat
    def on_beat(n, beat):
        _, pos = bar_beat(n, ppq)
        return abs(pos - beat) * ppq <= tol
    sbars = {}
    for n in s:
        b, _ = bar_beat(n, ppq)
        sbars.setdefault(b, []).append(n)
    kbars = {}
    for n in k:
        b, _ = bar_beat(n, ppq)
        kbars.setdefault(b, []).append(n)
    backbeat = sum(1 for b, ns in sbars.items() if any(on_beat(n, 1) for n in ns) and any(on_beat(n, 3) for n in ns))
    kick1 = sum(1 for b, ns in kbars.items() if any(on_beat(n, 0) for n in ns))
    vmax = max((n['vel'] for n in s), default=0)
    ghosts = sum(1 for n in s if n['vel'] <= 0.6 * vmax)
    # Onset positions on a 16th grid, for the pattern profile
    def grid(ns):
        return Counter(round((bar_beat(n, ppq)[1]) * 4) % 16 for n in ns)
    return {'kick_pitches': sorted(kick), 'snare_pitches': sorted(snare),
            'snare_bars': len(sbars), 'snare_bars_with_backbeat': backbeat,
            'D1_backbeat_share': backbeat / len(sbars) if sbars else None,
            'kick_bars': len(kbars), 'kick_bars_with_beat1': kick1,
            'D2_kick_beat1_share': kick1 / len(kbars) if kbars else None,
            'snare_hits': len(s), 'snare_vel_max': vmax, 'snare_ghosts': ghosts,
            'D3_ghost_share': ghosts / len(s) if s else None,
            'snare_velocity_histogram': dict(sorted(Counter(n['vel'] for n in s).items())),
            'kick_grid16': dict(sorted(grid(k).items())), 'snare_grid16': dict(sorted(grid(s).items())),
            'kick_per_bar_median': sorted(len(v) for v in kbars.values())[len(kbars) // 2] if kbars else None}


def bass(m):
    ppq, notes = m['ppq'], m['notes']
    pcs = Counter(n['pitch'] % 12 for n in notes)
    inscale = sum(v for pc, v in pcs.items() if pc in PHRYGIAN_F)
    durs = sorted(n['dur'] / ppq for n in notes)
    # Period: for each bar b and period p in {1,2,4}, does bar b repeat bar b-p (same onsets and pitches)?
    bars = {}
    for n in notes:
        b, pos = bar_beat(n, ppq)
        bars.setdefault(b, []).append((round(pos * 4), n['pitch']))
    nb = max(bars) + 1 if bars else 0
    content = [tuple(sorted(bars.get(b, []))) for b in range(nb)]
    period = {}
    for p in (1, 2, 4, 8):
        comp = [(content[b] == content[b - p]) for b in range(p, nb) if content[b] or content[b - p]]
        period[p] = sum(comp) / len(comp) if comp else None
    # 4-bar windows that differ from the previous 4-bar window
    win = [tuple(content[b:b + 4]) for b in range(0, nb, 4)]
    changed = [win[w] != win[w - 1] for w in range(1, len(win)) if any(win[w]) or any(win[w - 1])]
    names = ['C', 'C#', 'D', 'Eb', 'E', 'F', 'F#', 'G', 'Ab', 'A', 'Bb', 'B']
    return {'notes': len(notes), 'D4_in_scale_share': inscale / len(notes) if notes else None,
            'pc_histogram': {names[pc]: v for pc, v in sorted(pcs.items())},
            'most_frequent_pc': names[pcs.most_common(1)[0][0]] if pcs else None,
            'pitch_range': [min(n['pitch'] for n in notes), max(n['pitch'] for n in notes)] if notes else None,
            'duration_beats_median': durs[len(durs) // 2] if durs else None,
            'duration_beats_quartiles': [durs[len(durs) // 4], durs[3 * len(durs) // 4]] if durs else None,
            'bars': nb, 'D6_repeat_share_by_period_bars': period,
            'four_bar_windows_changed_share': sum(changed) / len(changed) if changed else None}


def arp(m):
    ppq, notes = m['ppq'], m['notes']
    on = sorted({n['tick'] for n in notes})
    iois = [b - a for a, b in zip(on, on[1:])]
    six = ppq // 4
    return {'onsets': len(on), 'D5_sixteenth_ioi_share': sum(1 for x in iois if x == six) / len(iois) if iois else None,
            'ioi_histogram_in_16ths': dict(sorted(Counter(round(x / six, 2) for x in iois).items()))}


def chords(m):
    """Descriptive, added after D5 proved not measurable (the Arp MIDI holds the arpeggiator's input chords): harmonic
    rhythm and pitch-class sets of the chord tracks."""
    ppq, notes = m['ppq'], m['notes']
    groups = {}
    for n in notes:
        groups.setdefault(n['tick'], []).append(n)
    on = sorted(groups)
    iv = [(b - a) / (ppq * 4) for a, b in zip(on, on[1:])]
    names = ['C', 'C#', 'D', 'Eb', 'E', 'F', 'F#', 'G', 'Ab', 'A', 'Bb', 'B']
    sets = [tuple(sorted({names[n['pitch'] % 12] for n in groups[t]})) for t in on]
    durs = [max(n['dur'] for n in groups[t]) / (ppq * 4) for t in on]
    return {'chord_onsets': len(on), 'interval_bars_histogram': dict(sorted(Counter(round(x, 2) for x in iv).items())),
            'duration_bars_histogram': dict(sorted(Counter(round(x, 2) for x in durs).items())),
            'distinct_pitch_class_sets': len(set(sets)),
            'pitch_class_set_histogram': {' '.join(k): v for k, v in Counter(sets).most_common()},
            'first_onset_bar': on[0] / (ppq * 4) if on else None}


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--midi', required=True)
    ap.add_argument('--dump', action='store_true', help='print the pitch histograms only (to map the drum pitches)')
    a = ap.parse_args()
    d = Path(a.midi)
    files = {p.name: read_smf(p) for p in sorted(d.glob('*.mid'))}
    out = {'files': [summary(k, v) for k, v in files.items()]}
    if not a.dump:
        ks = files.get('Kick & Snare.mid')
        if ks:
            hist = Counter(n['pitch'] for n in ks['notes'])
            kick = {p for p in hist if p in (35, 36)}
            snare = {p for p in hist if 37 <= p <= 40}
            out['drums'] = drums(ks, kick, snare)
        for name in ('808_Bass.mid', 'SubBass.mid'):
            if name in files:
                out[name.replace('.mid', '')] = bass(files[name])
        if 'Arp.mid' in files:
            out['arp'] = arp(files['Arp.mid'])
            out['arp_chords'] = chords(files['Arp.mid'])
        if 'Strings.mid' in files:
            out['strings_chords'] = chords(files['Strings.mid'])
        # Sensitivity of D1/D3 with the snare restricted to pitch 38 (37 is a side stick in General MIDI)
        if ks:
            out['drums_snare38_only'] = {k: v for k, v in drums(ks, kick, {38}).items()
                                         if k.startswith('D') or k in ('snare_hits', 'snare_bars')}
    print(json.dumps(out, indent=1))


if __name__ == '__main__':
    main()
