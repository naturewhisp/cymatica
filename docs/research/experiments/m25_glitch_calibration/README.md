# Calibrazione dei rilevatori di glitch (R-M25-8, 2026-11-05)

Strumento e dati del [dossier R-M25-8](../../M2_5_1_Windows_Audio_Glitch_Detection.md). `glitchcal.cpp` è un programma minimo in WASAPI puro: imita lo schema di scrittura di miniaudio, emette un tono a −80 dBFS e inietta stalli noti. Non è il motore di CYMATICA, non è compilato dal progetto e le sue misure sono esplorative.

- `run*_stalls.csv`, `x_stalls.csv`: stalli iniettati per corsa.
- `an.py`, `an2.py`: analisi delle catture.
- Le catture `.f32` e l'eseguibile non sono versionati: si rigenerano compilando `glitchcal.cpp`.
