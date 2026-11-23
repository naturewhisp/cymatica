# M2.5 CP-3 — I1–I3, kick e basso insieme: risultati

- **Pre-registrazione:** [documento](M2_5_CP3_I1_I3_Kick_Bass_Joint_Preregistration.md), SHA-256 (LF) `94b3bcc13ec192a667163e6b65808397e6f479a1de929f496b7573796fbf1a0a`.
- **Esecuzione:** 2026-11-23. **Dati:** `experiments/m25_cp3_bass/kick_bass_joint.{py,json}` sui render di `bass_prototype_v1`. Nessun nuovo render, nulla adottato.
- **Esito della regola: non soddisfatta** (J6). Il fallimento è di loudness del mix, non di mascheramento.

## 1. Criteri (kick c2)

| ID | Esito |
|---|---|
| J1 | 6·10⁻⁵: confermata |
| J2 | Energia bassa non di kick di V2m e V1 uguale per sezione: confermata (per costruzione). Guadagno di V2m: +0,34 / −2,09 / −0,18 dB sulla linea grezza |
| J3, P_body ≥ 6 dB, A / B / A′ | V1 10,4 / 10,3 / 10,4; V2m **7,9** / 9,6 / 8,8: confermata. Previsione 8–15 dB **mancata di 0,1 dB** per V2m in A |
| J4, R sui soli kick coincidenti ≥ 6 dB | V1 15,7 / 15,7 / 15,6; V2m 12,4 / 15,4 / 13,2: confermata. La parte di previsione «più basso della mediana su tutti i kick» **non misurata** |
| J5, P_click ≥ 0 dB, calo ≤ 0,5 dB da V0 | V1 e V2m: A 0,7 / 0,6, B 8,2 / 7,6, A′ 1,0 / 1,0 (V0 0,6 / 7,8 / 1,0): confermata |
| **J6** | Picchi −6,05 e −5,20 dBFS: rispettati. **Loudness: V1 −19,63 e V2m −18,90 LUFS, 0,73 LU di distanza (> 0,5): FALLITA** |
| J7 | E_K A e B a 0,02 dB; Q100 0,754 / 0,761 / 0,759: confermata |

## 2. Lettura

- **Il kick resta leggibile anche con il basso:** lo scarto scende da 23,6 a 10,4 dB (V1) e 7,9 (V2m) in A, sopra la soglia. Con il kick «now» i valori sono simili (nel json).
- **V2m è più forte di V1 a pari energia bassa.** Il basso autonomo si somma a Corpo; V1 sposta la voce bassa di Corpo un'ottava sotto e nel mix perde loudness. Pareggiare le basse **e** la loudness insieme non è possibile con un solo guadagno. Per l'ascolto gli estratti si allineano in loudness (attenuando V2m di circa 0,7 dB, basse comprese): dichiararlo.
- **Margini stretti:** P_body 7,9 in A (V2m) e Q100 0,754. Soglie ragionate, non validate.
- **V1 nella J4** è misurata sull'intero stem Corpo, non sulla sola voce bassa, che non è separabile: il suo R non si confronta alla pari con quello di V2m.

## 3. Difetto trovato e corretto

La prima esecuzione divideva le sezioni arrotondando i campioni: il primo kick di A′ cadeva in B, e P_body di V0 in A′ risultava 32,4 invece di 23,6. Il confronto con le misure di I1–I2 (23,4) l'ha scoperto. Ho corretto il calcolo (troncamento, come il planner) e rieseguito. **Nessun esito cambia**; nessuna soglia è stata toccata. Le misure del prototipo del basso usano la stessa divisione arrotondata: scarto di un kick, da non correggere retroattivamente, ma da tenere a mente.

## 4. Che cosa resta

- Il vincolo J6 era una mia scelta di criterio, presa senza considerare che V1 e V2m spostano energia in modo diverso. **Non lo riscrivo ora.** Una nuova regola (per esempio pareggio su loudness con basse dichiarate) richiede una nuova pre-registrazione.
- Combinazione con c di I4+I6: gli attacchi di Pulso cambiano, J4 andrà ricalcolata.
