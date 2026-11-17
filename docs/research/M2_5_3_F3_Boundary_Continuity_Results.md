# M2.5.3–4 — F3, continuità dei pattern ai confini: risultati

- **Pre-registrazione:** [M2_5_3_F3_Boundary_Continuity_Preregistration.md](M2_5_3_F3_Boundary_Continuity_Preregistration.md), SHA-256 (LF) `9e33b75d07323eeca8f537a03731d1d211ae033d20a2375bf99155c6cdf34f08`.
- **Esecuzione, 2026-11-17:** passo 0 e passo 1 misurati separatamente con il nuovo scenario `plan_export_v1`; Release e Debug, `development_measures_v1`, `full_m2_suite`. Dati: [esperimento](experiments/m25_3_f3_boundary/README.md).
- **Decisione:** policy del planner 1.0.2.0 e continuità dei pattern di Pulso e Trama nella prima frase di ogni sezione (D-M2-39).

## 1. Esiti

| ID | Predizione | Risultato | Esito |
|---|---|---|---|
| P0 | Dopo il passo 0 intent identici; cambiano solo il trace e gli hash che lo includono | Intent identici byte per byte; PCM invariato (`15689200359660949538`); cambiano gli hash del trace e del piano | Confermata |
| P1 | Cambia solo la frase 8 (e al più 16, 9, 17) | **Solo la frase 8**, e in essa solo Pulso e Trama. Corpo, Vettore e le NoteOff esplicite restano identici | Confermata |
| P2 | Prima battuta della frase 8 = ultima della frase 7 per Pulso e Trama | Trama: 2, 5, 7, 9, 14 in entrambe. Pulso: 0, 4, 8, 12 in entrambe. Prima del cambio: Pulso 2, 6, 10, 14 e Trama 0, 3, 5, 7, 12 | Confermata |
| P3 | Novità di Corpo con i picchi alle frasi 9 e 17 | 9 e 17 con M4, M5 e M5r | Confermata |
| P4 | Fixture PASS con i golden aggiornati; `meter_change_v1` PASS e **invariato** | Fixture PASS. `meter_change_v1` resta PASS in tutti i controlli M1–M6, ma il suo PCM cambia | **Mancata in parte** |

**Nota su P2:** nella finestra di una battuta della frase 8 compare anche un attacco di Pulso al passo 16. È il battere della battuta successiva: cade al frame 82285, mezzo frame prima del confine teorico (329142 / 4 = 82285,5 frame), per l'arrotondamento a frame interi. Il pattern della battuta resta 0, 4, 8, 12.

**Perché P4 manca:** anche la forma dello scenario del metro ha più sezioni, quindi la prima frase in 3/4 eredita i pattern della frase precedente in 4/4. I pattern euclidei si ricalcolano sui 12 step della battuta di 3/4 e restano sulla griglia: nessun fallback, tutte le note sulla griglia di semicrome. Lo stesso vale per la forma di prova A→B del planner. Non l'avevo previsto: la continuità è una regola del planner, non della fixture.

## 2. Deviazione dal testo pre-registrato

La pre-registrazione diceva di portare `DEFAULT_POLICY_VERSION_ID` a 1.0.2.0. Quella costante però alimenta anche RunRecord e SeedBank: cambiarla avrebbe toccato sistemi fuori dal planner. Ho introdotto invece una costante del solo planner, `kPlannerPolicyVersion` = 1.0.2.0, e lasciato invariato il default globale (1.0.1.0). P0 vale comunque: gli intent sono identici.

## 3. Golden aggiornati, con il motivo nel codice

| Golden | Prima | Dopo |
|---|---|---|
| Trace della fixture | 10949526468471789087 | 10233665394142753888 |
| Piano della fixture | 0x4f021cb9e8637598 | 0xb91ce72feeb37bc7 |
| PCM della fixture | 15689200359660949538 | 15914153099858413724 |
| Piano della forma di prova A→B | 0x2983fc5a5463d4cb | 0x44c54bd1ac626c3d |
| PCM di `meter_change_v1` | 4922831055463163399 | 58770584194502935 |

**Test:** gli helper usano ora per default la policy del planner (`kPlannerPolicyVersion`). Il controllo «una policy diversa cambia gli identificativi» confronta con la 1.0.1.0 del planner di M2.4, e il trace continua a dichiarare la policy con cui è stato prodotto.

## 4. Lettura e seguito

- Al confine A→B cambiano ancora tempo, modo, motivo e Corpo, ma Pulso e Trama proseguono per una frase. È l'indicazione della proposta d'ingresso, applicata come regola generale.
- **Se A→B suoni meno brusca** lo dirà l'ascolto cumulativo (registro delle domande, voce 8). Se non basta, il seguito previsto è la rampa di tempo a gradini (consolidamento §6.4, CP-3 o CP-4).
- **Scenario nuovo `plan_export_v1`:** esporta gli intent della fixture in CSV, per confrontare i piani prima e dopo ogni cambio del planner nei pacchetti successivi.

## 5. Build

Release e Debug headless `/WX` 296/296; `full_m2_suite` verde salvo `acao02_v1`, il limite noto.
