# CYMATICA — Stato milestone

Record di esecuzione richiesto da `AGENTS.md` §5.1. La roadmap autoritativa resta nella specifica (§32); questo file registra solo stato, decisioni ed evidenze.

## Stato corrente

| Campo | Valore |
|---|---|
| Milestone attiva | **M2 — Shading e pipeline visiva reattiva** (M1 completata) |
| Stato M0 | **Completata (tutti i criteri di accettazione verificati con evidenze)** |
| Stato M1 | **Completata (tutti i criteri di accettazione verificati con evidenze)** |
| Specifica | 0.8.2 (0.8.1 accettata il 2026-10-05; aggiunte System 1 del 2026-10-06, applicabili da M6) |
| Design | 0.3 (identità visiva e firme invarianti approvate il 2026-10-06) |
| Target prodotti | `cymatica_audio` (static lib), `cymatica_core` (static lib), `cymatica_replay` (static lib), `cymatica_game` (app), `cymatica_tests` (test runner) |
| Ultimo aggiornamento | 2026-10-06 |

## Decisioni d'ingresso M0

| ID | Decisione | Stato | Record |
|---|---|---|---|
| D-M0-01 | Build Tools 2026 Release, MSVC 14.50/14.51, Ninja, CMake ≥ 3.25 | **validated** | [ADR-0001](adr/0001-m0-toolchain-dependencies.md) |
| D-M0-02 | raylib 6.0 `dbc56a8…`, `SUPPORT_MODULE_RAUDIO=OFF`, `SUPPORT_CUSTOM_FRAME_CONTROL=OFF` | **validated** | ADR-0001 |
| D-M0-03 | miniaudio 0.11.25 `9634bed…` vendored, unico owner audio in `cymatica_audio` | **validated** | ADR-0001 |
| D-M0-04 | Catch2 v3.16.0 `317ac1e…` + CTest | **validated** | ADR-0001 |
| D-M0-05 | Libreria JSON: nlohmann/json 3.11.3 commit `9cca280a4d0ccf0c08f47a99aa71d1b0e52f8d03` | **validated** | ADR-0002 |
| D-M0-06 | CI Windows | open (non blocca M0/M1 locale) | spec §32 |

## Criteri di accettazione M0 (Spec §32)

| Criterio | Esito | Evidenza |
|---|---|---|
| Clean checkout configurabile e compilabile da comandi documentati | **PASSATO** | Verificato su clone fresco temporaneo da commit `65dc087`: configurazione, build da zero di tutti i 144 target ed esecuzione test superata tramite `scripts/build.ps1 -Test`. |
| `ctest` esegue e supera almeno un test reale | **PASSATO** | Catch2 test reali eseguiti con successo. |
| Eseguibile avviabile e chiudibile senza crash | **PASSATO** | `-Run -SmokeSeconds 3`, exit 0. |
| Tono udibile e shader di prova visibile | **PASSATO** | Tono sinusoidale 220 Hz udito e cambio a 440 Hz confermato. Shader Chladni a ~60 FPS su RTX 3060. |
| Unico `MINIAUDIO_IMPLEMENTATION` e unico device | **PASSATO** | `dumpbin /symbols`: 0 `ma_device_init` in `raylib.lib`, 1 definizione in `cymatica_audio.lib`. |
| Dipendenze e licenze censite | **PASSATO** | `docs/dependencies.md`. |
| Decisioni M0 registrate | **PASSATO** | ADR-0001. |
| Nessun tool futuro richiesto | **PASSATO** | Nessun ML, ONNX, FFmpeg o Android. |

## Decisioni d'ingresso M1

| ID | Decisione | Stato | Record |
|---|---|---|---|
| D-M1-01 | Profilo temporale: timeline interna 48 kHz, 120 Hz simulazione (400 campioni/tick esatti), max catch-up 4 tick, sospensione tecnica oltre 16 tick debito | **validated** | [ADR-0002](adr/0002-m1-timing-random-concurrency.md) |
| D-M1-02 | RNG deterministico e stream isolation: Stafford Mix 13 (SplitMix64 finalizer), `rng_version=1`, domain separation (3/4/5 tuple), `PolicyVersionId` esplicito | **validated** | ADR-0002 |
| D-M1-03 | Concorrenza real-time e scambio senza allocazioni: `TripleBuffer<T>` wait-free packed-CAS (telemetria/controllo), `SpscQueue<T>` FIFO per comandi e ack | **validated** | ADR-0002 |
| D-M1-04 | Serializzazione standard e contratti minimi: nlohmann/json v3.11.3 commit ufficiale, `RunRecord` round-trip JSON, schema isolato in `engine/replay` | **validated** | ADR-0002 |
| D-M1-05 | Player baseline contract: congelato in `docs/player_contract_m1.md` per validatore M4 ($R_{hit}=3.0$ px, $V_{max}=240$ px/s, dash $96$ px / cooldown $60$ ticks) | **validated** | ADR-0002, `docs/player_contract_m1.md` |

## Criteri di accettazione M1 (Spec §32)

| Criterio | Esito | Evidenza |
|---|---|---|
| `MusicClock` sample-based esatto | **PASSATO** | Timeline a 48 kHz con BPM razionale (`numerator`/`denominator`), derivazione esatta beat/bar/tempo senza accumulo di float. Test 1h virtuale a BPM non intero (127.5 BPM) con 0 drift verificato (`test_music_clock.cpp`). Resampling 44.1↔48 kHz con fattore razionale esatto 160/147 verificato. |
| Fixed-step accumulator 120 Hz | **PASSATO** | Formula `(k * 48000) / 120 = 400` campioni/tick esatta su 1h virtuale. Feed a 60 FPS e 120 FPS generano sequenze identiche di 240 tick in 2.0s (`test_fixed_step.cpp`). Catch-up limitato a 4 tick per update senza scartare tick autoritativi; segnalazione `overloadSuspensionRequired` su accumulo >16 tick. |
| Generatore deterministico e stream isolation | **PASSATO** | Golden vector Stafford Mix 13 convalidati. Consumo intensivo dello stream `VFX` non altera la sequenza generata dagli stream `Pattern` o `Director` a parità di seme (`test_random.cpp`). Distribuzione uniforme e intervalli testati. |
| Concorrenza lock-free e scambio thread sicuro | **PASSATO** | `TripleBuffer` wait-free (packed state atomic con slot front/back/shared mutuamente esclusivi) testato con 50.000 scritture/letture concorrenti ad alta frequenza: 0 frame corrotti/lacerati (tearing = 0, reads > 0). `SpscQueue` FIFO testata con 10.000 elementi tra thread produttore/consumatore senza perdite (`test_exchange.cpp`). Zero allocazioni su callback audio. |
| Canale Game-Audio e telemetria | **PASSATO** | Invio comandi `AudioCommand` e ricezione esplicita di `AudioCommandAck` verificati. Telemetria audio pubblicata tramite `TripleBuffer` con `transportEpoch`, `renderCursor` e `presentationCursor` precisi (`test_exchange.cpp`). |
| `RunRecord` JSON round-trip | **PASSATO** | Struttura serializzata in JSON standard con campi deterministici (semi, policy, durate) e deserializzata con verifica di uguaglianza identica (`test_run_record.cpp`). |
| Contratto giocatore minimale | **PASSATO** | Documentato e congelato in `docs/player_contract_m1.md` come baseline autoritativa per il validatore M4. |
| Suite di test CTest 100% superata | **PASSATO** | 19/19 test superati in Release e Debug; 18/18 superati in modalità headless (esclusi test su periferica audio reale). |
| Eseguibile `cymatica_game` funzionante | **PASSATO** | Smoke run di 3 secondi completato con successo: ~60 FPS video, 359 tick di simulazione autoritativa (120 Hz), ~146.400 frame audio renderizzati, shader Chladni reattivo, chiusura pulita exit code 0. |

## Ambiente di sviluppo

- Windows x64, NT 10.0.26300; AMD Ryzen 7 7700X; NVIDIA RTX 3060 (driver 617.42, OpenGL 3.3).
- MSVC 14.51.36231 (Build Tools 2026), CMake 4.1.2-msvc8, Ninja 1.12.1.
- miniaudio WASAPI 48 kHz, 2 canali f32.
- Frame timing: 60 FPS nominale render + 120 Hz simulazione fixed-step.

## Milestone successiva

**M2 — Shading e pipeline visiva reattiva** (Spec §32): pipeline render a doppio buffer/offscreen con raylib, shader nodali Chladni multipass / reattivi all'audio telemetrico, interpolazione dello stato tra tick autoritativi per rendering fluido a framerate arbitrario, profili prestazionali.
