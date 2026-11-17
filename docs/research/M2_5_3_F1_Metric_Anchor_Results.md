# M2.5.3–4 — F1, àncora metrica dichiarata: risultati

- **Pre-registrazione:** [M2_5_3_F1_Metric_Anchor_Preregistration.md](M2_5_3_F1_Metric_Anchor_Preregistration.md), SHA-256 (LF) `38ee0e1cee6c90f01e8fe0edf68220bcf3a2adca9c9448581661dfd22abe1125`.
- **Esecuzione, 2026-11-17:**
  - test `[metric_anchor]`;
  - `plan_export_v1` confrontato con il piano di F3;
  - `development_measures_v1`, `meter_change_v1`, `full_m2_suite`;
  - Release e Debug.

  Dati: [esperimento](experiments/m25_3_f1_metric_anchor/README.md).
- **Decisione:** stato metrico dichiarato per frase e profilo iniziale `fixture-metric-v1` (D-M2-40).

## 1. Cosa cambia

Ogni frase dichiara lo stato `Ancorato`, `Spostato` o `Ritorno` e uno spostamento di Pulso in passi. La rotazione di Pulso è lo spostamento dichiarato in `Spostato`, altrimenti 0, e non dipende più dall'indice del candidato. Una dichiarazione incoerente è una violazione strutturale del piano.

Nella fixture lo spostamento è di un ottavo, e vale solo per le frasi 12 e 13 di B. La frase 14 è il ritorno.

| Frasi (da 0) | Pulso prima, prima battuta | Pulso dopo |
|---|---|---|
| 0–3 | 2, 5, 7, 10, 13, 15 | 0, 3, 5, 8, 11, 13 |
| 4–8 | 0, 4, 8, 12 | invariato |
| 9–11, 14–23 | 2, 6, 10, 14 | 0, 4, 8, 12 |
| 12–13 (`Spostato`) | 2, 6, 10, 14 | invariato |

Il kick evitava il tempo 1 in 19 frasi su 24; ora lo evita in 2, quelle dichiarate.

## 2. Esiti

| ID | Predizione | Risultato | Esito |
|---|---|---|---|
| P1, R-MET-1 | Rotazione di Pulso ≠ 0 solo nelle frasi 12–13, uguale a 2 | Sì; nessun attacco sul passo 0 in 12–13 | Confermata |
| P2, R-MET-2 | Nelle frasi non `Spostato`, attacco sul passo 0 in ogni battuta e fase 0 massima (o pari) | Sì, in tutte le 22 frasi e in ogni battuta. Con 6 impulsi (frasi 0–3) le fasi 0, 1 e 3 sono pari (2 attacchi ciascuna) | Confermata |
| P3, nessun effetto collaterale | Vincitori invariati, 0 pareggi; Corpo, Trama e Vettore identici | Vincitori invariati, 0 pareggi; Corpo e Vettore identici, NoteOff identiche. **Trama cambia** nelle frasi 9–11 e 14–23 | **Mancata in parte** |
| P4, dove cambia Pulso | Cambia in 0–3, 9–11, 14–23; identico in 4–8, 12–13 | Esattamente così | Confermata |
| P5, sezioni | Picchi di novità di Corpo alle frasi 9 e 17 | 9 e 17 con M4, M5 e M5r; M5 e M5r ancora ammesse | Confermata |
| P6, scenari | Fixture PASS con golden aggiornati; `meter_change_v1` PASS con PCM aggiornato; golden della forma di prova cambiato | Fixture PASS, golden aggiornati; forma di prova cambiata. **`meter_change_v1` resta PASS con il PCM invariato** | **Mancata in parte** |

**Perché P3 manca.** Trama è scritta come sincope complementare: salta i passi in cui suona Pulso. Quando il kick torna sul tempo, Trama perde i passi 0 e 12 e riprende il 10 e il 14. Nella prima battuta della frase 9 passa così da 0, 3, 5, 7, 12 a 3, 5, 7, 10, 14. Il legame era nel codice, ma nella pre-registrazione l'avevo cercato solo nelle caratteristiche del punteggio. Il cambiamento è coerente con la regola di Trama, quindi non è un effetto da correggere. I vincitori non cambiano perché le caratteristiche del punteggio restano uguali.

**Perché P6 manca.** Il PCM di `meter_change_v1` è identico byte per byte, quindi nessun suo vincitore aveva una rotazione di Pulso diversa da 0. La predizione era prudente, non informata.

## 3. Conseguenze non predette, registrate

- **Gruppi simbolici della fixture (ADR-0006 5e.3c):** da 8 a 9. Le frasi 12–13 si separano da B, mentre la frase 14, il ritorno, resta nel gruppo delle 9–11. La partizione attesa nel test è aggiornata con la storia; i gruppi per frame restano 14.
- **Policy:** resta 1.0.2.0, la versione del planner per la fase (D-M2-39), come pre-registrato.

## 4. Golden aggiornati, con il motivo nel codice

| Golden | Prima | Dopo |
|---|---|---|
| Trace della fixture | 10233665394142753888 | 17158827938453439384 |
| Piano della fixture | 0xb91ce72feeb37bc7 | 0x7aa477a5519f9cf8 |
| PCM della fixture | 15914153099858413724 | 8239929959925757461 |
| Piano della forma di prova A→B | 0x44c54bd1ac626c3d | 0x6d73bd838ab32526 |
| PCM di `meter_change_v1` | 58770584194502935 | invariato |

## 5. Lettura e seguito

- **Ascolto cumulativo:** la voce 7 del registro è riformulata.
  - Si batte il piede a tempo in ogni parte?
  - Lo spostamento in B si sente come una scelta o come un errore?
  - La posizione e la durata dello spostamento sono valori d'autore iniziali, non validati.
- **Regole non coperte:**
  - restano fuori R-MET-3 (punteggio di fase pesato), R-MET-4 (banda di sincope) e R-MET-7 (sincope anticipatoria);
  - la regola complementare di Trama ora dipende dallo stato metrico: se in futuro servisse che Trama resti fissa nello spostamento, sarebbe una variabile da dichiarare.
- **Forme senza dichiarazione:** sono ancorate per default, come la forma di prova e quella del metro. Il direttore adattivo di M3 dovrà dichiarare lo stato.

## 6. Build

Release e Debug headless `/WX` 298/298, con 2 test nuovi (`[metric_anchor]`). `full_m2_suite` è verde salvo `acao02_v1`, il limite noto.
