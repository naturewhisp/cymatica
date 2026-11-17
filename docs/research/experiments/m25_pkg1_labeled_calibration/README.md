# Pacchetto di misura 1 — Calibrazione etichettata e vincolata

Pre-registrazione: [M2_5_Pkg1_Labeled_Onset_Calibration_Preregistration.md](../../M2_5_Pkg1_Labeled_Onset_Calibration_Preregistration.md). Risultati: [M2_5_Pkg1_Labeled_Onset_Calibration_Results.md](../../M2_5_Pkg1_Labeled_Onset_Calibration_Results.md).

- `acao02_v1_pkg1_2026-11-17.json`: manifest di `acao02_v1` con il blocco `labelled_calibration` (finestra, scelta della procedura precedente, griglia etichettata). Si rigenera con `cymatica_game --cacp-run acao02`, che scrive anche le liste per l'oracolo in `artifacts/cacp/acao02_v1_oracle/`.
- `oracle_mir_eval.py` e `oracle_mir_eval.output.txt`: prova V5. Si esegue con l'ambiente fissato di [`tools/pyenv`](../../../../tools/pyenv/requirements-oracle.txt).
