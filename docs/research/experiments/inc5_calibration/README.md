# Esperimento Inc-5 fase 5c — calibrazione e verifica degli onset per ruolo

Dati del dossier [M2_4_Inc5_AC-AO-02_Calibration.md](../../M2_4_Inc5_AC-AO-02_Calibration.md).

- `acao02_v1_2026-10-29.json`: manifest completo dello scenario `acao02_v1`. Contiene:
  - latenza intrinseca (set A);
  - griglia di calibrazione sul seed 1337;
  - profilo calibrato e congelato con SHA-256;
  - scansione dei seed 1–32;
  - verifica su `planner_probe_v1` e sul set sintetico B;
  - diagnostica dei rilasci;
  - ricostruzione del mix e presenza della Trama.

  Si rigenera con `cymatica_game --cacp-run acao02_v1` (Release), con `build_id` `83529f0` più il working tree della fase 5c.
- `acao02_v1_2026-10-29_dif5c.json`: lo stesso scenario dopo la remediation DIF-5c-01/02 (`build_id` `2d0eaf2` più il working tree della remediation). Le misure coincidono con il file precedente; si aggiungono `inconclusive_reasons`, `intrinsic_captures_valid`, `capture_valid` e `analysis_ok` della calibrazione, `verification_status`, `invalid_capture` per insieme, `proposed_thresholds_status` e `reconstruction_status`.
- **Sorgente:** [acao02_calibration.cpp](../../../../engine/cacp/acao02_calibration.cpp) e [stem_analysis.cpp](../../../../engine/cacp/stem_analysis.cpp).
- **Esclusi:** i WAV degli stem (circa 356 MB) non sono versionati.
