---
name: cacp-testing
description: >-
  Procedura operativa per testare e validare in prima persona CYMATICA tramite il CACP
  (Cymatica Agent Control & Observation Plane). Usare questa skill ogni volta che si
  sviluppano o verificano feature con impatto audiovisivo o di controllo: esecuzione dei
  runner diagnostici, verifica quantitativa dei manifest JSON e ascolto/ispezione multimodale
  diretta dei file audio WAV (AudioProbe) e dei frame grafici (VisualProbe).
---

# CACP Testing — Guida Operativa di Validazione Multimodale

Il CACP (**Cymatica Agent Control & Observation Plane**) è l'interfaccia di sviluppo e diagnostica che consente agli agenti di interagire con CYMATICA, stimolare il sistema con input normalizzati e osservare in prima persona i risultati audiovisivi tramite capacità multimodali.

Non limitarsi mai a test unitari con asserzioni booleane isolate: per ogni feature con impatto sonoro, visivo o di controllo, l'agente deve eseguire lo scenario diagnostico appropriato ed esaminare direttamente gli artefatti prodotti.

---

## 1. Flusso di Lavoro per la Validazione del Suono (AudioProbe)

In Milestone 2, il controllo del tono e la sintesi audio vengono verificati tramite il tap PCM lock-free `AudioProbe` e il profilo di prova `tone_probe_v0`.

### Passo 1: Esecuzione del Runner Diagnostico

Eseguire lo scenario diagnostico locale tramite CLI del gioco:

```powershell
.\build\x64-release\apps\cymatica_game\cymatica_game.exe --cacp-run tone_test
```

Il runner opera a scenario finito (3.0 secondi a 120 Hz / 48 kHz = 144.000 campioni stereo):
1. **0.0s – 1.0s:** Baseline `Released` a 220 Hz;
2. **1.0s – 2.0s:** Pressione e tenuta `Pressed`/`Held` a 440 Hz (SPAZIO);
3. **2.0s – 3.0s:** Rilascio `Released` a 220 Hz.

Il runner esporta automaticamente gli artefatti in `artifacts/cacp/`:
- `artifacts/cacp/tone_probe_v0_manifest.json` (metriche quantitative e telemetria);
- `artifacts/cacp/tone_probe_v0.wav` (traccia audio PCM stereo standard a 48 kHz).

### Passo 2: Verifica Quantitativa del Manifest JSON

Leggere il file `artifacts/cacp/tone_probe_v0_manifest.json` tramite `view_file` e verificare:
- `"verdict": "PASS"`
- `"dropped_frames": 0` (zero drop nella ring buffer `AudioProbe`)
- Per ciascuna delle 3 fasi (finestre stabili da 250 ms escludendo i primi 100 ms di transizione):
  - Fase 1: frequenza stimata $220\text{ Hz} \pm 2\text{ Hz}$, RMS $> -40\text{ dBFS}$
  - Fase 2: frequenza stimata $440\text{ Hz} \pm 2\text{ Hz}$, RMS $> -40\text{ dBFS}$
  - Fase 3: frequenza stimata $220\text{ Hz} \pm 2\text{ Hz}$, RMS $> -40\text{ dBFS}$
- `"wav_write_success": true` e `"manifest_write_success": true`

### Passo 2b: Convalida Automatica in CI e Integrità JSON
La pipeline GitHub Actions (`.github/workflows/windows-ci.yml`) esegue questo runner su ogni push e Pull Request in configurazione Release.
- Il manifest JSON generato deve essere rigorosamente conforme a RFC 8259 senza richiedere sostituzioni manuali di caratteri (`.Replace('\', '/')`).
- I percorsi Windows contenenti backslash devono essere serializzati con escape nativo esplicito (`\\`).
- La CI valida il file direttamente tramite `ConvertFrom-Json` in PowerShell e i test Catch2 con `nlohmann::json::parse()`.

### Passo 2c: Verifica Quantitativa di Headroom e Assenza di Clipping
Prima di considerare valido qualsiasi artefatto audio generato:
- Verificare che il numero di campioni saturati a fondo scala ($\pm 32767$ in 16-bit PCM o $|x| \ge 1.0\text{f}$ in float) sia **esattamente pari a 0**.
- Verificare che il picco massimo dell'ampiezza non superi **$-6\text{ dBFS}$** ($\approx 0.50\text{f}$), confermando il rispetto del gain staging nominale di progetto (ADR-0003 D-M2-06).
- **Gestione Lock File Windows:** prima di sovrascrivere file WAV in rigenerazioni locali, verificare che non siano bloccati in lettura esclusiva da player esterni o app di messaggistica (`[Errno 13] Permission denied`).

### Passo 3: Ascolto Multimodale Diretto (AC-AO-01B)

Verificare preliminarmente se il client/modello in uso dispone della capacità di ingestione multimodale audio (`audio/wav`).
Qualora l'ambiente o il client non supportino l'ingestione multimodale di audio binario, registrare formalmente lo stato come **`unsupported`** o **`blocked`** (Spec §31.5). L'assenza di tale capacità esterna non blocca il gate semantico mandatory AC-AO-01A (che resta validato quantitativamente dal manifest e dai test Catch2), ma non deve mai essere spacciata per ascolto avvenuto.

Se la capacità multimodale audio è attiva, invocare lo strumento `view_file` sul file binario:

```text
tool: view_file
AbsolutePath: <repo_root>/artifacts/cacp/tone_probe_v0.wav
```

Il sistema inietta la traccia audio direttamente nel contesto multimodale dell'agente. L'agente formula un **giudizio percettivo qualitativo** valutando:
1. **Purezza del timbro:** la sinusoide deve risultare pulita e stabile, priva di armoniche spurie indesiderate;
2. **Transizioni:** il passaggio 220 $\to$ 440 $\to$ 220 Hz deve avvenire senza click, pop, salti di fase violenti o distorsioni di clipping;
3. **Dinamica:** il livello sonoro percepito deve rimanere costante ed equilibrato tra le fasi;
4. **Artefatti temporali:** assenza totale di crackle, dropouts, jitter o discontinuità di buffer.

Registrare il giudizio percettivo formale nell'evidenza di milestone (es. `docs/m2_tone_probe_evidence.md`) includendo:
- Identificativo client / modello agente;
- Capacità multimodale verificata (`audio/wav`);
- Hash crittografico SHA-256 dell'artefatto WAV;
- Sessione e timestamp di valutazione;
- Giudizio percettivo ed esito formale.

---

## 1.2 Flusso di Validazione Polifonia e Voice Stealing (voice_probe_v1 — Inc-1)

In M2.4 Inc-1, l'allocazione delle voci, il ciclo di vita ADSR, il voice stealing e le transizioni di trasporto vengono verificate con il runner `voice_test` (alias `voice_probe_v1`):

### Esecuzione del Runner Diagnostico

```powershell
.\build\x64-release\apps\cymatica_game\cymatica_game.exe --cacp-run voice_test
```

Lo scenario diagnostico offline (circa 2.5 secondi a 48 kHz):
1. **0.0s – 0.5s:** Attivazione simultanea di 2 voci sullo stesso ruolo (occupazione slot 0 e 1, 220 Hz + 330 Hz);
2. **0.5s – 1.0s:** Attivazione di una terza voce sullo stesso ruolo (440 Hz): verifica del **voice stealing** deterministico dello slot più vecchio con rampa di de-click di 2 ms (96 campioni) su steal tail statica;
3. **1.0s – 1.8s:** Transizione **Pause / Resume**: verifica della rampa fisica di de-click di 2 ms verso silenzio numerico assoluto (0.0f) e ripresa priva di discontinuità;
4. **1.8s – 2.5s:** Comando **Stop**: verifica della de-click tail lineare di 2 ms e reset a Idle di tutti gli 8 slot.

Artefatti esportati in `artifacts/cacp/`:
- `artifacts/cacp/voice_probe_v1_manifest.json` (metriche di voice allocation, drop count, de-click e verifiche di soglia);
- `artifacts/cacp/voice_probe_v1.wav` (traccia audio PCM stereo 48 kHz).

### Verifica Quantitativa e Multimodale
- **Manifest JSON:** `"verdict": "PASS"`, `"voice_steals_detected": 1`, `"dropped_frames": 0`, `"dsp_anomalies_passed": true`.
- **Dinamica e Headroom:** 0 campioni saturati a fondo scala, picco massimo $\le -6\text{ dBFS}$ (tipicamente $\approx -9.37\text{ dBFS}$).
- **Ispezione Multimodale Vincolante (`view_file` su `voice_probe_v1.wav`):** Invocare `view_file` per ascoltare direttamente l'audio nel contesto multimodale e formulare un giudizio percettivo esplicito:
  - *Timbro:* purezza sinusoidale senza ronzii o asprezze da aliasing;
  - *Accordi:* chiarezza dell'intervallo di quinta (220+330 Hz) e quarta (330+440 Hz) senza intermodulazione parassita;
  - *Voice Stealing:* transizione fluida e continua tra 220 Hz e 440 Hz a 500 ms senza click percettibili;
  - *Pause / Resume / Stop:* decadimento a zero lineare senza click, silenzio assoluto durante la pausa e al termine dello stop.
  Dichiarare esplicitamente lo stato di disponibilità dello strumento multimodale audio nelle evidenze.

---

## 1.3 Flusso di Validazione Music Planner e Candidate Search (planner_probe_v1 — Inc-3)

In M2.4 Inc-3, la generazione musicale gerarchica FormPlan $\to$ SectionPlan $\to$ PhrasePlan, la ricerca bounded di candidati, il voice tracking cross-frase e l'orchestrazione delle triadi vengono verificati con il runner `planner_test` (alias `planner_probe_v1`):

### Esecuzione del Runner Diagnostico

```powershell
.\build\x64-release\apps\cymatica_game\cymatica_game.exe --cacp-run planner_test
```

Lo scenario diagnostico offline (8 battute a 48 kHz / 120 Hz ticks, circa 15.0 secondi = 720.000 campioni):
1. **Battute 0..3 (Sezione A, 120 BPM, Re Dorico):** Pianificazione frase iniziale con progressione armonica modale, accordi aperti su Corpo con voice leading conservativo, pattern euclideo su Pulso e Trama, e variazione motivica melodica su Vettore.
2. **Battute 4..7 (Sezione B, 140 BPM, Sol Eolico):** Transizione di tempo e scala modale, estinzione lease open-pad cross-frase tramite `scheduleNoteOff`, nuova frase musicale a 140 BPM, mantenimento del limite fisico $\le 2$ voci per ruolo e voice leading senza interruzioni.
3. **Decadimento e Stop:** Comando `Stop` a tick 1750 (frame 700.000), verifica del silenzio numerico pulito post de-click tail.

Artefatti esportati in `artifacts/cacp/`:
- `artifacts/cacp/planner_probe_v1_manifest.json` (metriche di generazione, scoring deterministico, invarianti motivici, budget triadi e telemetria);
- `artifacts/cacp/planner_probe_v1.wav` (traccia musicale PCM stereo 48 kHz completa).

### Verifica Quantitativa e Multimodale
- **Manifest JSON:**
  - `"verdict": "PASS"`
  - `"determinism_passed": true` (bit-exactness degli eventi musicali pianificati a parità di seed)
  - `"stream_isolation_passed": true` (isolamento seed garantito: mutazioni nel canale Gameplay non alterano la forma musicale)
  - `"motif_invariance_passed": true` (rispetto delle note ancora e dei gradi modali)
  - `"triad_budget_passed": true` (massimo 2 voci simultanee su Corpo senza furti illegali)
  - `"fallback_passed": true` (gestione deterministica fail-closed con SilentRecovery su orizzonte temporale esausto)
  - `"non_silent_audio_passed": true` (energia RMS percepita $> -40\text{ dBFS}$ in entrambe le sezioni)
  - `"true_peak_passed": true` ($\le -1.0\text{ dBTP}$ verificato dal meter BS.1770-5)
  - `"zero_clipping_passed": true` (zero campioni saturati)
  - `"dsp_anomalies_passed": true` (zero NaN/Inf/denormals)
- **Ispezione Multimodale Vincolante (`read_file` su `planner_probe_v1.wav`):** Invocare `read_file` per verificare la composizione musicale generata:
  - *Struttura e Coerenza Armonica:* riconoscibilità dei centri tonali (Re Dorico $\to$ Sol Eolico) e fluidità degli accordi di sottofondo;
  - *Interlocking Ritmico:* alternanza e complementarietà tra i pattern euclidei di Pulso e i riff arpeggiati di Trama;
  - *Espressività Melodica:* presenza della melodia di Vettore con variazioni motiviche organiche;
  - *Dinamica e Limiting:* assenza di pompaggi eccessivi, distorsione, click o salti di fase.

---

## 2. Flusso di Collaudo dal Vivo (Smoke Run)

Per verificare l'applicazione completa end-to-end (inclusi finestra grafica raylib, context OpenGL e riproduzione fisica sugli altoparlanti WASAPI):

```powershell
powershell -ExecutionPolicy Bypass -File scripts/build.ps1 -Run -SmokeSeconds 4
```

In modalità smoke automatica (`smokeSeconds > 0`):
- Il gioco avvia la finestra a 60 FPS;
- Simula attivamente la pressione prolungata di SPAZIO tra 1.0s e 2.5s;
- Chiunque assista alla prova sente chiaramente il cambio di tono dal vivo (220 Hz $\to$ 440 Hz $\to$ 220 Hz);
- Verifica a fine corsa: exit code 0, zero sospensioni tecniche, `pure_seed=ELIGIBLE`.

In modalità interattiva manuale (`cymatica_game.exe` senza flag smoke):
- La finestra resta aperta indefinitamente;
- L'utente o un tester possono premere e rilasciare fisicamente la barra spaziatrice per verificare la reattività a schermo e in cuffia.

---

## 3. Evoluzione del CACP nelle Milestone e Sottofasi

Il CACP si sviluppa in parallelo continuo e proporzionato con il codice: ogni incremento con impatto audiovisivo o di controllo introduce o aggiorna il relativo profilo diagnostico.

- **Milestone 2.2 / M2.3:** `tone_probe_v0` (`--cacp-run tone_test` / `tone_probe_v0`) — tap PCM lock-free, stima spettrale/RMS.
- **Milestone 2.4 (Sviluppo Parallelo Incrementale):**
  - **Inc-1:** `voice_probe_v1` (`--cacp-run voice_test` / `voice_probe_v1`) — 8 voice slot, voice stealing, steal tails 2 ms, de-click Pause/Resume/Stop.
  - **Inc-2:** `timbre_bus_probe_v1` (`--cacp-run timbre_test` / `synth_test` / `timbre_bus_probe_v1`) — palette timbrica 4 ruoli, 4 bus, ducking, protezione terminale zero-latency.
  - **Inc-3:** `planner_probe_v1` (`--cacp-run planner_test` / `planner_probe_v1`) — generazione simbolica, invarianti motivici, trace diagnostico.
  - **Inc-4:** `fixture_probe_v1` (`--cacp-run fixture_test` / `profile_test` / `fixture_probe_v1`) — forma tripartita A-B-A' 96 battute, transizioni cross-epoch, profiling $\le 2,0$ ms.
  - **Inc-5:** Suite diagnostica consolidata, artefatti WAV definitivi e dossier CACP per il gate M2.5.
- **Milestone 3:** `VisualProbe` (cattura sequenziale di frame grafici, verifica telegraph, maschere di collisione e hazard visuali).
- **Milestone 4:** Scenari deterministici `.cymtest`, clip pre/post evento, replay comparativo e structured evaluation results.
- **Milestone 5:** Valutazione live della latenza end-to-end e osservazione non invasiva del gameplay in tempo reale.
