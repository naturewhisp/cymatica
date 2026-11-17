# M2.5.2 — Livello della Trama

Pre-registrazione: [M2_5_2_Trama_Level_Preregistration.md](../../M2_5_2_Trama_Level_Preregistration.md). Risultati: [M2_5_2_Trama_Level_Results.md](../../M2_5_2_Trama_Level_Results.md).

- `stem_export_v3_2026-11-17.json`: manifest dell'esportazione (81 WAV float in `artifacts/cacp/stem_export_v3/`, non versionati), con V1 e la loudness BS.1770 degli estratti del mix per offset. Si rigenera con `cymatica_game --cacp-run stem_export_v3`.
- `trama_level_criterion.py`, `trama_level_criterion_2026-11-17.json` e `trama_level_criterion.output.txt`: criterio di presenza, subordinazione e sicurezza con MoSQITo (circa 20 minuti), nell'ambiente fissato `tools/pyenv`.
