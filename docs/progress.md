# CYMATICA — Stato milestone

Record di esecuzione richiesto da `AGENTS.md` §5.1. La roadmap autoritativa resta nella specifica (§32); questo file registra solo stato, decisioni ed evidenze.

## Stato corrente

| Campo | Valore |
|---|---|
| Milestone attiva | **M1 — independent review remediation** (remediation DIF-M1-15...25 completata; pronta per re-review M1 closure) |
| Stato M0 | **Completata (tutti i criteri di accettazione verificati con evidenze)** |
| Stato M1 | **Remediated & Verified (33/33 test passati; zero warning C4324/W4; zero alloc RT)** |
| Specifica | 0.8.2 (0.8.1 accettata il 2026-10-05; aggiunte System 1 del 2026-10-06, applicabili da M6) |
| Design | 0.3 (identità visiva e firme invarianti approvate il 2026-10-06) |
| Target prodotti | `cymatica_audio` (static lib), `cymatica_core` (static lib), `cymatica_replay` (static lib), `cymatica_game` (app), `cymatica_tests` (test runner) |
| Ultimo aggiornamento | 2026-10-07 |

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
| D-M1-05 | Player baseline contract: congelato in `docs/player_contract_m1.md` per validatore M4 ($R_{hit}=3.0$ px, $V_{max}=240$ px/s, dash $96$ px / cooldown $60$ ticks) | **accepted for implementation** | ADR-0002, `docs/player_contract_m1.md` |

## Criteri di accettazione M1 (Spec §32)

| Criterio | Esito | Evidenza |
|---|---|---|
| `MusicClock` sample-based esatto | **PASSATO** | Timeline a 48 kHz con BPM razionale (`numerator`/`denominator`), derivazione esatta beat/bar/tempo senza accumulo di float. Confini esatti in O(1) verificati per ogni beat $b \in [0, 68]$ a 127.5 BPM (`test_music_clock.cpp`). `setTempoMap` restituisce esito esplicito `[[nodiscard]] -> bool` respingendo configurazioni non valide o non supportate (DIF-M1-22). Test 1h virtuale a BPM non intero (127.5 BPM) con 0 drift verificato. Resampling 44.1↔48 kHz con fattore razionale esatto 160/147 verificato. |
| Fixed-step accumulator 120 Hz | **PASSATO** | Formula `(k * 48000) / 120 = 400` campioni/tick esatta. Mappatura sample-frame su tick a divisione intera superiore $\lceil (F \cdot H)/S \rceil = (F + 399) / 400$ verificata su confini e punti interni (0, 1, 200, 399, 400, 401, 799, 800, 801, 48000) (DIF-M1-15). Accumulo del resto in nanosecondi in `advanceNs` con zero drift su 60h virtuali e `interpolationAlpha` clampato strettamente a $[0.0f, 1.0f)$ anche durante backlog (DIF-M1-20). Catch-up limitato a 4 tick per update senza scartare tick autoritativi; segnalazione `overloadSuspensionRequired` oltre 16 tick con gestione a runtime (riconciliazione transport a render cursor, invalidazione `pureSeedEligible = false`, contatore sospensioni) (DIF-M1-21). |
| Generatore deterministico e stream isolation | **PASSATO** | Golden vector esatti invarianti per Stafford Mix 13, `stableMix3`, `stableMix4`, `stableMix5`, `deriveStreamSeed` e `sampleU64` convalidati. Formula di Lemire corretta senza overflow signed su `randomRangeI32`; rigetto isolato tramite `retryTag` senza mutazione di `sampleIndex` (`test_random.cpp`). Test di uniformità Chi-quadro su `randomRangeI32` ($p=0.001$, $\chi^2 < 36.12$). Consumo intensivo dello stream `VFX` non altera la sequenza generata dagli stream `Pattern` o `Director` a parità di seme. |
| Concorrenza lock-free e scambio thread sicuro | **PASSATO** | `TripleBuffer` wait-free con CAS bounded (max 4 tentativi) testato con 50.000 scritture/letture concorrenti ad alta frequenza: 0 frame corrotti/lacerati (tearing = 0, reads > 0). `SpscQueue` FIFO testata con 10.000 elementi tra thread senza perdite; `size()` safe contro unsigned modular wrap. Data race su `framesRendered` / `renderCursor` eliminata con release/acquire atomici (DIF-M1-16). `AudioControlFrame` preserva parametri invariati tra slot tramite master snapshot (DIF-M1-17). Backpressure comandi e acks completi senza perdite garantiti (DIF-M1-18). Macchina a stati transport (`Running`, `Pausing`, `Paused`, `Resuming`) Spec §20.8 implementata e verificata (DIF-M1-19). Zero allocazioni heap nella callback audio realtime verificate tramite tracker di allocazione dedicato (DIF-M1-25). `static_assert` per tipi lock-free verificata. |
| Canale Game-Audio e telemetria | **PASSATO** | Invio comandi `AudioCommand` e ricezione esplicita di `AudioCommandAck` verificati con contatore di ack scartati (`droppedAcks`). Telemetria audio pubblicata tramite `TripleBuffer` con `sequenceNumber` (rilevamento drop), `transportEpoch`, `renderCursor` e `presentationCursor` tracciati coerentemente sulla timeline logica interna a 48 kHz anche con device a 44.1 kHz (`test_exchange.cpp`). |
| `RunRecord` JSON round-trip | **PASSATO** | Struttura serializzata in JSON standard con campi deterministici (semi, policy, durate) e campo `final_metrics` (Spec §15.7), deserializzata con verifica di uguaglianza identica (`test_run_record.cpp`). Validazione fail-fast secondo Spec §23.4 (schema_version 1, rng_version 1, consistenza hex/u64, timing_profile obbligatorio e valori finiti) verificata con test dedicati (DIF-M1-23). |
| Contratto giocatore minimale | **PASSATO** | Documentato e congelato in `docs/player_contract_m1.md` come baseline autoritativa per il validatore M4; stato contrattuale formalizzato come `accepted for implementation` per tuning v0 (DIF-M1-24). |
| Suite di test CTest 100% superata | **PASSATO** | 33/33 test superati in Release e Debug; 32/32 superati in modalità headless (escluso test su periferica audio reale). |
| Eseguibile `cymatica_game` funzionante | **PASSATO** | Smoke run di 3 secondi completato con successo: ~60 FPS video, 359 tick di simulazione autoritativa (120 Hz), ~147.360 frame audio renderizzati, shader Chladni reattivo, pure_seed=ELIGIBLE, suspensions=0, chiusura pulita exit code 0. |

## Remediation Independent Review (DIF-M1-15 – DIF-M1-25)

| ID | Descrizione Defetto | Intervento | Stato / Evidenza |
|---|---|---|---|
| DIF-M1-15 | `sampleFrameToTick` divideva senza arrotondamento all'eccesso per campioni interni al tick | Implementata formula $\lceil (F \cdot H) / S \rceil = (F + 399) / 400$ per $F > 0$ in `engine/core/fixed_step.cpp`. Aggiunti unit test per frame 0, 1, 200, 399, 400, 401, 799, 800, 801, 48000. | **Risolto** (`test_fixed_step.cpp`) |
| DIF-M1-16 | Data race su `renderCursor` letto da thread game in `framesRendered()` senza sincronizzazione atomica | Convertito `renderCursor` in `std::atomic<std::uint64_t>` con release store nella callback audio e acquire load in `framesRendered()`. Reso atomico anche `deviceStarted`. | **Risolto** (`engine/audio/audio_engine.cpp`) |
| DIF-M1-17 | `AudioControlFrame` per frequenza/tono sovrascriveva volume/gain allo slot corrente | Introdotto snapshot master consolidato `masterControl_` in `AudioEngine::Impl` che preserva i parametri invariati tra slot del triple buffer. Aggiunto test dedicato di persistenza parametri. | **Risolto** (`test_exchange.cpp`) |
| DIF-M1-18 | Acknowledgment comandi audio scartati silenziosamente con `ackQueue` a 16 slot | Estesa capacità di `commandQueue` e `ackQueue` a 128 slot. Implementata contropressione `inFlightCommands_` su `sendCommand` per garantire consegna deterministica al 100% di tutti gli ack senza drop. | **Risolto** (`test_exchange.cpp`) |
| DIF-M1-19 | Macchina a stati del transport assente in `AudioEngine` (Spec §20.8) | Introdotto enum `TransportState` (`Running`, `Pausing`, `Paused`, `Resuming`). In stato `Paused`, la callback genera silenzio e congela il cursore; `transportEpoch` incrementa sui cambi di stato. | **Risolto** (`test_exchange.cpp`) |
| DIF-M1-20 | Resto nanosecondi in `FixedStepAccumulator` non accumulava frazioni; `interpolationAlpha` poteva raggiungere o superare 1.0f durante backlog | Aggiunto accumulo del resto razionale in `advanceNs` (zero drift su 60h virtuali). Applicato clamp stretto a `[0.0f, 1.0f)` tramite `std::nextafter(1.0f, 0.0f)`. Test backlog e 60h aggiunti. | **Risolto** (`test_fixed_step.cpp`) |
| DIF-M1-21 | Segnale `overloadSuspensionRequired` non gestito da loop principale a runtime | Gestito segnale in `apps/cymatica_game/main.cpp`: riconciliazione transport via `reconcileToTick()`, invalidamento `pureSeedEligible = false`, incremento `technicalSuspensionCount`, visualizzazione HUD. | **Risolto** (`apps/cymatica_game/main.cpp`) |
| DIF-M1-22 | `MusicClock::setTempoMap` falliva silenziosamente o permetteva configurazioni invalide | `setTempoMap` marcato `[[nodiscard]] -> bool`, respinge denominatori a 0, BPM non positivi, `timeSignatureNum != beatsPerBar` e configurazioni non 4/4 per prototipo M1. Test dedicati aggiunti. | **Risolto** (`test_music_clock.cpp`) |
| DIF-M1-23 | `RunRecord::fromJson` accettava JSON arbitrari senza validazione schema e non serializzava `TimingProfile` | Aggiunto `TimingProfile` in `RunRecord`. Implementata validazione fail-fast secondo Spec §23.4 (schema_version 1, rng_version 1, congruenza seme esadecimale e uint64, float finiti). | **Risolto** (`test_run_record.cpp`) |
| DIF-M1-24 | D-M1-05 marcata "validated" prima dei test del validatore M4 | Aggiornato stato contrattuale di D-M1-05 a `accepted for implementation` in `docs/player_contract_m1.md`, `ADR-0002` e `progress.md`. | **Risolto** (`docs/player_contract_m1.md`) |
| DIF-M1-25 | Assenza di test sul divieto di allocazioni dinamiche nella callback audio | Implementato hook globale di allocazione `operator new/delete` con `AllocationScope` e test dedicato che verifica 0 allocazioni su 100 blocchi consecutivi di `processBlock()`, con self-test anti-tautologia. | **Risolto** (`test_exchange.cpp`) |

## Ambiente di sviluppo

- Windows x64, NT 10.0.26300; AMD Ryzen 7 7700X; NVIDIA RTX 3060 (driver 617.42, OpenGL 3.3).
- MSVC 14.51.36231 (Build Tools 2026), CMake 4.1.2-msvc8, Ninja 1.12.1.
- miniaudio WASAPI 48 kHz, 2 canali f32.
- Frame timing: 60 FPS nominale render + 120 Hz simulazione fixed-step.

## Milestone successiva

**Milestone 2 — Music Intent e musica procedurale v1** (Spec §32): prima forma musicale simbolica condivisa (form/phrase skeleton, 4 ruoli musicali Pulso/Corpo/Trama/Vettore, ritmi euclidei, scale/modi, memoria di motivo minima, `MusicIntentEvent`, scheduler ahead-of-time, synth/mix base, telemetria coerente).

### Requisiti e decisioni d'ingresso M2

| ID | Requisito / Decisione | Stato | Dettagli |
|---|---|---|---|
| D-M2-01 | **Canale thread-safe `SetTempoMap`** | **Prerequisito d'ingresso obbligatorio** | In M1 `AudioEngine` impiega una `MusicClock` interna a 120 BPM fissi. Per consentire a M2 di gestire sezioni musicali multiple, variazioni di tempo e time signature controllate da `MusicIntentEvent` e scheduler, è necessario un canale formale da Game/Coordinator verso `AudioEngine`.<br>• **Protocollo:** estensione di `AudioCommand` con comando `SetTempoMap` (oppure buffer lock-free dedicato `TripleBuffer<TempoMap>` / SPSC) contenente numeratore, denominatore e metrica.<br>• **Sincronizzazione:** applicazione sample-accurate all'inizio del blocco o all'epoch/frame designato con emissione di `AudioCommandAck`.<br>• **Invariante:** zero allocazioni dinamiche e assenza di lock bloccanti nella callback audio realtime, con aggiornamento atomico e coerente della telemetria senza salti o discontinuità di fase. |
| D-M2-02 | Rappresentazione simbolica `MusicIntentEvent` | Open (M2 entry) | Definizione della struttura per i 4 ruoli (Pulso, Corpo, Trama, Vettore) e parametri di intensità/tensione (Spec §16, §20.5). |
| D-M2-03 | Voice budget e DSP scheduling | Open (M2 entry) | Lookahead ahead-of-time (100–250 ms), limite polifonia per voce e policy di saturazione (Spec §20.7). |
