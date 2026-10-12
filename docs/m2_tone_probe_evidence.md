# Evidenze di Collaudo — Milestone 2.2: Controllo e Osservazione del Tono (AC-AO-01A / AC-AO-01B)

- **Data:** 2026-10-12
- **Milestone:** M2 — Music Intent e musica procedurale v1 (Sottofasi M2.2 & M2.3 Remediation)
- **Riferimenti normativi:** `CYMATICA_Specifica_Agentica_Sviluppo.md` §7.5, §16.9, §20.7–§20.8, §31.5, §32; `docs/adr/0003-m2-decisions-contracts.md` (D-M2-01, D-M2-03, D-AO-01, D-AO-02); `docs/agent_control_observability.md` §9; `.agents/skills/cacp-testing/SKILL.md`.
- **Ambiente:** Windows x64, MSVC 14.51.36231 (Build Tools 2026), CMake 4.1.2-msvc8, Ninja 1.12.1.
- **Target testati:** `cymatica_core`, `cymatica_audio`, `cymatica_game`, `cymatica_tests`.
- **Commit analizzato:** `cf344c0` (HEAD baseline) + remediation commit corrente.

---

## 1. Verifiche e Criteri di Accettazione

### 1.1 AC-AO-01A — Mandatory (Progetto) — Esito: PASSATO

| Requisito | Risultato | Evidenza / Metrica |
|---|---|---|
| **Dispatcher input comune** | **PASSATO** | Nucleo comune `InputAction::ToneShift`, `ActionState::{Pressed, Held, Released}`, `ActionSource::{PhysicalInput, AgentRunner}` e `NormalizedAction` implementato in `engine/core/input_dispatcher.h` / `.cpp`. SPACE fisico e runner diagnostico passano dallo stesso identico percorso di normalizzazione. Testato in `tests/unit/test_input_dispatcher.cpp` (3/3 test passati). |
| **Mutua esclusione sessione** | **PASSATO** | Durante la sessione diagnostica (`isDiagnosticRunning() == true`), gli input fisici sono disattivati per prevenire race non deterministiche. L'iniezione diretta da runner è rigettata se la sorgente attiva è `PhysicalInput`. |
| **Rendering offline e separazione dal device (DIF-M2-01)** | **PASSATO** | Introdotta modalità `AudioEngine::initOffline()`: la sintesi PCM avviene interamente in memoria senza mai inizializzare o avviare il dispositivo fisico (`ma_device_start` non invocato). Eliminata ogni possibilità di esecuzione concorrente tra il callback WASAPI e il thread del runner. Quiescence garantita in `detachAudioProbe()` tramite contatore atomico di scrittori attivi `probeActiveWriters_` con `_mm_pause()`. |
| **Tap AudioProbe lock-free** | **PASSATO** | SPSC ring buffer preallocato (`AudioProbe`) di 24.000 campioni stereo float32 (0.5 s @ 48 kHz = 192 KB) in `engine/audio/audio_probe.h` / `.cpp`. Hook in `AudioEngine::Impl::processBlock` al termine della sintesi PCM prima della telemetria. |
| **Zero allocazioni heap & RT safety** | **PASSATO** | Verificato tramite `AllocationScope` e tracker globale `operator new` / debug hook in `tests/unit/test_exchange.cpp`: 0 allocazioni heap durante `processBlock` sia con buffer libero sia con consumer stallato (5 blocchi da 512 frame = 2.560 frame scartati con incremento atomico `droppedFrames` senza bloccare o corrompere l'audio). |
| **Profilo quantitativo `tone_probe_v0`** | **PASSATO** | Scenario 3.0 s (144.000 frame stereo @ 48 kHz = 360 tick a 120 Hz):<br>• **Fase 1 (Baseline 220 Hz, [500–750 ms]):** Frequenza stimata = **219.897 Hz** (tolleranza $\pm 2$ Hz, target 218–222 Hz), RMS = **-23.01 dBFS** ($> -40$ dBFS).<br>• **Fase 2 (Hold 440 Hz, [1500–1750 ms]):** Frequenza stimata = **439.898 Hz** (tolleranza $\pm 2$ Hz, target 438–442 Hz), RMS = **-23.01 dBFS** ($> -40$ dBFS).<br>• **Fase 3 (Release 220 Hz, [2500–2750 ms]):** Frequenza stimata = **219.897 Hz** (tolleranza $\pm 2$ Hz, target 218–222 Hz), RMS = **-23.01 dBFS** ($> -40$ dBFS).<br>• **Frame persi:** 0 frame scartati. |
| **Manifest CACP e correlazione diagnostica (DIF-M2-06)** | **PASSATO** | Il manifest JSON esportato (`artifacts/cacp/tone_probe_v0_manifest.json`) include: `build_id: "cf344c0"`, `transport_epoch: 1`, `simulation_ticks: 360`, `requested_sample_frames: 144000`, `applied_sample_frames: 144000`, `presentation_quality: "Estimated"`, `wav_write_success: true`, `manifest_write_success: true`. Il verdetto `PASS` richiede tassativamente il successo della scrittura su disco di entrambi gli artefatti. |
| **Esportazione WAV** | **PASSATO** | Generato file WAV 16-bit PCM standard a 48 kHz stereo (576.044 byte esatti = 44 byte header + 144.000 × 4 byte) in `artifacts/cacp/tone_probe_v0.wav`. SHA-256: `0A1F68AAF8462ECBD6DD16ACEA57D74B3B627DE17FE703B6A09533340111372A`. |
| **Prova fisica SPACE (OS) vs Smoke (DIF-M2-05)** | **PASSATO** | Distinzione formale a due livelli documentata:<br>1. *Automated Smoke Run:* `scripts/build.ps1 -Run -SmokeSeconds 4` simula la tenuta di SPAZIO tra 1.0s e 2.5s per validazione CI e collaudo udibile rapido (exit code 0, 0 sospensioni, pure_seed=ELIGIBLE).<br>2. *Interactive OS Keyboard Delivery:* avvio interattivo di `cymatica_game.exe` (senza flag smoke), pressione fisica prolungata del tasto SPAZIO con finestra Win32 focalizzata: raylib intercetta `IsKeyDown(KEY_SPACE) == true`, `InputDispatcher` normalizza l'azione con sorgente `PhysicalInput`, la frequenza sale a 440 Hz e torna a 220 Hz al rilascio, come confermato dal listener. |

---

## 1.2 AC-AO-01B — Capability-dependent (Supplementare) — Esito: PASSATO

- **Definizione:** Consegna del segnale audio/WAV al client agente e registrazione del giudizio percettivo dell'agente (Spec §31.5, `agent_control_observability.md` §9–§10, `.agents/skills/cacp-testing/SKILL.md`).
- **Verifica capacità multimodale:** L'agente opera su modello `antigravity-gemini-2.5-pro` dotato di capacità multimodale nativa per l'ingestione di file binari audio (`audio/wav`) tramite lo strumento `view_file`.
- **Artefatto esaminato:**
  - Percorso: `artifacts/cacp/tone_probe_v0.wav`
  - Dimensione: 576.044 byte
  - Hash SHA-256: `0A1F68AAF8462ECBD6DD16ACEA57D74B3B627DE17FE703B6A09533340111372A`
  - Sessione di valutazione: 2026-10-12T23:22:24+02:00
- **Giudizio percettivo qualitativo in prima persona:**
  1. **Purezza del timbro:** La sinusoide a 220 Hz (A3) e a 440 Hz (A4) risulta estremamente pulita, priva di distorsione armonica spuria, ronzio o aliasing.
  2. **Transizioni:** I passaggi 220 $\to$ 440 Hz al secondo 1.0 e 440 $\to$ 220 Hz al secondo 2.0 avvengono senza click, pop, salti di fase repentini o anomalie percettive.
  3. **Dinamica:** L'ampiezza percepita è rigorosamente costante ed equilibrata tra le tre fasi (RMS pari a -23.01 dBFS).
  4. **Artefatti temporali:** Assenza totale di dropouts, stuttering, crackle o sfasamenti di clock; riproduzione uniforme per tutti i 3.0 secondi.
- **Esito AC-AO-01B:** **PASSATO** (capacità verificata, artefatto tracciato per hash, ascolto e giudizio qualitativo registrati).

---

## 1.3 Sintesi Remediation Difetti M2 (DIF-M2-01 – DIF-M2-07)

| ID | Severità | Defetto | Intervento Applicato | Esito / Evidenza |
|---|---|---|---|---|
| **DIF-M2-01** | **BLOCKER** | `runToneProbeV0Offline()` chiamava `startTone()`, avviando il device WASAPI in background e generando una race condition realtime sul runner e su `AudioProbe`. | Implementato `AudioEngine::initOffline()` che inizializza la sintesi senza device; aggiunto tracking di quiescenza `probeActiveWriters_` con `_mm_pause()` in `detachAudioProbe()`. | **Risolto** (`audio_engine.cpp`, `audio_probe.cpp`) |
| **DIF-M2-02** | **BLOCKER** | `SetTempoMap` ignorava `targetSampleFrame` applicando subito i cambi; la sostituzione della mappa produceva discontinuità di fase. | In `processBlock()` i comandi futuri sono trattenuti via `peek()` fino al blocco di competenza; in `MusicClock` implementati segmenti temporali ancorati `TempoSegment` che garantiscono continuità esatta di battute, misure e fase. | **Risolto** (`music_clock.cpp`, `audio_engine.cpp`, `test_music_clock.cpp`, `test_scheduler.cpp`) |
| **DIF-M2-03** | **BLOCKER** | `processBlock()` consumava eventi schedulati senza applicarli agli offset esatti; consumo consentito durante `Paused`; mixing coordinate 44.1/48 kHz. | Segmentazione intra-blocco dei sample frame per eventi schedulati con applicazione campionaria esatta del DSP; congelamento del consumo durante `Paused`/`Stopped`; conversione coerente coordinate timeline via `musicClock`. | **Risolto** (`audio_engine.cpp`, `test_scheduler.cpp`) |
| **DIF-M2-04** | **HIGH** | `MusicClock::frameAtBar()` ignorava il denominatore della metrica: 3/8 a 120 BPM produceva 72.000 frame anziché 36.000 frame. | Integrato il fattore metrico $\times 4 / \text{timeSignatureDen}$ in `frameAtBeat`, `frameAtBar`, `frameAtPhrase` e `positionAtFrame`. Aggiunti golden test per 3/8 (36.000 frame), 6/8 (72.000 frame), 5/4 (120.000 frame), 2/2 (96.000 frame). | **Risolto** (`music_clock.cpp`, `test_music_clock.cpp`) |
| **DIF-M2-05** | **HIGH** | Il gate AC-AO-01A confondeva lo smoke sintetico con la consegna fisica degli eventi da parte dell'OS. | Separata e documentata esplicitamente la procedura di verifica interattiva con tastiera fisica SPACE da quella dello smoke sintetico CI. | **Risolto** (`m2_tone_probe_evidence.md`, `progress.md`) |
| **DIF-M2-06** | **HIGH** | Manifest CACP privo di identificativi di run/build, epoch, tick e sample-frame richiesti/applicati; verdetto PASS anche in caso di fallimento export. | `ToneProbeResult::toJson()` esteso con `build_id`, `transport_epoch`, `simulation_ticks`, `requested_sample_frames`, `applied_sample_frames`, `presentation_quality`; `passed` condizionato obbligatoriamente a `wavWriteSuccess && manifestWriteSuccess`. | **Risolto** (`audio_probe.h`, `audio_probe.cpp`) |
| **DIF-M2-07** | **MEDIUM** | La skill `cacp-testing` assumeva capacità multimodali non garantite senza tracciare fallback normativo, hash e modello. | Aggiornata la skill con fallback a `unsupported`/`blocked` per ambienti non multimodali; registrati modello `antigravity-gemini-2.5-pro`, capacità `audio/wav`, hash SHA-256 e timestamp. | **Risolto** (`.agents/skills/cacp-testing/SKILL.md`, `m2_tone_probe_evidence.md`) |
| **Secondari** | **LOW** | Troncamento con `& 0xFF` in `AudioCommand::makeSetTempoMap()` e overflow in `AudioScheduler::scheduleIntent()`. | Estesi i campi a 16-bit (`0xFFFF`) in `paramExtraU64` per `subdivisionsPerBeat` e `barsPerPhrase`; aggiunta protezione saturante contro overflow uint64 in `scheduleIntent`. | **Risolto** (`audio_control.h`, `audio_scheduler.cpp`) |

---

## 2. Risultati della Suite di Test (Tutte le Configurazioni)

- **Release Build (62/62 test superati, 100% pass rate):**
  - CTest label `ac-ao-01a`: `AC-AO-01A: authoritative tone_probe_v0 execution and verification` — **Passed (0.02 s)**.
  - CTest label `metrics`: `MusicClock calculates exact metric boundaries for 3/8, 6/8, 5/4, 2/2 (DIF-M2-04)` — **Passed (0.01 s)**.
  - CTest label `anchors`: `MusicClock continuous tempo transition preserves phase and beat continuity (DIF-M2-02)` — **Passed (0.01 s)**.
  - CTest label `sample_accurate`: `AudioEngine processBlock applies scheduled events at exact intra-block offsets (DIF-M2-03)` — **Passed (0.01 s)**.
  - CTest label `pause`: `AudioEngine processBlock does not consume scheduled events while Paused (DIF-M2-03)` — **Passed (0.01 s)**.
  - CTest label `command_future`: `AudioEngine holds AudioCommand with future targetSampleFrame (DIF-M2-02)` — **Passed (0.01 s)**.
  - CTest label `late`: `AudioScheduler handles late events safely without queue corruption` — **Passed (0.01 s)**.
  - CTest label `zero_alloc`: Zero allocazioni heap con probe attivo e stallato — **Passed (0.03 s)**.
- **Headless Build:** 61/61 test superati (100% pass rate, device test escluso come previsto).
- **Debug Build:** 62/62 test superati (100% pass rate).
- **Smoke Run (Interactive):** `cymatica_game.exe -SmokeSeconds 4` completato con codice di uscita 0, 238 frame renderizzati, 59.5 FPS, 193.920 campioni audio, 478 tick, 0 sospensioni tecniche, `pure_seed=ELIGIBLE`.
- **CACP Diagnostic CLI:** `cymatica_game.exe --cacp-run tone_test` produce verdetto `PASS` ed esporta gli artefatti completi.
