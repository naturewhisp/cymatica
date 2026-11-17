# M2.5.3–4 — F3, continuità dei pattern ai confini

Pre-registrazione: [M2_5_3_F3_Boundary_Continuity_Preregistration.md](../../M2_5_3_F3_Boundary_Continuity_Preregistration.md). Risultati: [M2_5_3_F3_Boundary_Continuity_Results.md](../../M2_5_3_F3_Boundary_Continuity_Results.md).

- `fixture_intents_policy1010.csv`: intent della fixture (seed 1337) prima di ogni cambio, policy del planner 1.0.1.0.
- `fixture_intents_policy1020_step0.csv`: dopo il passo 0 (policy 1.0.2.0), **identico byte per byte** al precedente (P0).
- `fixture_intents_policy1020_f3.csv`: dopo il passo 1 (continuità); cambia solo la frase 8 (P1).
- I tre file si producono con `cymatica_game --cacp-run plan_export` (nuovo scenario `plan_export_v1`), allo stato del codice di ciascun passo.
