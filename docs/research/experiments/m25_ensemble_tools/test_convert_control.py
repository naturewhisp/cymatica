"""Checks of convert_control.py (M2.5.1c): python test_convert_control.py, from this folder."""
import json
import os
import sys
import tempfile

from convert_control import ConversionError, convert, note_to_midi

HERE = os.path.dirname(os.path.abspath(__file__))
results = []


def check(name, condition):
    results.append(bool(condition))
    print(('ok   ' if condition else 'FAIL ') + name)


def fails(fn):
    try:
        fn()
    except ConversionError:
        return True
    return False


def table(text):
    f = tempfile.NamedTemporaryFile('w', suffix='.csv', delete=False, encoding='utf-8')
    f.write(text)
    f.close()
    return f.name


check('C4 is MIDI 60', note_to_midi('C4') == 60)
check('sharps and flats', note_to_midi('F#4') == 66 and note_to_midi('Bb3') == 58 and note_to_midi('A4') == 69)
check('invalid note name fails', fails(lambda: note_to_midi('H4')))

ok = convert(table('frase, inizio, nota, durata\n1, 0, D4, 4\n1, 4, F4, 4\n2, 0, D4, 2\n'))
check('table: two phrases', [len(p) for p in ok['phrases']] == [2, 1])
check('table: values kept', ok['phrases'][0][1] == {'onset': 4, 'midi': 65, 'duration': 4})
check('table: one phrase with notes fails', fails(lambda: convert(table('frase, inizio, nota, durata\n1, 0, D4, 4\n'))))
check('table: bad header fails', fails(lambda: convert(table('a, b, c, d\n1, 0, D4, 4\n2, 0, D4, 4\n'))))
check('table: onset out of the phrase fails', fails(lambda: convert(table('frase, inizio, nota, durata\n1, 64, D4, 4\n2, 0, D4, 4\n'))))
check('table: zero duration fails', fails(lambda: convert(table('frase, inizio, nota, durata\n1, 0, D4, 0\n2, 0, D4, 4\n'))))

top = convert(table('frase, inizio, nota, durata\n1, 0, D4, 4\n1, 0, A4, 4\n2, 0, D4, 4\n'))
check('simultaneous notes: the highest is kept', top['phrases'][0] == [{'onset': 0, 'midi': 69, 'duration': 4}])
check('simultaneous notes: counted in the report', top['conversion_report']['dropped_lower_simultaneous'] == 1)
clip = convert(table('frase, inizio, nota, durata\n1, 60, D4, 8\n2, 0, D4, 4\n'))
check('a note crossing the phrase end is clipped', clip['phrases'][0][0]['duration'] == 4)
check('clipping counted in the report', clip['conversion_report']['clipped_at_phrase_end'] == 1)

f8 = convert(os.path.join(HERE, 'controls', 'F8_bach_bwv1002_borea_double_violin.mid'))
r8 = f8['conversion_report']
check('F8: meter 2/2 read from the file, phrase of 64 sixteenths', r8['meter'] == '2/2' and r8['phrase_steps'] == 64)
check('F8: 17 phrases, all with notes', r8['phrases'] == 17 and r8['phrases_with_notes'] == 17)
check('F8: engraved score, nothing moved to the grid', r8['onsets_moved_to_grid'] == 0)
check('F8: 13 lower notes of double stops dropped', r8['dropped_lower_simultaneous'] == 13)

bgm = convert(os.path.join(HERE, 'test_material', 'bgm_epic', 'bgm_epic (Vocals).mid'))
rb = bgm['conversion_report']
check('bgm_epic: transcription moved to the grid and reported', rb['onsets_moved_to_grid'] > 100)
check('bgm_epic: zero-length notes dropped and reported', rb['dropped_zero_length'] == 1)
check('bgm_epic: 4 phrases only', rb['phrases'] == 4)

f9 = convert(os.path.join(HERE, 'controls', 'F9_subagent_table.csv'))
check('F9: 24 phrases, 436 notes, nothing changed', f9['conversion_report']['phrases'] == 24 and
      sum(len(p) for p in f9['phrases']) == 436 and f9['conversion_report']['dropped_lower_simultaneous'] == 0)

check('not a MIDI file fails', fails(lambda: convert(table('not, a, midi, table\n').replace('.csv', '.csv'))))
bad = tempfile.NamedTemporaryFile('wb', suffix='.mid', delete=False)
bad.write(b'RIFF0000')
bad.close()
check('a non-SMF .mid fails', fails(lambda: convert(bad.name)))

json.dumps(f8)  # serialisable
print(f'{sum(results)}/{len(results)} checks passed')
sys.exit(0 if all(results) else 1)
