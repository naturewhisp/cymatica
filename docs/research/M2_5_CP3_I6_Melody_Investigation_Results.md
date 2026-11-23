# M2.5 CP-3 — I6, melodia: risultati dell'indagine

- **Pre-registrazione:** [documento](M2_5_CP3_I6_Melody_Investigation_Preregistration.md), SHA-256 (LF) `9e763b537ad4ef4ed6ff572b54ceeb83d97b8411d88cff2a48d83ca1d90c17b1`.
- **Esecuzione:** 2026-11-23. **Dati:** [esperimento](experiments/m25_cp3_melody/README.md). Nessun cambiamento alla musica.

## 1. Esiti

| ID | Riferimento | Esito |
|---|---|---|
| M1, stesso contorno della battuta 1 | **89 %** (stessa figura esatta 61 %) | Confermata |
| M2, mediana 64 seed | 89 % (86–99 %) | Confermata |
| M3, contorni diversi per sezione | 5, 12, 5: media 7,3 | **Mancata** (≤ 6); seed 34: 4, 2, 4 |
| M4, contenuto informativo, seconda metà meno prima | −1,16; −1,06; −0,004 bit | Confermata di stretta misura: A′ è praticamente piatta |
| M5, frasi che finiscono sulla tonica | pari 0 %, dispari **0 %** | Confermata |
| M6, frasi con respiro in battuta 4 | 0 (0 su 1536 nei seed) | Confermata |

**Riserva su M4:** un modello che impara dal brano trova per forza più prevedibile ciò che viene dopo. Il calo non dimostra noia; dice solo che la seconda metà riusa la prima.

## 2. Lettura

1. **La melodia sale e scende con gli accordi, ma la figura resta la stessa:** stesso ritmo (I4) e stesso contorno in quasi nove battute su dieci. Spiega «una figura che si ripete» meglio della sola altezza.
2. **Non si chiude mai:** nessuna frase del riferimento finisce sulla tonica, neppure l'ultima del brano, e nessuna respira. Manca la differenza fra domanda e risposta.
3. **M3 mancata è informativa:** B usa 12 contorni e A solo 5. La varietà c'è **fra** le frasi di B, non dentro.

## 3. Leva proposta (I4 e I6 insieme)

Motivo di **due battute**: *a* come oggi, *a′* che ne conserva la testa e cambia la coda, quindi contorno e ritmo diversi. Alla fine della frase:
- **domanda** (frasi pari): coda sospesa, su un grado dell'accordo diverso dalla tonica;
- **risposta** (frasi dispari): coda sulla tonica, con un attacco in meno, cioè un respiro.

*a′* deriva da *a* con operazioni già verificate (inversione, liquidazione, ornamento): niente varietà fittizia.

**Come si misurerà:** M1 sotto il 60 %; G1 di I4 sotto il 60 %; M5 con differenza ≥ 50 punti; almeno 8 respiri; contratti dei punti 1–4 e lignaggio K4 invariati.

**Da decidere prima del codice:** se la frase pari risponde alla dispari o viceversa, e come convivono la catena dei motivi (D-M2-45) e il motivo doppio.

## 4. Limiti

- Nessun corpus umano: quanto contorno ripetuto sia «troppo» non è noto.
- Il modello a breve termine non è IDyOM. Installarlo con un corpus resta possibile, ma qui non cambierebbe la diagnosi simbolica.
- Finire fuori dalla tonica può essere una scelta modale. Qui non è una scelta: nessuna regola la esprime.

## 5. Aggiunte dopo i dati

- **Finali di frase:** nei 64 seed la tonica chiude lo 0,5 % delle frasi e un grado dell'accordo di tonica il 55 %. L'ultima nota del brano è sempre Sol o Fa, mai Re: la melodia non conclude.
- **Contro-evidenza da pesare:** il seed 34, **preferito da entrambi gli ascoltatori**, è più ripetitivo del riferimento su queste misure (contorni 4, 2, 4 contro 5, 12, 5; contenuto informativo 1,41 contro 2,03 bit; finali di frase tutti uguali dentro ogni sezione). Ridurre la ripetizione misurata, quindi, **non garantisce** che piaccia di più. Due ascoltatori non bastano a separare le cause (giro con la sensibile, cella di AURA), ma la leva va provata all'ascolto e non adottata sulla sola misura.
