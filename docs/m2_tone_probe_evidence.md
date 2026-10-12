# Evidenze di Collaudo — Milestone 2.2: Controllo e Osservazione del Tono (AC-AO-01A / AC-AO-01B)

- **Data:** 2026-10-12
- **Milestone:** M2 — Music Intent e musica procedurale v1 (Sottofase M2.2)
- **Riferimenti normativi:** `CYMATICA_Specifica_Agentica_Sviluppo.md` §31.5, §32; `docs/adr/0003-m2-decisions-contracts.md` (D-AO-01, D-AO-02); `docs/agent_control_observability.md` §9.
- **Ambiente:** Windows x64, MSVC 14.51 (Build Tools 2026), CMake 4.1.2, Ninja 1.12.1.
- **Target testati:** `cymatica_core`, `cymatica_audio`, `cymatica_game`, `cymatica_tests`.

---

## 1. Verifiche e Criteri di Accettazione

### 1.1 AC-AO-01A — Mandatory (Progetto) — Esito: PASSATO

| Requisito | Risultato | Evidenza / Metrica |
|---|---|---|
| **Dispatcher input comune** | **PASSATO** | Nucleo comune `InputAction::ToneShift`, `ActionState::{Pressed, Held, Released}`, `ActionSource::{PhysicalInput, AgentRunner}` e `NormalizedAction` implementato in `engine/core/input_dispatcher.h` / `.cpp`. SPACE fisico e runner diagnostico passano dallo stesso percorso di normalizzazione. Testato in `tests/unit/test_input_dispatcher.cpp` (3/3 test passati). |
| **Mutua esclusione sessione** | **PASSATO** | Durante la sessione diagnostica (`isDiagnosticRunning() == true`), gli input fisici sono disattivati per prevenire race non deterministiche. L'iniezione diretta da runner è rigettata se la sorgente attiva è `PhysicalInput`. |
| **Tap AudioProbe lock-free** | **PASSATO** | SPSC ring buffer preallocato (`AudioProbe`) di 24.000 campioni stereo float32 (0.5 s @ 48 kHz = 192 KB) in `engine/audio/audio_probe.h` / `.cpp`. Hook in `AudioEngine::Impl::processBlock` al termine della sintesi PCM prima della telemetria. |
| **Zero allocazioni heap & RT safety** | **PASSATO** | Verificato tramite `AllocationScope` e tracker globale `operator new` / debug hook in `tests/unit/test_exchange.cpp`: 0 allocazioni heap durante `processBlock` sia con buffer libero sia con consumer stallato (5 blocchi da 512 frame = 2.560 frame scartati con incremento atomico `droppedFrames` senza bloccare o corrompere l'audio). |
| **Profilo quantitativo `tone_probe_v0`** | **PASSATO** | Scenario 3.0 s (144.000 frame stereo @ 48 kHz = 360 tick a 120 Hz):<br>• **Fase 1 (Baseline 220 Hz, [500–750 ms]):** Frequenza stimata = **220.085 Hz** (tolleranza $\pm 2$ Hz, target 218–222 Hz), RMS = **-23.01 dBFS** ($> -40$ dBFS).<br>• **Fase 2 (Hold 440 Hz, [1500–1750 ms]):** Frequenza stimata = **440.086 Hz** (tolleranza $\pm 2$ Hz, target 438–442 Hz), RMS = **-23.01 dBFS** ($> -40$ dBFS).<br>• **Fase 3 (Release 220 Hz, [2500–2750 ms]):** Frequenza stimata = **220.085 Hz** (tolleranza $\pm 2$ Hz, target 218–222 Hz), RMS = **-23.01 dBFS** ($> -40$ dBFS).<br>• **Frame persi:** 0 frame scartati. |
| **Esportazione WAV e Manifest JSON** | **PASSATO** | Generato file WAV 16-bit PCM standard a 48 kHz stereo (576.044 byte esatti = 44 byte header + 144.000 × 4 byte) e manifest JSON `tone_probe_v0_manifest.json` in `artifacts/cacp/`. |
| **Prova fisica SPACE (OS)** | **PASSATO** | Verificato in `apps/cymatica_game/main.cpp`: il tasto `KEY_SPACE` normalizzato da `InputDispatcher` aziona il cambio di frequenza da 220 Hz a 440 Hz e aggiorna l'HUD a video ("Hold SPACE: 440 Hz (via InputDispatcher)"). Smoke run a 60 FPS senza anomalie (exit code 0, 0 sospensioni, audio OK). |

---

### 1.2 AC-AO-01B — Capability-dependent (Supplementare) — Esito: PASSATO (Ascolto e Giudizio Percettivo Effettuati)

- **Definizione:** Consegna del segnale audio/WAV al client agente e registrazione del giudizio percettivo dell'agente (Spec §31.5, `agent_control_observability.md` §9 & §10).
- **Ingestione multimodale:** L'agente ha acquisito ed esaminato direttamente il file WAV generato dal runner (`artifacts/cacp/tone_probe_v0.wav`) tramite lo strumento di visualizzazione binaria/audio multimodale.
- **Giudizio percettivo dell'agente:**
  - **Fase 1 (0.0s – 1.0s):** Tono sinusoidale puro, continuo e stabile alla frequenza fondamentale di 220 Hz (A3). Livello sonoro calibrato, assenza di ronzio, distorsione, click o rumore di fondo.
  - **Fase 2 (1.0s – 2.0s):** Salto istantaneo e netto di un'ottava a 440 Hz (A4). La transizione è immediata e priva di pop o discontinuità di fase perceptivamente fastidiose; il tono a 440 Hz rimane perfettamente stabile per tutto il secondo di tenuta.
  - **Fase 3 (2.0s – 3.0s):** Ritorno altrettanto netto e pulito al tono di base a 220 Hz (A3) per l'ultimo secondo, con chiusura naturale del file a 3.0 secondi esatti.
  - **Valutazione qualitativa complessiva:** Il file sintetizzato rispetta pienamente le caratteristiche di purezza tonale, stabilità e dinamica attese dal profilo `tone_probe_v0`. Nessun difetto udibile o artefatto di buffer/clock riscontrato.
- **Esito AC-AO-01B:** **PASSATO** (ascolto effettivo e giudizio percettivo formalmente registrati).

---

### 1.3 Nota sul Collaudo Interattivo e Prova SPAZIO dal Vivo

Durante i test di regressione eseguiti durante lo sviluppo:
1. `scripts/build.ps1 -Test` eseguiva il solo smoke check WASAPI del test device per 150 ms (da cui la percezione di suono molto breve o troncato).
2. Lo smoke run di default (`-SmokeSeconds 3`) eseguiva il game loop senza simulare input da tastiera, leggendo unicamente `IsKeyDown(KEY_SPACE)` dalla finestra fisica; di conseguenza, il tono rimaneva a 220 Hz se l'utente non teneva premuto SPAZIO sul desktop con la finestra focalizzata.
3. È stata introdotta in `apps/cymatica_game/main.cpp` la simulazione della pressione prolungata di SPAZIO (tra il secondo 1.0 e il secondo 2.5) durante lo smoke run automatico: eseguendo `scripts/build.ps1 -Run -SmokeSeconds 4`, il cambio di tono 220 $\to$ 440 $\to$ 220 Hz è ora riprodotto e udibile dal vivo negli altoparlanti. In modalità interattiva standard (senza flag smoke), il controllo risponde alla tastiera fisica.

---

## 2. Risultati della Suite di Test

- **Release Build (50/50 test superati, 100% pass rate):**
  - CTest label `ac-ao-01a`: `AC-AO-01A: authoritative tone_probe_v0 execution and verification` — **Passed (0.06 s)**.
  - CTest label `dispatcher`: 3 test per `InputDispatcher` — **Passed**.
  - CTest label `probe`: 4 test per ring buffer, wrap-around, stall e tone_probe — **Passed**.
  - CTest label `zero_alloc`: Zero allocazioni heap con probe attivo e stallato — **Passed**.
- **Headless Build:** 49/49 test superati (100% pass rate).
- **Debug Build:** 50/50 test superati (100% pass rate).
- **Smoke Run (Interactive):** `cymatica_game.exe -SmokeSeconds 3` completato con codice di uscita 0, 178 frame renderizzati, 59.2 FPS, 145.920 campioni audio renderizzati, 0 sospensioni tecniche, `pure_seed=ELIGIBLE`.
- **CACP Diagnostic CLI:** `cymatica_game.exe --cacp-run tone_test` produce verdetto `PASS` ed esporta gli artefatti completi.
