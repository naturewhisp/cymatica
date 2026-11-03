# Strumenti di verifica del modello matematico (indagine A e riconciliazione)

Strumenti con cui l'agente ha verificato i numeri dei dossier [M2_4_Inc5_Math_Model_Claude.md](../../M2_4_Inc5_Math_Model_Claude.md) e [M2_4_Inc5_Math_Model_Reconciliation.md](../../M2_4_Inc5_Math_Model_Reconciliation.md). Sono archiviati su raccomandazione della review dei commit `c1f1a44`/`794b642`/`862e0e6`. Non fanno parte della build: non sono in CMake e non sono test.

| File | Verifica | Dove è usato |
|---|---|---|
| `tie_probe.cpp` | pianifica `fixture-m2-4-v1` (8 candidati) e `planner_probe_v1` (6 e 8) con i seed 1337 e 1..64; stampa punteggi, pareggi e vincitori di ogni decisione; confronta il contenuto fra seed | dossier A §4.2 (Teorema D, 0 pareggi) |
| `limiter_probe.cpp` | applica il `MasterLimiter` reale a 1 s di seni stereo (incluso f_s/4 con fase −π/4) e misura il true peak prima e dopo con `analysis::estimateTruePeakDbtp` | riconciliazione §4.2 (AG-MM-10 smentito) |
| `clock_check.py` | confini della fixture, durate dei beat a 140 BPM, inversione esatta del confine di beat contro la forza bruta (2·10⁵ casi) | dossier A §1 |
| `envelope_slopes.py` | rapporto ρ fra le pendenze di rilascio e di attacco per ruolo | dossier A §6.4 |
| `grid_140bpm.py` | griglia relativa alla frase nella sezione B: jitter di ±1 campione con periodo 7 | riconciliazione §4.1 (AG-MM-07) |
| `phrase_phase_f32.py` | valore in float32 di `phrasePhase` all'ultimo frame di una frase | riconciliazione §5 (AG-MM-02) |

**Compilazione dei programmi C++** (Windows, da una Developer PowerShell della toolchain di ADR-0001, dopo `scripts/build.ps1 -Config Release`, dalla radice del repository e con l'output in una directory di lavoro esterna):

```powershell
cl /nologo /std:c++20 /O2 /EHsc /MD /utf-8 /Iengine\audio /Iengine\core docs\research\experiments\inc5_math_model\tie_probe.cpp build\x64-release\engine\audio\cymatica_audio.lib build\x64-release\engine\core\cymatica_core.lib /Fe:<dir>\tie_probe.exe
cl /nologo /std:c++20 /O2 /EHsc /MD /utf-8 /Iengine\audio /Iengine\core docs\research\experiments\inc5_math_model\limiter_probe.cpp build\x64-release\engine\audio\cymatica_audio.lib build\x64-release\engine\core\cymatica_core.lib /Fe:<dir>\limiter_probe.exe
```

**Script Python:** usano solo la libreria standard: `python -I <script>.py`.

**Commit di riferimento:** `df0bcbe`. I programmi usano solo API presenti a quel commit.
