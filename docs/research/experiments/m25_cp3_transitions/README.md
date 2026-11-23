# CP-3, I5: indagine sui raccordi

[Pre-registrazione](../../M2_5_CP3_I5_Transition_Investigation_Preregistration.md), [risultati](../../M2_5_CP3_I5_Transition_Investigation_Results.md).

`transition_investigation.py` legge i brani di `cumulative_listening_v1`, il mix dell'esperto decodificato (fuori dal repository) e il piano di `plan_export`, e scrive `transition_investigation.json`.

## Prototipo della rampa di tempo

[Pre-registrazione](../../M2_5_CP3_I5_Tempo_Ramp_Prototype_Preregistration.md), [risultati](../../M2_5_CP3_I5_Tempo_Ramp_Prototype_Results.md). `tempo_ramp.py write` sposta nel tempo gli eventi del piano, `cymatica_game --cacp-run warped_plan_render_v1` li rende col motore reale, `tempo_ramp.py measure` verifica e prepara gli estratti ciechi. Estratti e chiave non sono versionati.

## Ingresso di A′ (I5b)

`aprime_entry.py` (stesso schema `write`, `warped_plan_render_v1`, `measure`): [pre-registrazione](../../M2_5_CP3_I5b_BAprime_Harmony_Preregistration.md), [risultati](../../M2_5_CP3_I5b_BAprime_Harmony_Results.md). Prima di `write` la cartella non deve contenere altri CSV.

## Ingresso di A′ senza salto (I5c)

`melody_approach.py`, stesso schema: [pre-registrazione](../../M2_5_CP3_I5c_Melody_Approach_Preregistration.md), [bis](../../M2_5_CP3_I5c_Melody_Approach_Preregistration_bis.md), [risultati](../../M2_5_CP3_I5c_Melody_Approach_Results.md).

## Ingresso basso e ritorno raccordato (I5d)

`register_return.py` (stesso schema, con `--out`): [pre-registrazione](../../M2_5_CP3_I5d_Register_Return_Preregistration.md), [risultati](../../M2_5_CP3_I5d_Register_Return_Results.md).
