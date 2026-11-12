"""Minimal SMF reader for inspection: notes, tempo, timing grid regularity. Reads files only, executes nothing."""
import collections
import struct
import sys


def vlq(b, i):
    v = 0
    while True:
        c = b[i]; i += 1
        v = (v << 7) | (c & 0x7F)
        if not c & 0x80:
            return v, i


def read(path):
    b = open(path, 'rb').read()
    assert b[:4] == b'MThd'
    fmt, ntrk, div = struct.unpack('>HHH', b[8:14])
    i = 14
    notes, tempos, meta = [], [], []
    for _ in range(ntrk):
        assert b[i:i + 4] == b'MTrk', 'bad track'
        ln = struct.unpack('>I', b[i + 4:i + 8])[0]
        j, end, t, status = i + 8, i + 8 + ln, 0, 0
        on = {}
        while j < end:
            d, j = vlq(b, j); t += d
            c = b[j]
            if c == 0xFF:
                typ = b[j + 1]; ln2, k = vlq(b, j + 2); data = b[k:k + ln2]; j = k + ln2
                if typ == 0x51: tempos.append((t, int.from_bytes(data, 'big')))
                elif typ in (1, 3, 4): meta.append((typ, data.decode('latin-1')))
                elif typ == 0x58: meta.append(('timesig', tuple(data)))
                continue
            if c in (0xF0, 0xF7):
                ln2, k = vlq(b, j + 1); j = k + ln2; continue
            if c & 0x80:
                status = c; j += 1
            hi = status & 0xF0
            if hi in (0xC0, 0xD0):
                j += 1; continue
            d1, d2 = b[j], b[j + 1]; j += 2
            if hi == 0x90 and d2 > 0:
                on[d1] = (t, d2)
            elif hi in (0x80, 0x90) and d1 in on:
                s, v = on.pop(d1); notes.append((s, t - s, d1, v))
        i = end
    return fmt, ntrk, div, sorted(notes), tempos, meta


for path in sys.argv[1:]:
    fmt, ntrk, div, notes, tempos, meta = read(path)
    name = path.replace('\\', '/').split('/')[-1]
    print(f'== {name}: format {fmt}, tracks {ntrk}, ticks/quarter {div}, notes {len(notes)}')
    print('   meta', meta[:6], 'tempo', [(t, round(60e6 / us, 2)) for t, us in tempos[:3]])
    if not notes:
        continue
    six = div / 4
    off = [abs(s / six - round(s / six)) for s, _, _, _ in notes]
    on_grid = sum(1 for x in off if x < 0.02) / len(notes)
    vel = collections.Counter(v for *_, v in notes)
    poly = sum(1 for a, b_ in zip(notes, notes[1:]) if b_[0] < a[0] + a[1]) / max(1, len(notes) - 1)
    span = (notes[-1][0] + notes[-1][1]) / div / 4
    print(f'   on the 16th grid {on_grid:.0%}, distinct velocities {len(vel)}, overlap with next {poly:.0%}, '
          f'length {span:.1f} bars, pitch {min(n[2] for n in notes)}-{max(n[2] for n in notes)}')
    print('   first notes (tick, dur, midi, vel):', notes[:8])
