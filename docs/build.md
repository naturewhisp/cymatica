# CYMATICA — Guida alla compilazione

Documento normativo richiesto da `CYMATICA_Specifica_Agentica_Sviluppo.md` §28.1 e `AGENTS.md` §3.
Ultimo aggiornamento: 2026-10-06 (Milestone 0). Decisioni toolchain: [ADR-0001](adr/0001-m0-toolchain-dependencies.md).

## Prerequisiti su Windows

1. Windows 10/11 x64.
2. Visual Studio **Build Tools 2026** (canale Release) con workload C++ Desktop x64 e toolset MSVC 14.50. Altre versioni non sono verificate.
3. CMake ≥ 3.25 e Ninja ≥ 1.10 (quelli inclusi nei Build Tools vanno bene).
4. Git (dipendenze scaricate con commit pinned).

Le installazioni Preview/Insiders di Visual Studio possono coesistere: lo script le ignora.

---

## Metodo 1: script (consigliato, anche per agenti)

`scripts/build.ps1` trova i Build Tools Release con `vswhere`, entra nella DevShell x64 ed esegue configure, build e opzionalmente test/avvio. Funziona da qualsiasi directory se invocato con percorso assoluto, oppure dalla radice del repository:

```powershell
.\scripts\build.ps1                          # Release
.\scripts\build.ps1 -Test                    # Release + tutti i test CTest (richiede un device audio)
.\scripts\build.ps1 -Test -Headless          # Release, esclude i test con label "device" (CI/senza audio)
.\scripts\build.ps1 -Config Debug -Test      # Debug
.\scripts\build.ps1 -Clean -Test             # rimuove la build dir prima di configurare
.\scripts\build.ps1 -Run                     # avvia il gioco
.\scripts\build.ps1 -Run -SmokeSeconds 3     # avvio temporizzato, esce da solo
```

Il toolset predefinito è 14.50; per usarne un altro impostare `$env:CYMATICA_VCVARS_VER` (stringa vuota = default dell'installazione).

Le modifiche al `PATH` utente valgono solo per terminali aperti dopo la modifica.

---

## Metodo 2: CMake Presets

Da una **Developer PowerShell for VS** (Build Tools 2026), nella radice del repository:

```powershell
cmake --preset x64-release            # oppure x64-debug
cmake --build --preset x64-release
ctest --preset x64-release            # tutti i test
ctest --preset x64-release-headless   # esclude label "device"
.\build\x64-release\apps\cymatica_game\cymatica_game.exe
```

Le opzioni di build delle dipendenze sono definite solo in `CMakeLists.txt`, non nei preset.

---

## Verifica dell'unico owner audio

raylib è compilata con `SUPPORT_MODULE_RAUDIO=OFF`; miniaudio è implementata solo in `cymatica_audio` (spec §28.2). `dumpbin` è disponibile solo nella DevShell:

```powershell
# atteso 0
dumpbin /symbols build\x64-release\_deps\raylib-build\raylib\raylib.lib | Select-String "ma_device_init" | Measure-Object
# atteso 1
dumpbin /symbols build\x64-release\engine\audio\cymatica_audio.lib | Select-String "External.*ma_device_init\b" | Measure-Object
```
