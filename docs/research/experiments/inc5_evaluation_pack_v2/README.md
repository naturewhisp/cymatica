# Esperimento Inc-5 fase 5e.2: diversità fra seed, gruppi musicali, evaluation_pack_v2

Evidenze della fase 5e.2 ([ADR-0006](../../../adr/0006-m2-4-inc5-entry-decisions.md), precisazioni 5e-04, 5e-09, 5e-11 e DIF-5e.0-02). Generate in Release su `8671164` più il working tree di 5e.2.

- `seed_diversity_and_groups_2026-11-02.json`:
  - diversità fra i seed 1..64, con N = 64, k = 8 e metriche M1–M4. Esito: un solo contenuto, cioè `NULL_DIVERSITY`; la generalizzazione della calibrazione è `INCONCLUSIVE`;
  - indice del gruppo musicale di ogni frase del seed 1337: 14 gruppi su 24 frasi.

  Dati prodotti da `cymatica_game --cacp-run procedural_v1`.
- `evaluation_pack_v2_manifest_2026-11-02.json`: manifest dello scenario, con conteggi, impegno del seed, gruppi e `n_eff` atteso. Generato con `cymatica_game --cacp-run evaluation_pack_v2 --pack-seed 1618033988`.
- `v2_always_check_2026-11-02.json` e `v2_oracle_check_2026-11-02.json`: riepiloghi aggregati del checker (`presence_stats_v1`) per due valutatori di riferimento, la baseline banale «sempre presente» e l'oracolo uguale alla verità misurata. Non sono valutatori reali: delimitano la procedura dal basso e dall'alto.
- **Esclusi:** i file pubblici, la verità sigillata e gli elenchi per affermazione, che rivelerebbero la chiave del pacchetto.
