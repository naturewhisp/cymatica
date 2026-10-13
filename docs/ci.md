# CYMATICA — Continuous Integration (CI)

Documento normativo e guida alla pipeline CI di CYMATICA (`spec §28.1`, `AGENTS §2–§3`, `docs/progress.md` D-M0-06).
Ultimo aggiornamento: 2026-10-13.

---

## 1. Architettura della Pipeline

La pipeline di Continuous Integration è definita in `.github/workflows/windows-ci.yml` ed è configurata per eseguire la build e i test automatici su ambiente Windows nativo con MSVC.

### 1.1 Trigger e Concorrenza

- **Eventi di trigger**:
  - `push` su branch `main`
  - `pull_request` verso branch `main`
  - `workflow_dispatch` (avvio manuale via interfaccia web o GitHub CLI)
- **Concorrenza**:
  - `group: ${{ github.workflow }}-${{ github.ref }}` con `cancel-in-progress: true`: cancellazione immediata di esecuzioni obsolete in caso di push ravvicinati sullo stesso branch/PR.
- **Permessi minimi**:
  - `permissions: contents: read` (nessun permesso di scrittura, nessun token o secret con privilegi elevati).
- **Timeout**:
  - 30 minuti per job.

---

## 2. Matrice e Ambienti di Esecuzione

| Configurazione | Runner GitHub Actions | Compilatore / Toolchain | Generatore | Presets CMake / CTest |
|---|---|---|---|---|
| **Release** | `windows-2025` | MSVC x64 (v144/v145 tramite Build Tools / VS) | Ninja | `x64-release` / `x64-release-headless` |
| **Debug** | `windows-2025` | MSVC x64 (v144/v145 tramite Build Tools / VS) | Ninja | `x64-debug` / `x64-debug-headless` |

- **fail-fast**: impostato a `false` per garantire che entrambi i target (Release e Debug) completino l'esecuzione e forniscano feedback diagnostico completo indipendentemente dall'esito dell'altro.
- **Inizializzazione ambiente**:
  La build invoca `scripts/build.ps1`, che individua l'installazione Visual Studio stabile tramite `vswhere.exe`, inizializza la Developer Shell x64 (`Enter-VsDevShell`) e imposta le variabili d'ambiente senza mutazioni globali di sistema.
- **Diagnostica toolchain**:
  Prima della compilazione, il workflow stampa i dettagli di installazione di Visual Studio/MSVC, la versione di CMake e la versione di Ninja. Differenze di revisione rispetto al toolset di sviluppo locale (MSVC 14.50/14.51 di ADR-0001) sono tracciate nel log diagnostico della build.

---

## 3. Fasi di Verifica

### 3.1 Clean Build e CTest Headless

In entrambe le configurazioni (`Release` e `Debug`):
1. Esecuzione di `scripts/build.ps1 -Clean -Config <Config> -Test -Headless`.
2. Compilazione pulita da zero di tutti i target del repository e delle dipendenze pinned (`raylib` 6.0, `Catch2` v3.16.0, `nlohmann/json` v3.11.3).
3. Esecuzione della suite CTest tramite i preset headless dedicati:
   - `x64-release-headless` per Release
   - `x64-debug-headless` per Debug
   I test che richiedono un dispositivo fisico WASAPI o output audio reale sono etichettati con `[device]` e vengono esclusi su macchine CI headless.

### 3.2 Test Diagnostico CACP (Release)

Al termine di CTest nel job `Release`, la CI esegue la verifica offline del CACP (Cymatica Agent Control & Observation Plane):
```powershell
.\build\x64-release\apps\cymatica_game\cymatica_game.exe --cacp-run tone_test
```
Il runner opera interamente in modalità offline (`AudioEngine::initOffline`), sintetizzando il PCM in memoria ed esportando gli artefatti diagnostici senza alcuna dipendenza da device WASAPI fisici, GPU interattiva o server X/desktop.

Il workflow verifica programmaticamente il file manifest `artifacts/cacp/tone_probe_v0_manifest.json`:
- `verdict == PASS`
- `telemetry.dropped_frames == 0`
- Esito positivo delle tre fasi tonali (220 Hz baseline $\to$ 440 Hz hold $\to$ 220 Hz release con tolleranza $\pm 2$ Hz e RMS $>-40$ dBFS)
- `wav_write_success == true` e `manifest_write_success == true`
- Esistenza fisica del file WAV esportato (`artifacts/cacp/tone_probe_v0.wav`)

> [!NOTE]
> Il superamento del test CACP in CI certifica i requisiti quantitativi e contrattuali di **AC-AO-01A**. Non certifica il criterio supplementare **AC-AO-01B**, poiché l'ascolto percettivo in prima persona dell'agente costituisce una procedura separata documentata in `docs/m2_tone_probe_evidence.md`.

---

## 4. Gestione Risorse e Sicurezza

- **Zero binari negli artefatti**: non vengono caricate directory di build o eseguibili completi.
- **Log diagnostici su fallimento**: solo in caso di errore (`if: failure()`), vengono raccolti i log di CTest (`build/**/Testing/Temporary/*.log`) e gli eventuali manifest CACP (`artifacts/cacp/*.json`) con retention ridotta a 3 giorni.
- **Nessun secret**: la pipeline non consuma secret né credenziali esterne.
