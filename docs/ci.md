# CYMATICA — Continuous Integration (CI)

Documento normativo e guida alla pipeline CI di CYMATICA (`spec §28.1`, `AGENTS §2–§3`, `docs/progress.md` D-M0-06).
Ultimo aggiornamento: 2026-11-03 (livelli di CI: ordinaria e completa).

---

## 1. Architettura della Pipeline

La pipeline di Continuous Integration è definita in due workflow, entrambi su ambiente Windows nativo con MSVC:

| Livello | Workflow | Quando | Verifiche |
|---|---|---|---|
| CI ordinaria | `.github/workflows/windows-ci.yml` | ogni push e PR su `main`, avvio manuale | Release: build pulita, tutti i test headless, `tone_test` e `fixture_test` (catena O3, golden di piano e PCM). Debug: build pulita completa e test headless senza tag `[slow]` |
| CI completa | `.github/workflows/windows-ci-full.yml` | ogni notte (02:30 UTC) su `main`, avvio manuale | Release e Debug: build pulita e **tutti** i test headless, compresi gli `[slow]` |
| Gate locale M2 | `scripts/build.ps1` e scenari CACP | chiusura di sottofasi e milestone | suite headless completa nelle due configurazioni, `full_m2_suite`, profiling, pacchetti v1/v2, `release_ab_v1`, riproducibilità |
| Validazione percettiva | sessioni dedicate | su richiesta | ascolto esperto, sessioni alla cieca, confronti A/B |

I test restano tutti obbligatori: il livello stabilisce solo la frequenza con cui bloccano. Il tag `[slow]` marca le generazioni pesanti dei pacchetti di valutazione, circa il 91 % del tempo CTest in Debug (run 41599948296). Si assegna solo a test lenti in Debug che restano coperti dalla Release della CI ordinaria; non si usa per test brevi né per escludere test instabili. Decisione: [ADR-0006](adr/0006-m2-4-inc5-entry-decisions.md), precisazione del 2026-11-03 sui livelli di CI.

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
  - 30 minuti per job nella CI ordinaria, 60 nella CI completa.

---

## 2. Matrice e Ambienti di Esecuzione

| Configurazione | Runner GitHub Actions | Compilatore / Toolchain | Generatore | Presets CMake / CTest |
|---|---|---|---|---|
| **Release** | `windows-2025` | MSVC x64 (v144/v145 tramite Build Tools / VS) | Ninja | `x64-release` / `x64-release-headless` |
| **Debug** | `windows-2025` | MSVC x64 (v144/v145 tramite Build Tools / VS) | Ninja | `x64-debug` / `x64-debug-fast` (CI ordinaria), `x64-debug-headless` (CI completa) |

- **fail-fast**: impostato a `false` per garantire che entrambi i target (Release e Debug) completino l'esecuzione e forniscano feedback diagnostico completo indipendentemente dall'esito dell'altro.
- **Inizializzazione ambiente**:
  La build invoca `scripts/build.ps1`, che individua l'installazione Visual Studio stabile tramite `vswhere.exe`, inizializza la Developer Shell x64 (`Enter-VsDevShell`) e imposta le variabili d'ambiente senza mutazioni globali di sistema.
- **Diagnostica toolchain**:
  Prima della compilazione, il workflow stampa i dettagli di installazione di Visual Studio/MSVC, la versione di CMake e la versione di Ninja. Differenze di revisione rispetto al toolset di sviluppo locale (MSVC 14.50/14.51 di ADR-0001) sono tracciate nel log diagnostico della build.

---

## 3. Fasi di Verifica

### 3.1 Clean Build e CTest Headless

Nella CI ordinaria la Release esegue `scripts/build.ps1 -Clean -Config Release -Test -Headless`; la Debug esegue `scripts/build.ps1 -Clean -Config Debug` e poi `ctest --preset x64-debug-fast`, che esclude le label `device`, `perf` e `slow`. La CI completa esegue `scripts/build.ps1 -Clean -Config <Config> -Test -Headless` in entrambe le configurazioni. `scripts/build.ps1 -Test -Headless` resta il comando di validazione completa. Dal 2026-11-11 (M2.5.1c, decisione del titolare) tutte le build della CI passano `-WarningsAsErrors` (`CYMATICA_WARNINGS_AS_ERRORS=ON`, `/WX`); la build prosegue dopo un errore (`ninja -k 0`) e la matrice ha `fail-fast: false`, così una corsa raccoglie gli errori di tutte le unità compilabili di entrambe le configurazioni invece di fermarsi al primo. Non è un inventario esaustivo: le unità che dipendono da un target fallito non vengono compilate, e un passo fallito impedisce i passi successivi dello stesso job (con la build fallita, CTest non parte). I test dei preset headless e fast girano in parallelo (`execution.jobs` 8); CTest esegue comunque tutti i test e riassume i falliti alla fine.

Passi:
1. Esecuzione di `scripts/build.ps1` come sopra.
2. Compilazione pulita da zero di tutti i target del repository e delle dipendenze pinned (`raylib` 6.0, `Catch2` v3.16.0, `nlohmann/json` v3.11.3).
3. Esecuzione della suite CTest tramite i preset headless dedicati:
   - `x64-release-headless` per Release (label `device` e `perf` escluse)
   - `x64-debug-fast` (CI ordinaria) o `x64-debug-headless` (CI completa) per Debug
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

### 3.3 Fixture Test CACP (Release)

Dopo `tone_test`, il job Release della CI ordinaria esegue `--cacp-run fixture_test` (ADR-0006 5e-03) e verifica dal manifest `artifacts/cacp/fixture_probe_v1_manifest.json`:

- `verdict == PASS`;
- catena O3 superata e senza discrepanze;
- `dropped_frames == 0`;
- `golden_pcm_status == PASS` e uguaglianza esatta fra `pcm_content_hash` e `golden_pcm_content_hash`.

---

## 4. Gestione Risorse e Sicurezza

- **Zero binari negli artefatti**: non vengono caricate directory di build o eseguibili completi.
- **Log diagnostici su fallimento**: solo in caso di errore (`if: failure()`), vengono raccolti i log di CTest (`build/**/Testing/Temporary/*.log`) e gli eventuali manifest CACP (`artifacts/cacp/*.json`) con retention ridotta a 3 giorni.
- **Nessun secret**: la pipeline non consuma secret né credenziali esterne.
