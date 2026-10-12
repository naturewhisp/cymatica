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

### Passo 3: Ascolto Multimodale Diretto (AC-AO-01B)

Invocare lo strumento `view_file` sul file binario audio:

```text
tool: view_file
AbsolutePath: <repo_root>/artifacts/cacp/tone_probe_v0.wav
```

Il sistema inietta la traccia audio direttamente nel contesto multimodale dell'agente. L'agente deve formulare un **giudizio percettivo qualitativo** valutando:
1. **Purezza del timbro:** la sinusoide deve risultare pulita e stabile, priva di armoniche spurie indesiderate;
2. **Transizioni:** il passaggio 220 $\to$ 440 $\to$ 220 Hz deve avvenire senza click, pop, salti di fase violenti o distorsioni di clipping;
3. **Dinamica:** il livello sonoro percepito deve rimanere costante ed equilibrato tra le fasi;
4. **Artefatti temporali:** assenza totale di crackle, dropouts, jitter o discontinuità di buffer.

Registrare il giudizio percettivo formale nell'evidenza di milestone (es. `docs/m2_tone_probe_evidence.md`).

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

## 3. Evoluzione del CACP nelle Milestone Successive

Questa skill viene estesa progressivamente con l'avanzamento delle milestone:

- **Milestone 2:** `AudioProbe` tap PCM lock-free, `tone_probe_v0`, stima spettrale/RMS e ascolto multimodale WAV.
- **Milestone 3:** `VisualProbe` (cattura sequenziale di frame grafici, verifica telegraph, maschere di collisione e hazard visuali).
- **Milestone 4:** Scenari deterministici `.cymtest`, clip pre/post evento, replay comparativo e structured evaluation results.
- **Milestone 5:** Valutazione live della latenza end-to-end e osservazione non invasiva del gameplay in tempo reale.
