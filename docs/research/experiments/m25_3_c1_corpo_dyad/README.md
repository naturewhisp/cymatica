# M2.5.3–4 — Contratto minimo, punto 1: diade di Corpo (C1, C1b)

Pre-registrazioni: [C1](../../M2_5_3_C1_Corpo_Dyad_Preregistration.md) e [C1b](../../M2_5_3_C1b_Corpo_Dyad_Symmetric_Preregistration.md). Riferimento: il piano di F1 ([`fixture_intents_f1.csv`](../m25_3_f1_metric_anchor/fixture_intents_f1.csv), commit `97d07e7`).

## File

- **Piani:** `fixture_intents_c1.csv` e `fixture_intents_c1b.csv`, da `cymatica_game --cacp-run plan_export` allo stato del codice di ciascuna variante.
- **Script:**
  - `contradicted_bars.py <csv>`: battute in cui la voce alta di Corpo forma classe d'intervallo 1 o 6 con Vettore;
  - `role_clashes_path.py <csv>`: lo script `role_clashes.py` della baseline, con il percorso come argomento e la colonna `timbre` letta come numero.
- **Misure della baseline sullo stesso piano:**
  - `ctnctr_*.output.txt` e `vettore_corpo_registers_*.output.txt`, con gli script di [m25_ensemble_baseline](../m25_ensemble_baseline/);
  - `score_margins.output.txt`: vincitore, punteggio e margine sul miglior altro candidato valido, per frase. Prodotto con un test temporaneo che leggeva i trace, poi rimosso.

## Esiti

| | F1 | C1 | C1b |
|---|---|---|---|
| Battute contraddette (A, B, A′) | 8, 0, 8 (Si contro Fa) | 8, 0, 8 (Mi contro Fa) | **0, 0, 0** |
| Corpo–Vettore ic6 / ic1 in A | 3,1% / 0 | 0 / 3,1% | 0 / 0 |
| Vincitori diversi da F1 | — | frasi 0–3 | frasi 0–3 e 16–23 |

## Approfondimento: la melodia che cambia con C1b è migliore?

**Armonia della melodia: nessuna differenza.** CTnCTR, quota di note d'accordo e PCS di Vettore coincidono entro 0,01–0,06 in tutte le sezioni (`ctnctr_*`). Lo stesso vale per i tempi 1 e 3 e per i tempi 2 e 4. Il beneficio armonico di C1b viene tutto dalla diade, e la riparazione dopo la scelta (C1c) lo darebbe uguale.

**Registro: un vantaggio e un rischio.**
- **Il vantaggio:** in A′ Vettore passa da 50–57 a 74–81 MIDI. Con F1 era sotto la voce alta di Corpo in 127 attacchi su 128; con C1b è sopra in 127 su 127. Il dossier R-M25-7 lo segnala come difetto («Vettore non è mai la voce superiore in A e A′») ed è il punto 3 del contratto.
- **Il rischio:** a 74–81 Vettore sta dentro il registro di Trama (74–84). L'arpeggio e la melodia si sovrappongono, e la separazione fra ruoli non è stata misurata.

**Varietà: si perde.** In F1, A ha due ostinati: `A F D E` nelle frasi 0–3 (inversione del contorno) e `D F A G` nelle 4–7. Con C1b nelle frasi 0–3 vince lo spostamento d'ottava, e A suona `D F A G` in tutte le 31 battute contate, come A′. Il contrasto interno di A sparisce.

**Fragilità: la scelta è quasi casuale.** In A e A′ il vincitore di F1 supera il secondo di 400–425 punti su 100 000, cioè lo 0,4%. Con C1b i margini restano gli stessi (350–475), ma il vincitore è un altro. Una variazione minima di una caratteristica, qui la distanza di condotta delle parti, basta a cambiare la melodia.

**Lettura:**
- Il cambio di melodia non è un miglioramento casuale: è neutro sull'armonia, migliora il registro in A′ ma lo porta dentro Trama, e toglie varietà ad A.
- Soprattutto, non è stabile: il prossimo cambiamento del planner lo rovescerebbe.
- L'idea buona che contiene, Vettore sopra Corpo in A′, va presa come ipotesi dichiarata del punto 3, con la separazione da Trama misurata. Non va ereditata per caso.

**Nuovo rilievo:** nella fixture i margini in A e A′ sono sotto lo 0,5%. La scelta della melodia in quelle sezioni non è robusta, il che pesa sul rischio di varietà fittizia (R-M25-7) e sul punto 6 (seed oltre i pareggi).

## C1c, adottata

`fixture_intents_c1c.csv` e le relative uscite `*_c1c.output.txt`. Selezione identica a F1, diade riparata solo nel vincitore. Esiti nei [risultati](../../M2_5_3_C1_Corpo_Dyad_Results.md).
