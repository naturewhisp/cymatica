# Release-to-attack envelope slope ratio rho = (e0 / R) / (1 / A) per role (dossier A §6.4), linear ADSR of
# VoiceAllocator; Pulso level at NoteOff (half step) from its decay. Standard library only.
FS = 48000
ROLES = {'Corpo': (0.015, 0.75), 'Trama': (0.020, 0.65), 'Vettore': (0.005, 0.85)}
for name, (attack_s, sustain) in ROLES.items():
    a = 1 / (attack_s * FS)
    print(name, ' '.join('%dms:%.2f' % (ms, (sustain / (ms / 1000 * FS)) / a) for ms in (2, 15, 30, 50, 80)))
for bpm, timbre in ((120, 0.40), (140, 0.40)):
    step = 60 / bpm / 4 * FS
    attack, decay = FS / 1000, (0.05 + 0.07 * timbre) * FS
    level = max(0.0, 1 - (step / 2 - attack) / decay)
    print('Pulso', bpm, 'level at NoteOff %.3f' % level, 'rho 2ms %.3f' % ((level / 96) / (1 / attack)))
