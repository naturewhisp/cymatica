# M2.5.2 — Rapporto FM della Trama

Pre-registrazione: [M2_5_2_FM_Ratio_Preregistration.md](../../M2_5_2_FM_Ratio_Preregistration.md). Risultati: [M2_5_2_FM_Ratio_Results.md](../../M2_5_2_FM_Ratio_Results.md).

- `fm_ratio_context.py` e `fm_ratio_context.output.txt`: criteri C1–C3 sul modello di Sethares, calcolati sulle 400 note Trama della fixture (seed 1337) e sulle note di Corpo e Vettore che suonano al loro attacco. Ingresso: `../cacp_methods/fixture_intents_seed1337.csv`. Comando: `python -I fm_ratio_context.py ../cacp_methods/fixture_intents_seed1337.csv`.
- `procedural_v1_fm11_2026-11-16.json`: manifest di `procedural_v1` dopo l'adozione (rapporto 1:1, Trama −9 dB): loudness per stem, true peak, limiter.
- `acao02_v1_fm11_2026-11-16.json`: manifest di `acao02_v1` dopo l'adozione. Mostra il profilo calibrato diverso da quello congelato (`INCONCLUSIVE`) e la griglia della Trama, da confrontare con [`inc5_calibration/acao02_v1_2026-10-29_dif5c.json`](../inc5_calibration/acao02_v1_2026-10-29_dif5c.json).
