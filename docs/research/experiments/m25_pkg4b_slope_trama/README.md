# Pacchetto di misura 4b — Pendenza confrontabile e sensibilità della Trama

Pre-registrazione: [M2_5_Pkg4b_Slope_And_Trama_Sensitivity_Preregistration.md](../../M2_5_Pkg4b_Slope_And_Trama_Sensitivity_Preregistration.md). Risultati: [M2_5_Pkg4b_Slope_And_Trama_Sensitivity_Results.md](../../M2_5_Pkg4b_Slope_And_Trama_Sensitivity_Results.md).

- `stem_export_v2_2026-11-17.json`: manifest dell'esportazione (32 WAV float in `artifacts/cacp/stem_export_v2/`, non versionati; K1). Si rigenera con `cymatica_game --cacp-run stem_export_v2`.
- `slope_bands.py`, `slope_bands_2026-11-17.json`, `slope_bands.output.txt`: pendenza a terzi d'ottava, controlli G1–G2.
- `trama_sensitivity.py`, `trama_sensitivity_2026-11-17.json`, `trama_sensitivity.output.txt`: K2–K3, con MoSQITo (circa 7 minuti).
- Gli script si eseguono con l'ambiente fissato `tools/pyenv`.
