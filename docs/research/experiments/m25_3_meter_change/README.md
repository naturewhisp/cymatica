# M2.5.3–4 — Scenario tecnico del cambio di metro (`meter_change_v1`)

Pre-registrazione: [M2_5_3_Meter_Change_Scenario_Preregistration.md](../../M2_5_3_Meter_Change_Scenario_Preregistration.md).

- `meter_change_v1_2026-11-17.json`: manifest della prima esecuzione, verdetto PASS con M1–M6. Si rigenera con `cymatica_game --cacp-run meter_change` (Release, meno di un secondo); fa parte di `full_m2_suite` come voce vincolante, e il test `[meter_change]` lo esegue in CI.
- **Golden PCM dello scenario:** `4922831055463163399`, registrato alla prima esecuzione in `meter_change.h`.
