# float32 value of MusicPosition::phrasePhase at the last frame of a 16-beat phrase (reconciliation §5, AG-MM-02)
import struct


def f32(x):
    return struct.unpack('f', struct.pack('f', x))[0]


NEXT_BELOW_ONE = f32(1.0 - 2**-24)
for bpm in (300, 120, 60, 20, 5, 1.4):
    dur = 60 * 48000 / bpm
    beat_phase = min(f32((dur - 1) / dur), NEXT_BELOW_ONE)
    print('BPM %6.1f -> phrasePhase at the last frame = %.9f' % (bpm, f32(f32(15.0 + beat_phase) / 16.0)))
