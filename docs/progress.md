# CYMATICA — Stato milestone

Record di esecuzione richiesto da `AGENTS.md` §5.1. La roadmap autoritativa resta nella specifica (§32); questo file registra solo stato, decisioni ed evidenze.

## Stato corrente

| Campo | Valore |
|---|---|
| Milestone attiva | **M2 — Music Intent e musica procedurale v1**; M2.1/M2.2/M2.3 concluse, **M2.4 prossima**, M2.5 ancora da verificare. M2 complessiva non conclusa. |
| Stato M0 | **Completata (tutti i criteri di accettazione verificati con evidenze)** |
| Stato M1 | **Completata: review indipendente OK, zero difetti residui, gate round 3 passati** |
| Specifica | Versione **0.8.3** con nota integrativa di ricerca M2.4 del 2026-10-13 (§16.12–16.13 e §32); nessuna nuova decisione artistica/DSP approvata dalla sola ricerca. |
| Design | 0.3.1 (baseline identità 0.3 approvata il 2026-10-06; osservazione agentica documentata il 2026-10-07) |
| Evidenza storica M1 | `b39bddb46449ee0b203c7234746185ba241d9a42`; Release 41/41, Debug 41/41, Headless 40/40, smoke exit 0, `/W4` zero warning; [evidenze](m1_review_round3.md). Questi risultati appartengono al gate M1. |
| Ultima CI Headless verificata | Commit documentale [`02885b699402cbabc8ef1cd6877de24c9ff0a1a9`](https://github.com/naturewhisp/cymatica/commit/02885b699402cbabc8ef1cd6877de24c9ff0a1a9): [GitHub Actions run #38395937263](https://github.com/naturewhisp/cymatica/actions/runs/38395937263) PASS — Windows/MSVC Release 74/74, Debug 74/74, CACP `tone_test` PASS; non sostituisce collaudo hardware o giudizio artistico. |
| Integrazione documentale M2.4 | Nello stesso commit `02885b6994`: ricerca revisionata, piano/protocollo sperimentale, addendum proposto per milestone future e raccordo nella specifica; implementazione M2.4 non ancora attestata. |
| Target prodotti | `cymatica_audio` (static lib), `cymatica_core` (static lib), `cymatica_replay` (static lib), `cymatica_game` (app), `cymatica_tests` (test runner) |
| Ultimo aggiornamento | 2026-10-13 (riallineamento metadati e documenti, senza nuove prove hardware) |

## Decisioni d'ingresso M0

| ID | Decisione | Stato | Record |
|---|---|---|---|
| D-M0-01 | Build Tools 2026 Release, MSVC 14.50/14.51, Ninja, CMake ≥ 3.25 | **validated** | [ADR-0001](adr/0001-m0-toolchain-dependencies.md) |
| D-M0-02 | raylib 6.0 `dbc56a8…`, `SUPPORT_MODULE_RAUDIO=OFF`, `SUPPORT_CUSTOM_FRAME_CONTROL=OFF` | **validated** | ADR-0001 |
| D-M0-03 | miniaudio 0.11.25 `9634bed…` vendored, unico owner audio in `cymatica_audio` | **validated** | ADR-0001 |
| D-M0-04 | Catch2 v3.16.0 `317ac1e…` + CTest | **validated** | ADR-0001 |
| D-M0-05 | Libreria JSON: nlohmann/json 3.11.3 commit `9cca280a4d0ccf0c08f47a99aa71d1b0e52f8d03` | **validated** | ADR-0002 |
| D-M0-06 | CI Windows: pipeline GitHub Actions (`.github/workflows/windows-ci.yml`, runner `windows-2025`, matrice Release/Debug Headless, test CACP offline) | **validated** | [docs/ci.md](ci.md), `.github/workflows/windows-ci.yml` |

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
| Suite di test CTest 100% superata | **PASSATO** | Round 3: 41/41 Release e Debug, 40/40 headless, commit `b39bddb`; [evidenze](m1_review_round3.md). |
| Eseguibile `cymatica_game` funzionante | **PASSATO** | Round 3: smoke 3 s exit 0, 356 tick, 145.920 frame audio, shader/audio OK, ELIGIBLE, zero sospensioni; [evidenze](m1_review_round3.md). |

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

### Sottofasi operative M2

Suddivisione richiesta dal titolare il 2026-10-07; roadmap e criteri autoritativi in specifica §32, «Sottofasi operative M2». Nessuna sottofase completata o nuova evidenza runtime attestata da questa modifica documentale. Stato complessivo invariato: M2 attiva.

| Sottofase | Stato | Evidenze / prossimo passo |
|---|---|---|
| M2.1 — Decisioni e contratti | **Completata** | Congelate D-M2-01, D-M2-02, D-M2-03, D-AO-01, D-AO-02 in [ADR-0003](adr/0003-m2-decisions-contracts.md); contratti di transport, intent, scheduling, dispatcher del tono e AudioProbe accettati. |
| M2.2 — Controllo e osservazione del tono | **Completata (remediation validata)** | AC-AO-01A superato (modalità offline `initOffline` priva di callback fisica WASAPI concorrente, quiescence garantita su `detachAudioProbe`, manifest JSON completo con correlation fields ed export gate vincolante, frequenze 220 $\to$ 440 $\to$ 220 Hz $\pm 2$ Hz verificate, prova fisica OS SPACE documentata distintamente dallo smoke sintetico); AC-AO-01B superato (capacità multimodale verificata su modello `antigravity-gemini-2.5-pro`, artefatto WAV `0A1F68A...` ascoltato e giudizio percettivo registrato in [m2_tone_probe_evidence.md](m2_tone_probe_evidence.md)). |
| M2.3 — Transport e scheduler | **Completata (remediation validata)** | Canale `SetTempoMap` sample-accurate con trattenimento comandi futuri fino al blocco di competenza (`peek`), ancore `TempoSegment` per continuità esatta di battute/fasi senza salti (DIF-M2-02). Applicazione degli eventi `ScheduledAudioEvent` agli offset campionari esatti intra-blocco (`[sample_accurate]`), congelamento del consumo durante `Paused`/`Stopped` (`[pause]`), conversione coerente coordinate 44.1/48 kHz (DIF-M2-03). Metrica musicale razionale esatta per 3/8 (36.000 frame/battuta a 120 BPM), 6/8, 5/4, 2/2 (DIF-M2-04). Protezione overflow uint64 in `scheduleIntent`. Ordinamento causale intra-blocco comandi/eventi, quantizzazione comandi tempo a confini musicali, gestione 3-way degli epoch, conservazione voci attive con micro-fadeout anti-click (DIF-M2-R4-01) e budget deterministico hard-realtime per callback (DIF-M2-R4-02). 75/75 test superati (Release, Debug), 74/74 Headless. |
| M2.4 — Musica procedurale v1 | **Prossimo passo** | Generatori procedurali dei 4 ruoli (Pulso, Corpo, Trama, Vettore), pattern euclidei puri di Bjorklund, synth polifonico a 8 voci e due sezioni musicali con transizione tempo/metrica. |
| M2.5 — Integrazione e verifica finale | **Da eseguire** | AC-AO-02, criteri originali M2 e review indipendente iterativa; record di build/profilo/procedura ed evidenza per ogni mandatory. |

La tabella traccia l'esecuzione e rinvia alla specifica per dipendenze e criteri. Aggiornare gli stati solo su evidenze: il passaggio interno non chiude M2 o autorizza M3. Nessun gate esistente viene rimosso; le capacità esterne supplementari restano distinte dai mandatory del progetto.

### Requisiti e decisioni d'ingresso M2

| ID | Requisito / Decisione | Stato | Dettagli |
|---|---|---|---|
| D-M2-01 | **Canale thread-safe `SetTempoMap`** | **accepted for implementation** | Estensione `AudioCommand` con tipo `SetTempoMap`, numeratore/denominatore BPM e metrica compattati, applicazione sample-accurate all'inizio del blocco o frame designato, incremento `transportEpoch`, `AudioCommandAck` esplicito e zero RT allocazioni ([ADR-0003](adr/0003-m2-decisions-contracts.md)). |
| D-M2-02 | Rappresentazione simbolica `MusicIntentEvent` | **accepted for implementation** | Struttura simbolica per i 4 ruoli (Pulso, Corpo, Trama, Vettore) con griglia ritmica euclidea pura (Bjorklund statico preallocato) e parametri di intensità/tensione ([ADR-0003](adr/0003-m2-decisions-contracts.md)). |
| D-M2-03 | Voice budget e DSP scheduling | **accepted for implementation** | Finestra ahead-of-time (100–250 ms, nominale 150 ms = 18 tick), `SpscQueue<ScheduledAudioEvent, 256>` con Coordinator unico producer, budget 8 voci polifoniche (2 per ruolo), voice-stealing con micro-fadeout di 2 ms ([ADR-0003](adr/0003-m2-decisions-contracts.md)). |
| D-AO-01 | Nucleo dispatcher comune del tono (`InputAction`/`ActionState`) | **accepted for implementation** | Dispatcher comune per SPACE fisico e runner diagnostico CACP M2; normalizzazione su `masterControl_` in Coordinator thread; mutua esclusione e runner locale a scenario finito ([ADR-0003](adr/0003-m2-decisions-contracts.md)). |
| D-AO-02 | Tap AudioProbe e profilo `tone_probe_v0` | **accepted for implementation** | Ring buffer SPSC lock-free (24.000 campioni stereo f32 = 0.5 s), zero allocazioni e zero lock nel callback audio, drop counter atomico; profilo 3.0 s (220 -> 440 -> 220 Hz $\pm 2$ Hz); separazione AC-AO-01A (mandatory) e AC-AO-01B (supplementare) ([ADR-0003](adr/0003-m2-decisions-contracts.md)). |

## Estensione documentale — controllo e osservazione agentica (2026-10-07)

Richiesta del titolare: analisi permanente e sviluppo progressivo insieme al gioco, incluso spazio/cambio tono sulla baseline M1 e osservazione temporale/live nelle fasi frenetiche. Record: [analisi](agent_control_observability.md), specifica §31.5/§32 e DESIGN §17.4.1.

- **Milestone attiva invariata: M2.** M1 rimane conclusa secondo le evidenze storiche; nessuna nuova esecuzione o rivalidazione in questa revisione documentale.
- **Nuovo lavoro M2 non eseguito:** AC-AO-01A (baseline tono, mandatory) / AC-AO-01B (ascolto agente, capability-dependent supplementare) e AC-AO-02 (tap Music Intent), decisioni D-AO-01/02 deferred; profilo `tone_probe_v0` proposed. Non è ancora disponibile un input/capture adapter per l'agente.
- **Successivi incrementi non eseguiti:** sequenze/input M3, replay/clip M4, live read-only e latenza M5, spike video M6, valutazione percettiva/streaming M7, riuso Lab M8. I relativi gate sono in specifica §32.
- **Verifica di questa revisione:** ispezione del percorso spazio e della telemetria nei sorgenti M1, controlli documentali di link locali, coerenza degli ID/gate e `git diff --check`. Build/test runtime non rieseguiti: nessun sorgente o build configuration modificato. Non costituisce chiusura di milestone.
- **Prossima azione:** congelare D-AO-01/02 con capacità client, ingressi, buffer/lifecycle e profilo tono, poi implementare il runner minimo M2; prove OS e ascolto restano esplicitamente separate dal test semantico.

### Riferimento Dreamforge — integrazione documentale

Confronto richiesto dal titolare il 2026-10-07, riferimento GitHub `71210140df8861745d210e9ee84d726355d143ea`. Specifica §31.6 e [analisi §13](agent_control_observability.md) distinguono invarianti di parità/control-data plane dalle evoluzioni proposed (bundle, evaluation result, development episode) M3/M4 e riuso M7/M8. Nessun nuovo deliverable M2, dipendenza o schema implementato; M2.1–M2.5 e decisioni ancora aperte invariati. Verifica documentale di link, coerenza dei rimandi e diff; build/test runtime non eseguiti per modifica solo documentale.

### Correzioni post-review del commit 1a88bc8

- **DIF-AO-DOC-01 risolto nella documentazione:** D-AO-01 include il dispatcher comune minimo del tono prima di AC-AO-01A in M2; D-AO-03 estende lo stesso nucleo in M3. Nessun dispatcher implementato o decisione d'ingresso congelata in questa correzione.
- **DIF-AO-DOC-02 risolto nella documentazione:** specifica §31.5/§32 distingue AC-AO-01A mandatory e AC-AO-01B supplementare dipendente dal client; stessa separazione per immagini/video e streaming successivi. I gate del progetto, report/benchmark e playtest originali restano obbligatori; assenza di capacità esterna documentata non equivale a PASS o difetto risolto.
- **Chiarimento LOW:** il frame statico M2 è opzionale, fuori dal gate tono e dal contratto VisualProbe M3.
- **Verifica:** controllo link locali, riferimenti e diff; nessuna build/test runtime rieseguita perché cambiano solo documenti. M2 resta attiva; prossimo task congelare D-AO-01/02 prima di implementare AC-AO-01A.

## Riapertura M1 — review del titolare (2026-10-07)

La review del codice `4db4b1b06342080c04ff99939bef73e9f282e129` conclude **NON OK**: DIF-M1-26 blocker realtime, DIF-M1-27 overflow, DIF-M1-28 parsing seed. Revocata l'attestazione di chiusura precedente; le dichiarazioni M1 conclusa/M2 attiva sopra descrivono soltanto lo stato storico ora superato. Nessun lavoro M2 implementato in questa remediation.

Correzioni round 3 implementate, decisioni D-M1-06/07/08 in ADR-0002. Gate e review indipendente **da eseguire** sul nuovo codice; i PASS storici non attestano questa revisione.

Review indipendente iterazione 1: NON OK per DIF-M1-29 (MusicClock), DIF-M1-30 (narrowing JSON), DIF-M1-31 (encoding/stato). Tutti corretti nella stessa remediation; iterazione 2 statica senza difetti residui, gate finali in corso. Un primo golden test MusicClock aveva un errore di trascrizione del valore atteso, corretto con oracle Python a precisione arbitraria prima dei gate finali.

### Chiusura rivalidata round 3

Commit runtime `b39bddb46449ee0b203c7234746185ba241d9a42`: DIF-M1-26–31 risolti. Review indipendente finale **OK**, zero difetti residui, zero warning /W4, Release 41/41, Debug 41/41, headless 40/40, smoke 3 s exit 0. [Record dettagliato](m1_review_round3.md) con dimensioni, comandi, ambiente e hash log. I precedenti stati NON OK/da eseguire descrivono le iterazioni storiche, superate da questo gate. M1 nuovamente chiusa; M2 attiva, sottofasi ancora da eseguire. Player tuning D-M1-05 resta accepted for implementation, validazione M4/M5 differita; nessun nuovo blocco M1.

---

## Remediation M2 Review (DIF-M2-01 – DIF-M2-07)

Revisione indipendente dei commit M2.1–M2.3 conclusa con richiesta di remediation (DIF-M2-01..07). Interventi applicati e validati:

| ID | Severità | Descrizione Defetto | Intervento Applicato | Stato / Evidenza |
|---|---|---|---|---|
| DIF-M2-01 | **BLOCKER** | `runToneProbeV0Offline()` chiamava `startTone()`, avviando il device WASAPI in background e generando race condition realtime con il runner e `AudioProbe`. | Implementato `AudioEngine::initOffline()`: sintesi interamente in memoria senza dispositivo né callback WASAPI. Aggiunto tracking di quiescenza `probeActiveWriters_` con `_mm_pause()` in `detachAudioProbe()`. | **Risolto** (`audio_engine.cpp`, `audio_probe.cpp`, test CACP) |
| DIF-M2-02 | **BLOCKER** | `SetTempoMap` ignorava `targetSampleFrame`; la sostituzione della mappa produceva salti di fase. | In `processBlock()` comandi con `targetSampleFrame` futuro sono trattenuti via `peek()`. In `MusicClock` introdotte ancore `TempoSegment` per continuità esatta di battute, misure e fase. | **Risolto** (`music_clock.cpp`, `audio_engine.cpp`, `test_music_clock.cpp`, `test_scheduler.cpp`) |
| DIF-M2-03 | **BLOCKER** | Eventi schedulati consumati senza applicarli agli offset esatti; consumo consentito durante `Paused`; coordinate 44.1/48 kHz mescolate. | Slicing intra-blocco dei blocchi audio per eventi `ScheduledAudioEvent` con applicazione campionaria esatta del DSP; congelamento totale del consumo durante `Paused`/`Stopped`; conversione coerente coordinate via `MusicClock`. | **Risolto** (`audio_engine.cpp`, `test_scheduler.cpp`) |
| DIF-M2-04 | **HIGH** | `MusicClock::frameAtBar()` ignorava il denominatore: 3/8 calcolato a 72.000 frame anziché 36.000. | Applicato fattore metrico $\times 4 / \text{timeSignatureDen}$ per semiminime al minuto (Spec §7.5). Golden test convalidati per 3/8 (36.000 frame), 6/8 (72.000 frame), 5/4 (120.000 frame), 2/2 (96.000 frame). | **Risolto** (`music_clock.cpp`, `test_music_clock.cpp`) |
| DIF-M2-05 | **HIGH** | Il gate AC-AO-01A confondeva lo smoke sintetico con la consegna fisica degli eventi da parte dell'OS. | Documentata separazione esplicita tra smoke test sintetico CI e procedura di verifica interattiva con tasto fisico SPAZIO (Win32/raylib). | **Risolto** (`docs/m2_tone_probe_evidence.md`) |
| DIF-M2-06 | **HIGH** | Manifest CACP privo di correlazione prevista; verdetto PASS anche se export falliva. | `ToneProbeResult::toJson()` esteso con `build_id`, `transport_epoch`, `simulation_ticks`, `requested_sample_frames`, `applied_sample_frames`, `presentation_quality`. `passed` condizionato obbligatoriamente al successo della scrittura degli artefatti (`wavWriteSuccess && manifestWriteSuccess`). | **Risolto** (`audio_probe.h`, `audio_probe.cpp`, test CLI) |
| DIF-M2-07 | **MEDIUM** | Skill `cacp-testing` assumeva capacità multimodali non garantite senza tracciare fallback normativo, hash e modello. | Aggiornata la skill con fallback a `unsupported`/`blocked`; registrati modello `antigravity-gemini-2.5-pro`, capacità `audio/wav`, hash SHA-256 (`0A1F68A...`) e timestamp. | **Risolto** (`.agents/skills/cacp-testing/SKILL.md`, `docs/m2_tone_probe_evidence.md`) |
| DIF-M2-08 | **HIGH** | Violazione del vincolo perentorio "Zero warning /W4": variabili inutilizzate `totalBeats` e `kSimulationHz` generavano `C4189`. | Rimossa `kSimulationHz` in `audio_probe.cpp`; `totalBeats` integrato nel calcolo di `beatInBar` in `music_clock.cpp`. Compilazione pulita da zero (`-Clean`) verificata su Release e Debug con zero warning. | **Risolto** (`audio_probe.cpp`, `music_clock.cpp`) |
| DIF-M2-09 | **MEDIUM** | Race condition nel protocollo di quiescenza in modalità online: l'annuncio `fetch_add(1)` avveniva dopo la lettura del puntatore. | Applicato pattern di annuncio simmetrico sequential consistency: `fetch_add(1, memory_order_seq_cst)` prima di `audioProbe.load(memory_order_seq_cst)` in `processBlock`; corrispondente `store(nullptr, seq_cst)` e loop di quiescenza con `_mm_pause()` in `detachAudioProbe()`. | **Risolto** (`audio_engine.cpp`) |
| DIF-M2-10 | **LOW** | Disallineamento metrico intra-misura in `MusicClock::positionAtFrame` e `frameAtBar`: `beatInBar` ed `elapsedBars` ignoravano l'offset iniziale `startBeat`. | Introdotto calcolo continuo di `totalBars` ed `elapsedBeatsWithOffset` basato su `(startBeat % beatsPerBar)`; `frameAtBar` esteso per supportare segmenti che iniziano intra-misura. Aggiunto unit test dedicato `[intra_measure]`. | **Risolto** (`music_clock.cpp`, `test_music_clock.cpp`) |
| Secondari | **LOW** | Troncamento con `& 0xFF` in `makeSetTempoMap()` e overflow uint64 in `scheduleIntent()`. | Campi estesi a 16-bit (`0xFFFF`) per `subdivisionsPerBeat` e `barsPerPhrase`; protezione saturante contro overflow uint64 su `sampleFrame + durationFrames`. | **Risolto** (`audio_control.h`, `audio_scheduler.cpp`) |

---

## Remediation M2 Review — Round 3 (Findings 1–7)

Risoluzione dei 7 difetti identificati nella terza revisione indipendente di M2 (commit `0ec9d02`):

| ID / Problema | Severità | Descrizione Defetto | Intervento Applicato | Stato / Evidenza |
|---|---|---|---|---|
| Problema 1 | **ALTO** | Falso ACK: `setTempoMap()` falliva al 17º segmento ma `AudioEngine` emetteva `Applied` e incrementava `transportEpoch`. | `executeCommand()` verifica il ritorno di `setTempoMap()`: se `false`, emette `Rejected` e non incrementa l'epoch. Aggiunto test per rigetto del 17º segmento. | **Risolto** (`audio_engine.cpp`, `test_scheduler.cpp`) |
| Problema 2 | **ALTO** | Transizione metrica 4/4 → 3/8 al frame 96.000 produceva battuta 1 anziché battuta 0 (`4 % 3 = 1`). | Introdotto ancoraggio metrico locale `beatOffsetInBar`: al cambio di metrica su confine di misura viene azzerato a 0, garantendo Battuta 1, Beat 0, BeatPhase 0.0. Aggiunto test `[clock][meter_change]`. | **Risolto** (`music_clock.h`, `music_clock.cpp`, `test_music_clock.cpp`) |
| Problema 3 | **MEDIO** | Discontinuità intra-beat: cambio tempo a frame frazionario (es. frame 12.000) azzerava la fase. | Adottata politica esplicita fail-fast: `setTempoMap()` rifiuta salti frazionari intra-beat (`inverse.remainder != 0`) e cambi di metrica intra-misura. | **Risolto** (`music_clock.cpp`, `test_music_clock.cpp`) |
| Problema 4 | **BLOCKER** | Head-of-line blocking sul trasporto: comandi futuri in `commandQueue` bloccavano `Pause`, `Resume` e `Stop`, causando potenziale stallo permanente. | Disaccoppiati i comandi schedulati futuri in buffer locale bounded `pendingScheduledCommands_` (capacità 16) in `Impl`. Comandi di controllo immediati eseguiti all'istante senza attesa. Aggiunto test `[scheduler][hol_blocking]`. | **Risolto** (`audio_engine.cpp`, `test_scheduler.cpp`) |
| Problema 5 | **ALTO** | Eventi audio non protetti da cambi di epoch: note obsolete della vecchia mappa venivano sintetizzate dopo transizione. | Aggiunto `transportEpoch` a `MusicIntentEvent` e `ScheduledAudioEvent` (default 1). Nella callback audio, gli eventi con `transportEpoch < currentEpoch` vengono scartati immediatamente senza sintesi DSP. Aggiunto test `[scheduler][epoch_drop]`. | **Risolto** (`music_intent.h`, `audio_scheduler.h`, `audio_scheduler.cpp`, `audio_engine.cpp`, `test_scheduler.cpp`) |
| Problema 6 | **ALTO** | Limiti aritmetici: overflow uint64 in `remainingBars * beatsPerBar` e `currentRenderCursor + lookaheadFrames`; troncamento anticipato a 44,1 kHz. | Introdotta `core::saturatingMul` in `unsigned_math.h`; aritmetica saturante in `frameAtBar()` e `dispatchDueEvents()`; divisione ceil intera in `internalToDeviceFrames()`. Verificato con test di saturazione. | **Risolto** (`unsigned_math.h`, `music_clock.cpp`, `audio_scheduler.cpp`, `test_music_clock.cpp`) |
| Problema 7 | **MEDIO** | Provenienza manifest CACP: `buildId` statico obsoleto, `transportEpoch` hardcoded a 1, assenza di timestamp campionari per fase. | Iniettato `CYMATICA_BUILD_ID` dinamico da Git commit hash via CMake; lettura dinamica di `transportEpoch` da telemetria; aggiunto `start_sample_frame` a `PhaseMetric` e manifest JSON. | **Risolto** (`CMakeLists.txt`, `audio_probe.h`, `audio_probe.cpp`, test CACP CLI) |

**Esito suite completa post-remediation Round 3:**
- **Release:** 67/67 superati (100%), 0 falliti.
- **Headless:** 66/66 superati (100%), 0 falliti.
- **Debug:** 67/67 superati (100%), 0 falliti.

---

## Remediation M2 Review — Round 3 Follow-up (DIF-M2-R3-01 – DIF-M2-R3-05)

Risoluzione dei 5 difetti (1 Blocker, 4 High) e dei requisiti di test/CI identificati nella revisione successiva di M2.3:

| ID | Severità | Descrizione Defetto | Intervento Applicato | Stato / Evidenza |
|---|---|---|---|---|
| DIF-M2-R3-01 | **HIGH** | `ToneProbeResult::toJson()` produceva JSON non valido su Windows per via dei backslash non escapati nei percorsi (`artifacts\cacp\...`). La CI mascherava il problema con una replace. | Implementato helper `escapeJsonString()` in `audio_probe.cpp` che esegue l'escape di `\`, `"`, `\n`, `\r`, `\t` e caratteri di controllo. Rimosso il workaround string-replace dalla CI e aggiunto test Catch2 che valida il parsing diretto via `nlohmann::json::parse()`. | **Risolto** (`audio_probe.cpp`, `test_audio_probe.cpp`, `.github/workflows/windows-ci.yml`) |
| DIF-M2-R3-02 | **BLOCKER** | Avanzamento prematuro di `transportEpoch` in `processBlock()`: consumava comandi tempo prima di processare campioni ed eventi schedulati precedenti nello stesso blocco, scartando note valide. | Riscritto il render loop con slicing temporale causale: comandi schedulati ed eventi audio vengono interleavati e consumati in ordine cronologico intra-blocco. L'epoch avanza solo all'effettivo offset campionario del comando. Aggiunto test intra-blocco con verifica zero-crossings PCM. | **Risolto** (`audio_engine.cpp`, `test_scheduler.cpp`) |
| DIF-M2-R3-03 | **HIGH** | `MusicClock::setTempoMap()` rifiutava confini di beat validi a BPM frazionari (es. 140 BPM, frame 20.571) a causa di `inverse.remainder != 0`. | Allineato il criterio di validazione: se `deltaF == boundary.value` (dove `boundary` è calcolato da `frameAtBeat`), la transizione è accettata come confine esatto. Aggiunti unit test per 140 BPM e 127.5 BPM. | **Risolto** (`music_clock.cpp`, `test_music_clock.cpp`) |
| DIF-M2-R3-04 | **HIGH** | `SetTempoMap` immediato durante riproduzione riscriveva retroattivamente il segmento 0 o falliva per frame nel passato. | Comandi immediati (`targetSampleFrame == 0`) durante la riproduzione vengono quantizzati in avanti al successivo confine musicale (`nextBeatBoundary()` o `nextBarBoundary()`); comandi passati (`targetSampleFrame < blockStartFrame`) vengono rifiutati esplicitamente con `Rejected`. | **Risolto** (`music_clock.h`, `music_clock.cpp`, `audio_engine.cpp`, `test_scheduler.cpp`) |
| DIF-M2-R3-05 | **HIGH** | Eventi con epoch futura (`schedEvt.transportEpoch > transportEpoch`) venivano consumati o scartati senza protezione causale. | Gestione causale a 3 vie: eventi con epoch passata vengono scartati; con epoch corrente vengono sintetizzati; con epoch futura vengono trattenuti fino all'offset designato e, se l'epoch non avanza all'arrivo dell'evento, scartati come non-matching incrementando `droppedScheduledEvents`. | **Risolto** (`audio_engine.cpp`, `test_scheduler.cpp`) |
| Test Quality | **MEDIUM** | Il test `[scheduler][epoch_drop]` verificava una costante di telemetria anziché il buffer PCM reale renderizzato. | Test esteso: verifica reale sui campioni audio PCM sintetizzati (conteggio zero-crossings tra frequenza base 220 Hz e nota scartata 880 Hz). | **Risolto** (`test_scheduler.cpp`) |
| CI Hygiene | **LOW** | `actions/checkout` in GitHub Actions non era vincolato a commit SHA immutabile. | Fissato `actions/checkout@3d3c42e5aac5ba805825da76410c181273ba90b1 # v7.0.1`. | **Risolto** (`.github/workflows/windows-ci.yml`) |

**Esito suite completa post-remediation Round 3 Follow-up:**
- **Release:** 73/73 superati (100%), 0 falliti.
- **Headless:** 72/72 superati (100%), 0 falliti.
- **Debug:** 73/73 superati (100%), 0 falliti.

---

## Remediation M2 Review — Round 4 (DIF-M2-R4-01 & DIF-M2-R4-02)

Risoluzione dei 2 difetti emersi dalla revisione del commit `572875a`:

| ID | Severità | Descrizione Defetto | Intervento Applicato | Stato / Evidenza |
|---|---|---|---|---|
| DIF-M2-R4-01 | **HIGH** | Nota attiva senza `NoteOff`: con cambio di epoch (es. al frame 24.000 da 1 a 2), il `NoteOff` pianificato originariamente per il frame 25.000 veniva scartato come obsoleto (`epoch < current`), lasciando la nota a suonare indefinitamente. | Introdotta policy esplicita di conservazione voce attiva: `activeNoteEpoch_` traccia l'epoch di avvio; se un `NoteOff` con epoch precedente corrisponde alla voce attiva, non viene scartato ma applicato al suo frame naturale. Introdotto micro-fadeout di 2 ms (96 campioni at 48 kHz, ADR-0003) anti-click e salvaguardia auto-release dopo 1s per voci orfane. Unit test `[scheduler][voice_epoch_lifecycle]` verifica la continuità e convalida analiticamente silenzio assoluto (0 crossing, 0.0f amp) dopo la conclusione della nota. | **Risolto** (`audio_engine.cpp`, `test_scheduler.cpp`) |
| DIF-M2-R4-02 | **HIGH** | Manca budget massimo di eventi per callback hard-realtime: `while (scheduledEventQueue.peek(schedEvt))` in presenza di producer concorrente o loop intra-blocco poteva elaborare un numero illimitato di eventi, violando il contratto realtime di CYMATICA. | Introdotto budget per callback `kMaxScheduledEventsPerBlock = 256` (pari alla capacità totale della coda SPSC), applicato anche agli eventi scartati. Gli eventi eccedenti vengono differiti al blocco successivo; contatore atomico `deferredScheduledEvents` esposto su `AudioEngine` e pubblicato in `AudioTelemetryFrame`. Unit test `[scheduler][event_budget_concurrency]` con thread producer concorrente che satura la coda a ciclo continuo e verifica tempi di callback strettamente bounded ($< 50\text{ ms}$). | **Risolto** (`audio_engine.h`, `audio_engine.cpp`, `audio_telemetry.h`, `test_scheduler.cpp`) |

**Esito suite completa post-remediation Round 4:**
- **Release:** 75/75 superati (100%), 0 falliti.
- **Headless:** 74/74 superati (100%), 0 falliti.
- **Debug:** 75/75 superati (100%), 0 falliti.
- **Smoke test interattivo (3s):** exit 0, 179 frame, 59.5 fps, 145.920 campioni audio, 358 tick, pure_seed=ELIGIBLE, 0 sospensioni, shader=OK, audio=OK.
- **CACP CLI (`cymatica_game --cacp-run tone_test`):** Verdetto PASS, 0 frame persi, 144.000 frame catturati, validazione JSON diretta nativa con path backslash escapati (`artifacts\\cacp\\...`). Zero warning `/W4`.

---

## Sottofase M2.4 — Musica procedurale v1 (Pianificazione e Contratti d'Ingresso)

**Stato sottofase:** Prossima (Pianificazione congelata con commit propedeutica; sviluppo in attesa di avvio autorizzato).

In conformità a `AGENTS.md §1.1`, Spec §32 (Milestone 2) e ADR-0003, sono formalizzati i seguenti contratti architetturali propedeutici all'avvio dell'implementazione di M2.4:

### Aspetti architetturali trasferiti da M2.3 a M2.4 e M2.5
1. **Identità delle voci e correlazione NoteOn/NoteOff (M2.4):**
   - Superamento del modello monofonico temporaneo (`activeNoteEpoch_`).
   - Implementazione del **Voice Allocator polifonico a 8 voci** preallocate (2 Pulso, 2 Corpo, 2 Trama, 2 Vettore).
   - Correlazione biunivoca tra `NoteOn` e `NoteOff` tramite `(role, eventId)` nei voice slot, impedendo che un `NoteOff` con `eventId` diverso o ritardato interrompa una nota più recente sullo stesso epoch.
2. **Ciclo di vita delle note sostenute e gestione pad di Corpo (M2.4):**
   - Superamento della salvaguardia globale monofonica a 1 secondo (48.000 campioni post-transizione epoch).
   - Gestione del ciclo di vita basata sugli stadi di inviluppo (ADSR) per voce.
   - Note e pad armonici di `Corpo` possono essere legittimamente sostenuti attraverso i confini di misura, frase ed epoch, terminando su `NoteOff` esplicito o per saturazione del budget di ruolo (voice-stealing con micro-fadeout di 2 ms anti-click).
3. **Budget computazionale e deadline realtime della callback DSP (M2.4 / M2.5):**
   - Risoluzione del divario tra la soglia di hang test ($50\text{ ms}$) e la deadline fisica audio a 48 kHz ($10,67\text{ ms}$ per 512 campioni).
   - Target di budget DSP: tempo di calcolo callback $\le 2,0\text{ ms}$ per blocco da 512 campioni ($< 20\%$ del quantum audio).
   - Verifica tramite benchmark prestazionali in M2.4 e validazione continuativa in M2.5 su 3 minuti di generazione ininterrotta senza glitch né buffer underrun.




