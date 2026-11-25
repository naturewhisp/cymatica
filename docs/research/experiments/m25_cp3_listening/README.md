# CP-3: ascolto di conferma dell'esperto

[Disegno e ipotesi](../../M2_5_CP3_Expert_Listening_Hypotheses.md), hash in `progress.md`.

1. `cp3_material.py write`, poi `cymatica_game --cacp-run warped_plan_render_v1` (i file `k3_*` suonano con il kick Do2 e click +3 dB, solo offline), poi `cp3_material.py measure`: scrive `cp3_material.json` e il brano Z.
2. `make_cp3_package.py --date <data> --commit <sha> --checks cp3_package_checks.json`: pagina, audio e chiave in `artifacts/expert_listening/` (non versionati). La chiave `_NON_INVIARE` resta a chi conduce.

Servono i render delle prove precedenti in `artifacts/cacp/` (`tempo_ramp_v1_i5d`, `_i5e`, `_i46`, `_i46b`, `bass_prototype_v1`).

- Risposte dell'esperto: [esito](../../../m2_5_cp3_expert_listening_2026-11-23.md), trascrizione `risposte_2026-11-23_esperto.txt`; controlli post hoc `verify_answers.py` e `.json`.

- Secondo ascolto: `make_cp3b_package.py` ([ipotesi](../../M2_5_CP3_Expert_Listening_2_Hypotheses.md)), controlli `cp3b_package_checks.json`. Risposte dell’esperto del 2026-11-25: [esito](../../../m2_5_cp3_expert_listening_2026-11-25.md), trascrizione `risposte_2026-11-25_esperto.txt`, controlli post hoc `verify_answers_2.py` e `.json`.
