# CYMATICA — Stato milestone

Record di esecuzione richiesto da `AGENTS.md` §5.1. La roadmap autoritativa resta nella specifica (§32); questo file registra solo stato, decisioni ed evidenze.

## Stato corrente

| Campo | Valore |
|---|---|
| Milestone attiva | **M2 — Music Intent e musica procedurale v1** (M1 completata e validata da independent review) |
| Stato M0 | **Completata (tutti i criteri di accettazione verificati con evidenze)** |
| Stato M1 | **Completata (tutti i criteri di accettazione verificati con evidenze post-remediation round 2)** |
| Specifica | 0.8.3 (baseline 0.8.1/0.8.2 accettata; controllo/osservazione agentica documentati il 2026-10-07) |
| Design | 0.3.1 (baseline identità 0.3 approvata il 2026-10-06; osservazione agentica documentata il 2026-10-07) |
| Commit / Build verificato | Commit `4db4b1b` (MSVC /W4 0 warning, CTest Release 36/36, Debug 36/36, Headless 35/35, Smoke 3s exit 0) |
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
| `MusicClock` sample-based esatto | **PASSATO** | Timeline a 48 kHz con BPM razionale (`numerator`/`denominator`), derivazione esatta beat/bar/tempo senza accumulo di float. Confini esatti in O(1) verificati per ogni beat $b \in [0, 68]$ a 127.5 BPM (`test_music_clock.cpp`). `setTempoMap` e factory statica `create` restituiscono esito esplicito respingendo configurazioni non valide, rate zero o non supportate (DIF-M1-22). Protezione totale divide-by-zero su conversioni frame e coordinate. Test 1h virtuale a BPM non intero (127.5 BPM) con 0 drift verificato. Resampling 44.1↔48 kHz con fattore razionale esatto 160/147 verificato. |
| Fixed-step accumulator 120 Hz | **PASSATO** | Formula `(k * 48000) / 120 = 400` campioni/tick esatta. Mappatura sample-frame su tick a divisione intera superiore $\lceil (F \cdot H)/S \rceil = (F + 399) / 400$ verificata su confini e punti interni (0, 1, 200, 399, 400, 401, 799, 800, 801, 48000) (DIF-M1-15). Derivazione boundary razionale assoluto $\lfloor ((T + 1) \cdot S - 1) / 10^9 \rfloor$ con zero drift su 60h virtuali e `interpolationAlpha` clampato strettamente a $[0.0f, 1.0f)$ anche durante backlog (DIF-M1-20). Conservazione resto frazionario nanosecondi in `advanceSeconds()`, gestione input negativi/non-finiti/saturazione uint64. Catch-up limitato a 4 tick per update senza scartare tick autoritativi; segnalazione `overloadSuspensionRequired` oltre 16 tick con gestione a runtime (riconciliazione transport a render cursor, invalidazione `pureSeedEligible = false`, contatore sospensioni) (DIF-M1-21). |
| Generatore deterministico e stream isolation | **PASSATO** | Golden vector esatti invarianti per Stafford Mix 13, `stableMix3`, `stableMix4`, `stableMix5`, `deriveStreamSeed` e `sampleU64` convalidati. Formula di Lemire corretta senza overflow signed su `randomRangeI32`; rigetto isolato tramite `retryTag` senza mutazione di `sampleIndex` (`test_random.cpp`). Test di uniformità Chi-quadro su `randomRangeI32` ($p=0.001$, $\chi^2 < 36.12$). Consumo intensivo dello stream `VFX` non altera la sequenza generata dagli stream `Pattern` o `Director` a parità di seme. |
| Concorrenza lock-free e scambio thread sicuro | **PASSATO** | `TripleBuffer` wait-free con CAS bounded (max 4 tentativi) testato con 50.000 scritture/letture concorrenti ad alta frequenza: 0 frame corrotti/lacerati (tearing = 0, reads > 0). `SpscQueue` FIFO testata con 10.000 elementi tra thread senza perdite; `size()` safe contro unsigned modular wrap. Data race su `framesRendered` / `renderCursor` eliminata con release/acquire atomici (DIF-M1-16). `AudioControlFrame` preserva parametri invariati tra slot tramite master snapshot (DIF-M1-17). Backpressure comandi e acks completi senza perdite garantiti (DIF-M1-18). Macchina a stati transport (`Running`, `Pausing`, `Paused`, `Resuming`, `Stopped`) Spec §20.8 implementata e verificata con rigetto esplicito dei salti non ammessi (DIF-M1-19). Zero allocazioni heap nella callback audio realtime verificate tramite tracker di allocazione standard, aligned e CRT debug hook (DIF-M1-25). `static_assert` per tipi lock-free verificata. |
| Canale Game-Audio e telemetria | **PASSATO** | Invio comandi `AudioCommand` e ricezione esplicita di `AudioCommandAck` verificati con contatore di ack scartati (`droppedAcks`). Telemetria audio pubblicata tramite `TripleBuffer` con `sequenceNumber` (rilevamento drop), `transportEpoch`, `renderCursor` e `presentationCursor` tracciati coerentemente sulla timeline logica interna a 48 kHz anche con device a 44.1 kHz (`test_exchange.cpp`). |
| `RunRecord` JSON round-trip | **PASSATO** | Struttura serializzata in JSON standard con campi deterministici (semi, policy, durate) e campo `final_metrics` (Spec §15.7), deserializzata con verifica di uguaglianza identica (`test_run_record.cpp`). Validazione fail-fast secondo Spec §23.4 (schema_version 1, rng_version 1, consistenza hex/u64, timing_profile obbligatorio e valori finiti, divisibilità intera sample_rate / simulation_hz) verificata con test dedicati (DIF-M1-23). |
| Contratto giocatore minimale | **PASSATO** | Documentato e congelato in `docs/player_contract_m1.md` come baseline autoritativa per il validatore M4; stato contrattuale formalizzato come `accepted for implementation` per tuning v0 (DIF-M1-24). |
| Suite di test CTest 100% superata | **PASSATO** | 36/36 test superati in Release e Debug; 35/35 superati in modalità headless (escluso test su periferica audio reale). |
| Eseguibile `cymatica_game` funzionante | **PASSATO** | Smoke run di 3 secondi completato con successo: ~60 FPS video, 356 tick di simulazione autoritativa (120 Hz), ~145.920 frame audio renderizzati, shader Chladni reattivo, pure_seed=ELIGIBLE, suspensions=0, chiusura pulita exit code 0. |

## Remediation Independent Review (DIF-M1-15 – DIF-M1-25)

| ID | Descrizione Defetto | Intervento | Stato / Evidenza |
|---|---|---|---|
| DIF-M1-15 | `sampleFrameToTick` divideva senza arrotondamento all'eccesso per campioni interni al tick | Implementata formula $\lceil (F \cdot H) / S \rceil = (F + 399) / 400$ per $F > 0$ in `engine/core/fixed_step.cpp`. Aggiunti unit test per frame 0, 1, 200, 399, 400, 401, 799, 800, 801, 48000. | **Risolto** (`test_fixed_step.cpp`) |
| DIF-M1-16 | Data race su `renderCursor` letto da thread game in `framesRendered()` senza sincronizzazione atomica | Convertito `renderCursor` in `std::atomic<std::uint64_t>` con release store nella callback audio e acquire load in `framesRendered()`. Reso atomico anche `deviceStarted`. | **Risolto** (`engine/audio/audio_engine.cpp`) |
| DIF-M1-17 | `AudioControlFrame` per frequenza/tono sovrascriveva volume/gain allo slot corrente | Introdotto snapshot master consolidato `masterControl_` in `AudioEngine::Impl` che preserva i parametri invariati tra slot del triple buffer. Aggiunto test dedicato di persistenza parametri. | **Risolto** (`test_exchange.cpp`) |
| DIF-M1-18 | Acknowledgment comandi audio scartati silenziosamente con `ackQueue` a 16 slot | Estesa capacità di `commandQueue` e `ackQueue` a 128 slot. Implementata contropressione `inFlightCommands_` su `sendCommand` per garantire consegna deterministica al 100% di tutti gli ack senza drop. | **Risolto** (`test_exchange.cpp`) |
| DIF-M1-19 | Macchina a stati del transport assente o permissiva su salti diretti (Spec §20.8) | Introdotto enum `TransportState` (`Running`, `Pausing`, `Paused`, `Resuming`, `Stopped`). In stato `Paused`/`Stopped`, la callback genera silenzio e congela il cursore. Irrigidito `isValidTransportTransition`: rifiutati i salti diretti `Running <-> Paused`, transizione obbligata via `Pausing`/`Resuming`; transizioni verso `Stopped` consentite. I comandi non conformi vengono rigettati con `Rejected`. | **Risolto** (`test_exchange.cpp`) |
| DIF-M1-20 | Resto nanosecondi in `FixedStepAccumulator` non sincronizzato sui boundary razionali assoluti (Blocker) | Riscritto l'accumulatore su tempo cumulativo autoritativo assoluto $T$ (`totalElapsedNs_`): target tick derivato razionalmente in $O(1)$ come $\lfloor ((T + 1) \cdot S - 1) / 10^9 \rfloor$. `interpolationAlpha` calcolato su nanosecondi nel tick corrente e clampato a $[0.0f, 1.0f)$ con `std::nextafter`. `advanceSeconds()` conserva il resto frazionario in nanosecondi e gestisce input negativi, non-finiti e saturazione uint64. Verificata equivalenza esatta 2.0s (240 tick) a 60 e 120 FPS; verificato caso limite 240 delta nominali (1.999.999.920 ns) = 239 tick esatti. Zero drift su 60h. | **Risolto** (`test_fixed_step.cpp`) |
| DIF-M1-21 | Segnale `overloadSuspensionRequired` non registrato nel `RunRecord` | Implementato `RunRecord::recordIntervention(sampleFrame, reason, tick)` che aggiunge la `RuntimeIntervention` e imposta `pureSeedEligible = false`. Integrato in `apps/cymatica_game/main.cpp` all'occorrenza di sospensione tecnica. | **Risolto** (`apps/cymatica_game/main.cpp`, `test_run_record.cpp`) |
| DIF-M1-22 | `MusicClock` costruttore falliva silenziosamente su configurazioni invalide e `deviceRate == 0` trattato come identità | Introdotta factory statica `MusicClock::create(sampleRate, tempoMap)` che restituisce `std::nullopt` su configurazioni non valide, non 4/4 o con `sampleRate == 0`. Costruttore di default configurato a 48 kHz 120 BPM 4/4 valido. Conversioni frame e derivazioni coordinate protette da divisioni per zero, restituiscono 0 esplicito se `deviceRate == 0` o `sampleRate == 0`. | **Risolto** (`engine/audio/music_clock.cpp`, `test_music_clock.cpp`) |
| DIF-M1-23 | `RunRecord` non totalmente fail-fast: mancava validazione unificata in `toJson()` e per coerenza `timing_profile` | Centralizzato metodo `RunRecord::validate(std::string* error)` invocato sia all'inizio di `toJson()` sia al termine di `fromJson()`. Verifica schema/rng version, float finiti in `final_metrics` (`std::isfinite`), divisibilità esatta `sample_rate % simulation_hz == 0` e consistenza semantica `frames_per_tick == sample_rate / simulation_hz`. Gestione fail-fast con `std::runtime_error` su stringhe hex invalide e tipi non numerici. | **Risolto** (`engine/replay/run_record.cpp`, `test_run_record.cpp`) |
| DIF-M1-24 | D-M1-05 marcata "validated" prima dei test del validatore M4 | Aggiornato stato contrattuale di D-M1-05 a `accepted for implementation` in `docs/player_contract_m1.md`, `ADR-0002` e `progress.md`. | **Risolto** (`docs/player_contract_m1.md`) |
| DIF-M1-25 | Assenza di test su aligned allocations e heap CRT per divieto allocazioni callback audio | Esteso tracker globale con overload di `operator new/delete` aligned C++17 (`std::align_val_t`) e hook CRT debug `_CrtSetAllocHook` testato direttamente contro allocazioni raw `std::malloc/free`. Documentato audit statico delle funzioni miniaudio (`ma_waveform_read_pcm_frames`) e lock-free invocate nella callback: 0 allocazioni post-init su 100 blocchi verificati. | **Risolto** (`test_exchange.cpp`) |

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

## Estensione documentale — controllo e osservazione agentica (2026-10-07)

Richiesta del titolare: analisi permanente e sviluppo progressivo insieme al gioco, incluso spazio/cambio tono sulla baseline M1 e osservazione temporale/live nelle fasi frenetiche. Record: [analisi](agent_control_observability.md), specifica §31.5/§32 e DESIGN §17.4.1.

- **Milestone attiva invariata: M2.** M1 rimane conclusa secondo le evidenze storiche; nessuna nuova esecuzione o rivalidazione in questa revisione documentale.
- **Nuovo lavoro M2 non eseguito:** AC-AO-01A (baseline tono, mandatory) / AC-AO-01B (ascolto agente, capability-dependent supplementare) e AC-AO-02 (tap Music Intent), decisioni D-AO-01/02 deferred; profilo `tone_probe_v0` proposed. Non è ancora disponibile un input/capture adapter per l'agente.
- **Successivi incrementi non eseguiti:** sequenze/input M3, replay/clip M4, live read-only e latenza M5, spike video M6, valutazione percettiva/streaming M7, riuso Lab M8. I relativi gate sono in specifica §32.
- **Verifica di questa revisione:** ispezione del percorso spazio e della telemetria nei sorgenti M1, controlli documentali di link locali, coerenza degli ID/gate e `git diff --check`. Build/test runtime non rieseguiti: nessun sorgente o build configuration modificato. Non costituisce chiusura di milestone.
- **Prossima azione:** congelare D-AO-01/02 con capacità client, ingressi, buffer/lifecycle e profilo tono, poi implementare il runner minimo M2; prove OS e ascolto restano esplicitamente separate dal test semantico.

### Correzioni post-review del commit 1a88bc8

- **DIF-AO-DOC-01 risolto nella documentazione:** D-AO-01 include il dispatcher comune minimo del tono prima di AC-AO-01A in M2; D-AO-03 estende lo stesso nucleo in M3. Nessun dispatcher implementato o decisione d'ingresso congelata in questa correzione.
- **DIF-AO-DOC-02 risolto nella documentazione:** specifica §31.5/§32 distingue AC-AO-01A mandatory e AC-AO-01B supplementare dipendente dal client; stessa separazione per immagini/video e streaming successivi. I gate del progetto, report/benchmark e playtest originali restano obbligatori; assenza di capacità esterna documentata non equivale a PASS o difetto risolto.
- **Chiarimento LOW:** il frame statico M2 è opzionale, fuori dal gate tono e dal contratto VisualProbe M3.
- **Verifica:** controllo link locali, riferimenti e diff; nessuna build/test runtime rieseguita perché cambiano solo documenti. M2 resta attiva; prossimo task congelare D-AO-01/02 prima di implementare AC-AO-01A.
