"""Converts a melody (MIDI file or text table) into the JSON read by ensemble_tools_v1 (M2.5.1c, F7-F9).

Usage: python convert_control.py <input.mid | input.csv> <output.json>

Rules, fixed in the pre-registration addenda and applied mechanically to the whole input:
- MIDI: every track is read; score time comes from the ticks (the tempo map is ignored); the meter is the first time
  signature of the file (4/4 if absent). Onsets and durations are rounded to the nearest sixteenth; a duration rounded
  to 0 becomes 1. Notes with zero length in ticks are dropped (percussive markers of transcriptions).
- Table (format B of the instructions): "frase, inizio, nota, durata", onset and duration in sixteenths, note names
  such as D4, F#4, Bb3 (C4 = MIDI 60).
- A phrase is 4 bars of the meter; a note belongs to the phrase of its onset and is clipped to that phrase's end.
- Where several notes start on the same sixteenth, only the highest is kept (melody line).
- Fails with an explicit error, never silently: unreadable file, no notes, a phrase longer than 64 sixteenths
  (the grid of the tool), or fewer than 2 phrases with notes.
The output also carries a report of what each rule changed.
"""
import csv
import json
import struct
import sys

NOTE_INDEX = {'C': 0, 'D': 2, 'E': 4, 'F': 5, 'G': 7, 'A': 9, 'B': 11}
MAX_PHRASE_STEPS = 64


class ConversionError(Exception):
    pass


def note_to_midi(name):
    name = name.strip()
    if len(name) < 2 or name[0].upper() not in NOTE_INDEX:
        raise ConversionError(f'invalid note name: {name!r}')
    pitch, i = NOTE_INDEX[name[0].upper()], 1
    while i < len(name) and name[i] in '#b':
        pitch += 1 if name[i] == '#' else -1
        i += 1
    try:
        octave = int(name[i:])
    except ValueError as e:
        raise ConversionError(f'invalid octave in {name!r}') from e
    midi = 12 * (octave + 1) + pitch
    if not 0 <= midi <= 127:
        raise ConversionError(f'note out of MIDI range: {name!r}')
    return midi


def _vlq(b, i):
    v = 0
    while True:
        if i >= len(b):
            raise ConversionError('truncated variable-length quantity')
        c = b[i]
        i += 1
        v = (v << 7) | (c & 0x7F)
        if not c & 0x80:
            return v, i


def read_midi(path):
    """Returns (ticks per quarter, (numerator, denominator), notes [(start_tick, length_ticks, midi)])."""
    b = open(path, 'rb').read()
    if b[:4] != b'MThd':
        raise ConversionError('not a standard MIDI file')
    _, ntrk, div = struct.unpack('>HHH', b[8:14])
    if div & 0x8000:
        raise ConversionError('SMPTE time division not supported')
    i, notes, meter = 14, [], None
    for _ in range(ntrk):
        if b[i:i + 4] != b'MTrk':
            raise ConversionError('bad track header')
        length = struct.unpack('>I', b[i + 4:i + 8])[0]
        j, end, t, status, held = i + 8, i + 8 + length, 0, 0, {}
        while j < end:
            d, j = _vlq(b, j)
            t += d
            c = b[j]
            if c == 0xFF:
                typ = b[j + 1]
                ln, k = _vlq(b, j + 2)
                if typ == 0x58 and meter is None and ln >= 2:
                    meter = (b[k], 2 ** b[k + 1])
                j = k + ln
                continue
            if c in (0xF0, 0xF7):
                ln, k = _vlq(b, j + 1)
                j = k + ln
                continue
            if c & 0x80:
                status = c
                j += 1
            hi, chan = status & 0xF0, status & 0x0F
            if hi in (0xC0, 0xD0):
                j += 1
                continue
            d1, d2 = b[j], b[j + 1]
            j += 2
            if hi == 0x90 and d2 > 0:
                held.setdefault((chan, d1), []).append(t)
            elif hi in (0x80, 0x90) and held.get((chan, d1)):
                start = held[(chan, d1)].pop(0)
                notes.append((start, t - start, d1))
        i = end
    return div, meter or (4, 4), notes


def convert_midi(path):
    div, (num, den), raw = read_midi(path)
    sixteenth = div / 4
    bar_steps = num * 16 // den
    if bar_steps * 4 > MAX_PHRASE_STEPS:
        raise ConversionError(f'meter {num}/{den}: a phrase of 4 bars is {bar_steps * 4} sixteenths, more than {MAX_PHRASE_STEPS}')
    report = {'source': 'midi', 'meter': f'{num}/{den}', 'ticks_per_quarter': div, 'notes_read': len(raw)}
    zero = [n for n in raw if n[1] == 0]
    notes = []
    off_grid = 0
    for start, length, midi in raw:
        if length == 0:
            continue
        s = round(start / sixteenth)
        off_grid += 1 if abs(start / sixteenth - s) > 1e-9 else 0
        notes.append((s, max(1, round(length / sixteenth)), midi))
    report['dropped_zero_length'] = len(zero)
    report['onsets_moved_to_grid'] = off_grid
    return notes, bar_steps * 4, report


def convert_table(path):
    notes, phrase_steps = [], 64
    with open(path, encoding='utf-8-sig', newline='') as f:
        rows = list(csv.reader(f))
    if not rows:
        raise ConversionError('empty table')
    header = [c.strip().lower() for c in rows[0]]
    if header[:4] != ['frase', 'inizio', 'nota', 'durata']:
        raise ConversionError(f'unexpected header: {rows[0]}')
    for n, row in enumerate(rows[1:], start=2):
        if not row or all(not c.strip() for c in row):
            continue
        try:
            phrase, onset, name, dur = int(row[0]), int(row[1]), row[2], int(row[3])
        except (ValueError, IndexError) as e:
            raise ConversionError(f'line {n}: {row}') from e
        if phrase < 1 or not 0 <= onset < phrase_steps or dur < 1:
            raise ConversionError(f'line {n}: value out of range {row}')
        notes.append(((phrase - 1) * phrase_steps + onset, dur, note_to_midi(name)))
    return notes, phrase_steps, {'source': 'table', 'meter': '4/4', 'notes_read': len(notes)}


def to_phrases(notes, phrase_steps, report):
    if not notes:
        raise ConversionError('no notes')
    by_onset = {}
    for onset, dur, midi in notes:
        cur = by_onset.get(onset)
        if cur is None or midi > cur[1]:
            by_onset[onset] = (dur, midi)
    report['dropped_lower_simultaneous'] = len(notes) - len(by_onset)
    phrases, clipped = {}, 0
    for onset in sorted(by_onset):
        dur, midi = by_onset[onset]
        p, local = divmod(onset, phrase_steps)
        if local + dur > phrase_steps:
            dur = phrase_steps - local
            clipped += 1
        phrases.setdefault(p, []).append({'onset': local, 'midi': midi, 'duration': min(dur, 16)})
    report['clipped_at_phrase_end'] = clipped
    report['durations_capped_at_16'] = sum(1 for o in by_onset.values() if o[0] > 16)
    count = max(phrases) + 1
    out = [phrases.get(p, []) for p in range(count)]
    if sum(1 for p in out if p) < 2:
        raise ConversionError('fewer than 2 phrases with notes')
    report['phrases'] = count
    report['phrases_with_notes'] = sum(1 for p in out if p)
    report['phrase_steps'] = phrase_steps
    return out


def convert(path):
    notes, steps, report = convert_midi(path) if path.lower().endswith(('.mid', '.midi')) else convert_table(path)
    return {'phrases': to_phrases(notes, steps, report), 'conversion_report': report}


if __name__ == '__main__':
    if len(sys.argv) != 3:
        print(__doc__)
        sys.exit(2)
    try:
        result = convert(sys.argv[1])
    except ConversionError as e:
        print(f'conversion failed: {e}', file=sys.stderr)
        sys.exit(1)
    with open(sys.argv[2], 'w', encoding='utf-8', newline='\n') as f:
        json.dump(result, f, indent=1)
    print(json.dumps(result['conversion_report']))
