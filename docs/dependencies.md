# CYMATICA — Inventario dipendenze

Documento normativo richiesto da `CYMATICA_Specifica_Agentica_Sviluppo.md` §28.5 e `AGENTS.md` §2.
Ultimo aggiornamento: 2026-10-06 (Milestone 1).

## Riepilogo dipendenze attive

| Nome | Versione | Commit / Tag | Acquisizione | Licenza | Target | Owner interno |
|---|---|---|---|---|---|---|
| **miniaudio** | 0.11.25 | `9634bedb5b5a2ca38c1ee7108a9358a4e233f14d` | Vendored in `third_party/miniaudio/` | Public Domain / MIT No Attribution | `cymatica_audio` | `engine/audio` |
| **raylib** | 6.0 | `dbc56a87da87d973a9c5baa4e7438a9d20121d28` | CMake `FetchContent` (pinned SHA) | Zlib | `cymatica_game` | `apps/cymatica_game` |
| **Catch2** | v3.16.0 | `317ac1ed4c0bb6e6b91eafc817e05c488feffcb3` | CMake `FetchContent` (pinned SHA) | BSL-1.0 (Boost) | `cymatica_tests` | Test suite |
| **nlohmann/json** | v3.11.3 | `9cca280a4d0ccf0c08f47a99aa71d1b0e52f8d03` | CMake `FetchContent` (pinned SHA) | MIT | `cymatica_replay` | `engine/replay` |

---

## Schede dettagliate

### 1. miniaudio
- **Scopo:** Audio device management, mixing a basso livello e playback stream PCM.
- **Repository ufficiale:** `https://github.com/mackron/miniaudio`
- **Provenienza snapshot:** Tag `0.11.25` (commit `9634bedb5b5a2ca38c1ee7108a9358a4e233f14d`).
- **File inclusi nel repository:** `third_party/miniaudio/miniaudio.h`, `third_party/miniaudio/LICENSE`.
- **Integrità SHA-256:**
  - `miniaudio.h`: `AC7AF4DE748B7E26B777F37E01CEE313A308A7296A3EB080E2906B320CC55C89`
  - `LICENSE`: `457F1B500E0ADF6BC059EDDDFA78A2F62012E7C3BB43476C20E0BD23B25BA0EB`
- **Licenza:** Scelta alternativa tra Public Domain (Unlicense) e MIT No Attribution (permissiva, compatibile commerciale e open source).
- **Piattaforme:** Windows (WASAPI), Android (AAudio/OpenSL in futuro).
- **Vincoli architetturali:** Solo il modulo `engine/audio/audio_engine.cpp` include `MINIAUDIO_IMPLEMENTATION`. Nessun altro file o libreria nel binario può aprire il device o includere l'implementazione (spec §28.2).
- **Rischi:** Gestione device loss/cambio periferica durante la riproduzione; allocazioni nulle nel realtime callback verificate per contratto.

### 2. raylib
- **Scopo:** Creazione finestra, input management, contesto OpenGL 3.3 e compilazione shader GLSL.
- **Repository ufficiale:** `https://github.com/raysan5/raylib`
- **Provenienza snapshot:** Tag `6.0` (commit `dbc56a87da87d973a9c5baa4e7438a9d20121d28`).
- **Modalità acquisizione:** `FetchContent` di CMake con SHA di commit congelato.
- **Licenza:** Zlib (permissiva).
- **Piattaforme:** Desktop (Windows GLFW), Web (futuro), Android (futuro).
- **Configurazione obbligatoria (ADR-0001):**
  - `CUSTOMIZE_BUILD=ON`
  - `SUPPORT_MODULE_RAUDIO=OFF` (disabilita il backend audio interno di raylib per evitare duplicazioni del device audio)
  - `BUILD_EXAMPLES=OFF`
  - `BUILD_SHARED_LIBS=OFF`
- **Rischi:** Dipendenza da CMake ≥ 3.25 imposta da raylib 6.0; VSync non garantito su ogni display/driver, gestire loop con delta time.

### 3. Catch2
- **Scopo:** Framework per unit test, property-like test e micro-benchmark.
- **Repository ufficiale:** `https://github.com/catchorg/Catch2`
- **Provenienza snapshot:** Tag `v3.16.0` (commit `317ac1ed4c0bb6e6b91eafc817e05c488feffcb3`).
- **Modalità acquisizione:** `FetchContent` di CMake con SHA di commit congelato.
- **Licenza:** Boost Software License 1.0 (BSL-1.0, altamente permissiva).
- **Piattaforme:** Tutte.
- **Integrazione:** Registrazione automatica dei test tramite `catch_discover_tests` e runner CTest.
- **Rischi:** Tempo di compilazione iniziale leggermente maggiore rispetto a doctest header-only; mitigato dall'uso di Ninja e caching.

### 4. nlohmann/json
- **Scopo:** Serializzazione e deserializzazione JSON di `RunRecord`, configurazioni e trace di replay.
- **Repository ufficiale:** `https://github.com/nlohmann/json`
- **Provenienza snapshot:** Tag `v3.11.3` (commit `9cca280a4d0ccf0c08f47a99aa71d1b0e52f8d03`).
- **Modalità acquisizione:** `FetchContent` di CMake con SHA di commit congelato e `GIT_SHALLOW FALSE`.
- **Licenza:** MIT (permissiva, compatibile commerciale e open source).
- **Piattaforme:** Tutte (C++ standard).
- **Target CMake:** `nlohmann_json::nlohmann_json`.
- **Vincoli architetturali:** Confinata al target `cymatica_replay` e ai relativi test. `engine/core` resta indipendente da JSON (ADR-0002).
- **Configurazione CMake:** `JSON_BuildTests=OFF`, `JSON_Install=OFF`.

---

## Dipendenze future esplicitamente escluse in M1
Come da `AGENTS.md` §2 e Spec §19.4:
- Nessun runtime ONNX, FFmpeg o modelli ML.
- Nessun motore fisico esterno.
- Nessun parser JSON artigianale (scelta deliberata in M1: nlohmann/json).
