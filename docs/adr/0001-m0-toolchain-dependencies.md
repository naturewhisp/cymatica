# ADR-0001 — Toolchain, dipendenze core e test framework per M0

| Campo | Valore |
|---|---|
| ID | ADR-0001 (decisioni D-M0-01…D-M0-04 di `docs/progress.md`) |
| Milestone | M0 |
| Stato | **validated** — verificato sulla build M0 nel repository |
| Data | 2026-10-05 |
| Contratti interessati | spec §28.1–§28.2, §32 M0, §36 Q1; AGENTS §2–§3 |

## Contesto

M0 richiede toolchain, dipendenze e test framework pinned e un unico owner audio. Sulla macchina di sviluppo sono installati Build Tools 2019 (MSVC 14.29, CMake 3.20), Build Tools 2026 Release 18.3 (MSVC 14.44/14.50, CMake 4.1.2, Ninja 1.12.1) e Visual Studio Community 2026 Insiders 18.7 (canale Preview, MSVC 14.51, senza componente CMake). Nessuno strumento è nel `PATH` della shell normale.

## Decisioni

### D-M0-01 — Toolchain e generatore

- Baseline: **Visual Studio Build Tools 2026, canale Release**, toolset **MSVC 14.50** (cl 19.50.35724), x64 host e target.
- Generatore **Ninja** (1.12.1 incluso nei Build Tools), build single-config tramite preset (Debug/Release separati, out-of-source).
- `cmake_minimum_required(VERSION 3.25)`, imposto da raylib 6.0. Versione verificata: CMake 4.1.2-msvc8 incluso nei Build Tools.
- Ambiente: Developer Shell con toolset esplicito, senza modifiche globali:

```powershell
$vs = 'C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools'
Import-Module "$vs\Common7\Tools\Microsoft.VisualStudio.DevShell.dll"
Enter-VsDevShell -VsInstallPath $vs -SkipAutomaticLocation `
  -DevCmdArguments '-arch=x64 -host_arch=x64 -vcvars_ver=14.50'
```

Il percorso è specifico della macchina; `docs/build.md` dovrà renderlo configurabile (per esempio tramite `vswhere`).

Alternative: Insiders/MSVC 14.51 è un canale Preview, quindi non adatto come baseline riproducibile; resta ammesso come verifica locale facoltativa. Il generatore Visual Studio (MSBuild) funziona ma è multi-config e meno uniforme con CI e preset Ninja. Build Tools 2019 ha CMake 3.20, inferiore al minimo richiesto da raylib 6.0.

### D-M0-02 — raylib

- **raylib 6.0**, commit `dbc56a87da87d973a9c5baa4e7438a9d20121d28`, licenza Zlib, tramite `FetchContent` con SHA di commit.
- Opzioni: `CUSTOMIZE_BUILD=ON`, `SUPPORT_MODULE_RAUDIO=OFF`, `BUILD_EXAMPLES=OFF`, `BUILD_SHARED_LIBS=OFF`. Backend risultante: `PLATFORM_DESKTOP` (GLFW), `GRAPHICS_API_OPENGL_33`.

### D-M0-03 — miniaudio e unico owner audio

- **miniaudio 0.11.25**, commit `9634bedb5b5a2ca38c1ee7108a9358a4e233f14d`, **vendored** in `third_party/miniaudio/` (spec §28.2): solo `miniaudio.h` e `LICENSE`.
- SHA-256: `miniaudio.h` `AC7AF4DE748B7E26B777F37E01CEE313A308A7296A3EB080E2906B320CC55C89`; `LICENSE` `457F1B500E0ADF6BC059EDDDFA78A2F62012E7C3BB43476C20E0BD23B25BA0EB`.
- Licenza: doppia a scelta, Public Domain (Unlicense) oppure MIT No Attribution.
- Un solo target audio contiene `MINIAUDIO_IMPLEMENTATION` e apre il device. Verifica con `dumpbin /symbols`: nessun simbolo `ma_device_init` in `raylib.lib`, una sola definizione nel target audio.

### D-M0-04 — Test framework

- **Catch2 v3.16.0**, commit `317ac1ed4c0bb6e6b91eafc817e05c488feffcb3`, licenza BSL-1.0, tramite `FetchContent`; test registrati con `catch_discover_tests` ed eseguiti da `ctest`.
- Motivazione: integrazione CTest verificata, generatori per test parametrici e property-like (spec §25.2), micro-benchmark integrati utili ai budget di §27, manutenzione attiva.
- Alternative: doctest v2.5.3 (MIT) compila più in fretta ma offre meno generatori e benchmark. GoogleTest v1.18.0 (BSD-3) è più pesante e il mocking non serve ora. Costo accettato: libreria compilata e prima build più lenta.

### Fuori da questo ADR

- JSON/schema (spec §36 Q2): *deferred* a fine M0/M1; candidato principale nlohmann/json v3.12.0, da valutare insieme al formato dei dati.
- CI Windows: *open*.
- Licenza del progetto: *open*, blocca solo la distribuzione.

## Evidenza: spike esplorativo

Spike fuori dal repository (cartella scratch dell'agente), etichettato come **sperimentale**: non è la build M0 e non soddisfa i criteri M0. Eseguito il 2026-10-05 su Windows NT 10.0.26300, Ryzen 7 7700X, RTX 3060 (driver 616.92, OpenGL 3.3 core), build Release.

| Verifica | Esito |
|---|---|
| Configure + build Ninja/MSVC 14.50/CMake 4.1.2 (raylib, Catch2, miniaudio) | passato; configure 60,7 s, dominato dal clone completo per SHA |
| `ctest`: un test Catch2 | passato (1/1) |
| raylib senza raudio | log raylib: `raudio: not loaded`; 0 simboli `ma_device_init` in `raylib.lib` |
| Finestra, shader GLSL 330 di prova, tono 220 Hz via miniaudio | inizializzati senza errori; exit code 0 |

### Lezioni per M0 e risoluzione VSync

- Inizialmente lo smoke test ha generato decine di migliaia di frame in pochi secondi (VSync bypass): quando in raylib viene impostato `CUSTOMIZE_BUILD=ON`, l'opzione `SUPPORT_CUSTOM_FRAME_CONTROL` viene abilitata di default, disabilitando lo swap dei buffer e il frame sleep interno in `EndDrawing()`. Impostando esplicitamente `SUPPORT_CUSTOM_FRAME_CONTROL=OFF` in `CMakeLists.txt` e `CMakePresets.json`, raylib gestisce correttamente il vertical sync e il frame pacing a 60 FPS (179 frame in 3.02s nello smoke test).
- Caratteri non-ASCII nei nomi dei test (es. simbolo di sezione) provocano errori di parsing nel runner CTest su Windows: usare sempre identificatori ASCII.
- Il fetch per SHA richiede un clone completo; la cache delle dipendenze è preservata nella cartella di build.
- Sistema con due GPU: confermato che il contesto OpenGL opera su NVIDIA GeForce RTX 3060.

## Criteri di verifica per *validated*

I criteri M0 di spec §32, eseguiti sulla build nel repository da clean checkout con i comandi di `docs/build.md`, con evidenze in `docs/progress.md`.

## Revisione

Un cambio di versione o di toolset richiede un nuovo ADR o un aggiornamento di questo, l'aggiornamento di `docs/dependencies.md` e una nuova esecuzione dei criteri M0.

## Nota 2026-10-06

Le opzioni di build delle dipendenze sono definite solo in `CMakeLists.txt`; i target del progetto usano `/utf-8 /W4 /permissive-` (target `cymatica_compile_options`). Il preset di test `x64-release-headless` esclude i test con label `device`. Nessun cambio alle decisioni.

