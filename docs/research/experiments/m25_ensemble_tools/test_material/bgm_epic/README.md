# bgm_epic — materiale di collaudo per la conversione MIDI

- **Uso:** solo per collaudare lo script di conversione MIDI → JSON dello strumento d'insieme (M2.5.1c), quando arriverà la melodia F7. **Non è F7 e non si misura:** non è scritto da una persona verificabile ed è una trascrizione automatica da audio.
- **Fornito dal titolare il 2026-11-12.** Uso interno al progetto; non è un asset del gioco e non va distribuito.

## Provenienza

1. Audio sorgente: `app/assets/audio/bgm_epic.wav` del repository locale `aura` del titolare, al commit `c68aaf3`; 36.357.292 byte, SHA-256 `104c6e932e1aedfcf00fffd61c37800c00546a6c728bbcb6b7339300c21d0d3b`.
2. Caricato dal titolare su Suno, che ha generato gli stem e un MIDI per stem (archivio `bgm_epic MIDI.zip`, SHA-256 `d20400e4a0c2e23a10f7d622f88cf8eb9ec611a476534eba76204efc3d2d0a59`, file datati 2026-11-12 19:55).
3. I sette file `.mid` di questa cartella sono quelli dell'archivio, copiati senza modifiche.

| File | SHA-256 (primi 16) |
|---|---|
| `bgm_epic (Backing Vocals).mid` | `a24aed2fee1a0673` |
| `bgm_epic (Bass).mid` | `7f5b7b9ddb782b05` |
| `bgm_epic (Guitar).mid` | `693ffdd06e462b3f` |
| `bgm_epic (Keyboard).mid` | `0d28e361f6b83b95` |
| `bgm_epic (Synth).mid` | `2c7c95d8dbe8ea02` |
| `bgm_epic (Vocals).mid` | `cc37aaf9a715a5da` |
| `bgm_epic (Woodwinds).mid` | `813245b4283faf5d` |

## Caratteristiche misurate

Con [`midi_inspect.py`](../../midi_inspect.py), lettore SMF minimo scritto per questo scopo; output in [`midi_inspect_bgm_epic.output.txt`](../../midi_inspect_bgm_epic.output.txt).

- Formato 1, 480 tick per quarto, 16 battute (4 frasi da 4 battute).
- **Tempo misurato, non scritto:** la mappa del tempo varia fra 119,78 e 119,90 BPM.
- **Attacchi poco prima della griglia:** 238, 478, 717, 957 tick invece di 240, 480, 720, 960; solo il 2–6 % degli attacchi cade entro 0,02 sedicesimi dalla griglia.
- **Stem e strumenti non corrispondenti:** «Guitar» è un Alto Sax; «Vocals» e «Backing Vocals» contengono Synth Choir e una parte di batteria con note di 0–1 tick.

Sono i casi che la conversione deve gestire senza errori silenziosi: più tracce, mappa del tempo variabile, note di durata nulla, quantizzazione al sedicesimo.
