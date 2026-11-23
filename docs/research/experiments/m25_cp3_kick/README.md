# CP-3, I1–I2: kick

[Pre-registrazione](../../M2_5_CP3_I1_I2_Kick_Preregistration.md), [risultati](../../M2_5_CP3_I1_I2_Kick_Results.md).

- Clip: `cymatica_game --cacp-run kick_presence_ab_v1` (non versionate). Manifest: `kick_presence_ab_v1_manifest.json`.
- Piano: `cymatica_game --cacp-run plan_export`.
- `kick_presence.py --clips artifacts/cacp/kick_presence_ab_v1 --plan artifacts/cacp/plan_export_v1/fixture_intents_seed1337.csv` scrive `kick_presence.json` (misure e selezione). Le sezioni A, B, C del JSON sono A, B e A′.
