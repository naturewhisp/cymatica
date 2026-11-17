# Pacchetto di misura 6 — Misure dello sviluppo (`development_measures_v1`)

Pre-registrazione: [M2_5_Pkg6_Development_Measures_Preregistration.md](../../M2_5_Pkg6_Development_Measures_Preregistration.md). Risultati: [M2_5_Pkg6_Development_Measures_Results.md](../../M2_5_Pkg6_Development_Measures_Results.md).

- `development_measures_v1_2026-11-17.json`: manifest con M4 (riferimento), M5 e M5r; prove V1–V5 e ammissione. Si rigenera con `cymatica_game --cacp-run development_measures` (Release, pochi secondi), dai controlli in `../m25_ensemble_tools/controls/`.
- **Sorgente:** [development_measures.cpp](../../../../engine/cacp/development_measures.cpp). Variogramma e nullo condivisi con la M4 tramite `variogramOfMatrix`; i valori M4 di M2.5.1c restano identici.
