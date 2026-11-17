# Pacchetto di misura 3 — ILD, ITD e IACC per banda (`binaural_v1`)

Pre-registrazione: [M2_5_Pkg3_Binaural_Bands_Preregistration.md](../../M2_5_Pkg3_Binaural_Bands_Preregistration.md). Risultati: [M2_5_Pkg3_Binaural_Bands_Results.md](../../M2_5_Pkg3_Binaural_Bands_Results.md).

- `binaural_v1_2026-11-17.json`: manifest con i controlli C1–C4, C6 e le misure della Trama per banda. Si rigenera con `cymatica_game --cacp-run binaural` (Release, circa 15 s), che scrive anche i WAV dei controlli in `artifacts/cacp/binaural_v1/`.
- `oracle_scipy_gammatone.py` e `oracle_scipy_gammatone.output.txt`: prova C5, con l'ambiente fissato di `tools/pyenv`.
