# CP-3, I3: indagine sul basso

[Pre-registrazione](../../M2_5_CP3_I3_Bass_Investigation_Preregistration.md), [risultati](../../M2_5_CP3_I3_Bass_Investigation_Results.md).

`bass_investigation.py` legge i MIDI e i WAV dell'esperto (fuori dal repository, [decodifica](../expert_production_analysis/README.md)), gli stem di `kick_presence_ab_v1` e il piano di `plan_export`, e scrive `bass_investigation.json`. Le sezioni A, B, C del JSON sono A, B e A′.

## Prototipo a tre versioni

[Pre-registrazione](../../M2_5_CP3_I3_Bass_Prototype_Preregistration.md), [risultati](../../M2_5_CP3_I3_Bass_Prototype_Results.md).

1. `bass_prototype.py write` scrive le linee V2 in CSV;
2. `cymatica_game --cacp-run bass_prototype_v1` rende V0, V1 e le linee;
3. `bass_prototype.py measure --clips <cartella>` scrive `bass_prototype.json` e le clip allineate.
