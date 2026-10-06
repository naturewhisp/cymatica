# CYMATICA — Stato milestone

Record di esecuzione richiesto da `AGENTS.md` §5.1. La roadmap autoritativa resta nella specifica (§32); questo file registra solo stato, decisioni ed evidenze.

## Stato corrente

| Campo | Valore |
|---|---|
| Milestone attiva | **M0 — Repository, build e dependency inventory** |
| Stato | **Criteri verificati sul working tree; manca la verifica da clean checkout dopo il commit** |
| Specifica | 0.8.2 (0.8.1 accettata il 2026-10-05; aggiunte System 1 del 2026-10-06, applicabili da M6) |
| Commit / Build | Working tree non committato su `ac1ac59` + modifiche M0 |
| Target prodotti | `cymatica_audio` (static lib), `cymatica_game` (app), `cymatica_tests` (test runner) |
| Ultimo aggiornamento | 2026-10-06 |

## Decisioni d'ingresso M0

| ID | Decisione | Stato | Record |
|---|---|---|---|
| D-M0-01 | Build Tools 2026 Release, MSVC 14.50, Ninja, CMake ≥ 3.25 | **validated** | [ADR-0001](adr/0001-m0-toolchain-dependencies.md) |
| D-M0-02 | raylib 6.0 `dbc56a8…`, `SUPPORT_MODULE_RAUDIO=OFF`, `SUPPORT_CUSTOM_FRAME_CONTROL=OFF` | **validated** | ADR-0001 |
| D-M0-03 | miniaudio 0.11.25 `9634bed…` vendored, unico owner audio in `cymatica_audio` | **validated** | ADR-0001 |
| D-M0-04 | Catch2 v3.16.0 `317ac1e…` + CTest | **validated** | ADR-0001 |
| D-M0-05 | Libreria JSON/schema | deferred (fine M0/M1) | spec §36 Q2 |
| D-M0-06 | CI Windows | open (non blocca M0 locale) | spec §32 |

## Criteri di accettazione M0 (Spec §32)

| Criterio | Esito | Evidenza |
|---|---|---|
| Clean checkout configurabile e compilabile da comandi documentati | **PARZIALE** | `scripts/build.ps1 -Clean -Test` da build dir vuota, eseguito da altra cwd: exit 0, nessun `warning C` con `/W4`. Non ancora verificato su un clone fresco (richiede commit). |
| `ctest` esegue e supera almeno un test reale | **PASSATO** | 3/3 Catch2 (formula tick placeholder fino al MusicClock M1; engine inerte prima di init; `[audio][device]` render reale silenzioso). Preset headless esclude label `device`. |
| Eseguibile avviabile e chiudibile senza crash | **PASSATO** | `-Run -SmokeSeconds 3`, exit 0. |
| Tono udibile e shader di prova visibile | **PASSATO** | Tono sinusoidale 220 Hz udito, e cambio a 440 Hz con SPAZIO confermato a orecchio dall'utente (2026-10-06). Shader Chladni visibile a ~60 FPS su RTX 3060. |
| Unico `MINIAUDIO_IMPLEMENTATION` e unico device | **PASSATO** | `dumpbin /symbols`: 0 `ma_device_init` in `raylib.lib`, 1 definizione in `cymatica_audio.lib`. |
| Dipendenze e licenze censite | **PASSATO** | `docs/dependencies.md`. |
| Decisioni M0 registrate | **PASSATO** | ADR-0001. |
| Nessun tool futuro richiesto | **PASSATO** | Nessun ML, ONNX, FFmpeg o Android. |

## Ambiente di sviluppo

- Windows x64, NT 10.0.26300; AMD Ryzen 7 7700X; NVIDIA RTX 3060 (driver 616.92, OpenGL 3.3).
- MSVC 14.50.35717 (Build Tools 2026), CMake 4.1.2-msvc8, Ninja 1.12.1. VS Insiders presente, ignorato da `vswhere`.
- miniaudio WASAPI 48 kHz, 2 canali f32.
- Frame timing: ~60 FPS con `SetTargetFPS(60)` più hint VSync; il VSync non è stato isolato dal limiter.

## Correzioni durante la revisione M0 (2026-10-06)

1. Data race: il main thread modificava il `ma_waveform` letto dalla callback. Ora i parametri sono `std::atomic` applicati nella callback.
2. `startTone` non cambiava frequenza se già attivo; aggiunto `setToneFrequency()`.
3. Opzioni raylib centralizzate in `CMakeLists.txt`; `/utf-8 /W4 /permissive-` tramite target `cymatica_compile_options`.
4. Preset test headless e label CTest dai tag Catch2.
5. `build.ps1` indipendente dalla cwd, fallisce sui codici di uscita non zero, toolset sovrascrivibile.

Rischi già risolti: nomi test non-ASCII che rompevano i filtri CTest; `CUSTOMIZE_BUILD=ON` che attivava `SUPPORT_CUSTOM_FRAME_CONTROL` (FPS sbloccati).

## Per chiudere M0

1. Commit delle modifiche.
2. Clone fresco in una directory temporanea + `scripts/build.ps1 -Test`; registrare qui l'esito e il commit.

## Prossima milestone

**M1 — Tempo, seed e contratti deterministici** (Spec §32): profilo temporale e transport epoch (§7.5), `SeedBank`/`RandomKey` (§15.3–§15.5), canali bounded audio/coordinator (§20.5–§20.7). Roadmap System 1 (§11.7): nessun lavoro prima di M6.
