# M2.5.3–4 — Contratto minimo, punto 5b: operazioni pensate per la catena. Pre-registrazione

- **Stato:** scritto il 2026-11-17, prima del codice. L'hash SHA-256 (LF) è registrato in `progress.md`. Dopo l'hash il file non si modifica.
- **Decisione del titolare:** «Punto 5b» (2026-11-17), dopo i [risultati del punto 5](M2_5_3_C5_Derived_Variation_Results.md).
  - La catena con le tabelle attuali alterna due figure: l'inversione del contorno è quasi un'involuzione.
  - K4 non è stato superato: il genitore è più vicino della mediana in 5 derivazioni su 12.
  - Il seguito indicato era usare operazioni pensate per la catena.
- **Informata dai dati:** nasce dall'esito del punto 5 sulla stessa fixture, e lo dichiaro.
- **Decisioni collegate:** D-M2-45 (catena con limiti), D-M2-08 (tabelle delle ricette per sezione), D-M2-39 (policy 1.0.2.0).

## 1. Variabile

1. **Due trasformazioni nuove**, sempre dentro la battuta del motivo, con gli ancoraggi conservati e verificate da `verifyMotifTransformInvariants`:
   - **`PassingOrnament`** (parametro: indice della nota). La nota scelta, o la prima nota idonea dopo di essa con durata ≥ 2 passi, si divide in due metà: la prima conserva altezza, ancoraggio e attacco; la seconda diventa una **nota di passaggio**, non d'ancoraggio e senza alterazione. La nota di passaggio sta un grado verso la nota seguente, oppure, se le due note distano meno di 2 gradi, un grado sopra (volta superiore). L'ultima nota guarda alla prima, che riprende alla battuta seguente. Se nessuna nota è idonea, o il motivo ha già 8 note, il motivo resta invariato.
   - **`Liquidation`.** Toglie l'ultima nota non d'ancoraggio con indice > 0 e allunga la nota precedente fino a coprirne la durata. Se il motivo ha 2 note o nessuna nota removibile, resta invariato.
2. **Tabelle di catena:** le frasi con `motifSource = PreviousWinner` usano tabelle proprie, che sostituiscono quelle di sezione solo per quelle frasi. L'inversione non c'è più.

   | Sezione | Tabella di catena (8 ricette) |
   |---|---|
   | A | Identità; Ornamento 0, 1, 2, 3; Liquidazione; Trasposizione +1, −1 |
   | B | Trasposizione +1, −1; Ornamento 0, 1, 2, 3; Liquidazione; Trasposizione +2 |
   | A′ | Identità; Ornamento 0, 1, 2, 3; Liquidazione; Variazione ritmica 1; Identità |

   B resta senza identità, come in D-M2-08. A′ resta senza trasposizione, per conservare la ripresa.
3. **Profilo di punteggio:** `transformTensionMilli` aggiunge i due tipi, Ornamento **180** e Liquidazione **50**. Il primo aggiunge moto, il secondo dirada. I valori dei tipi esistenti non cambiano. Il profilo diventa `planner-scoring-v2-experimental`, versione **2**.
4. Limite di deriva, memoria e ritorni restano quelli del punto 5.

## 2. Criteri e regola d'adozione

Misure con lo scenario `lineage_measures_v1` (M5r su Vettore) e con `vettore_groups.py`, equivalente alla distanza 0.

| ID | Criterio | Superato se |
|---|---|---|
| **L1, struttura** | Origini come dichiarato, limite rispettato, invarianza del motivo superata | Tutto vero |
| **L2, contratto precedente** | Come K2 del punto 5 | 0 battute contraddette; 25/25 note dell'accordo sui cambi; 0 conflitti Vettore–Corpo; nessun `vettoreRegisterUnmet`; R-MET-2 |
| **L3, niente alternanza** | Frasi in catena senza ripartenza in cui il contenuto di Vettore è uguale a quello della frase n−2 e diverso dalla n−1 | Al massimo il 25% (con il punto 5 erano la maggioranza in A e A′) |
| **L4, ricorda** | Criterio K4 del punto 5 | Almeno il 75% |
| **L5, sviluppa** | Gruppi di Vettore, e un percorso cumulativo | Più di 5 gruppi, e almeno un percorso di 3 frasi in catena consecutive, ognuna con un contenuto mai sentito prima nella sezione |
| **L6, ritorna** | Criterio K5 del punto 5 | Superato |
| **L7, sezioni** | Novità di Foote su Corpo | Picchi alle frasi 9 e 17 |
| **L8, scenari** | Fixture e `meter_change_v1` | PASS, con i golden aggiornati col motivo |

**Adozione:** si adotta se superano L1, L2, L3, L4, L5, L7 e L8; L6 si riporta. Altrimenti l'esito è negativo, e il piano resta quello del punto 5.

**Predizioni qualitative:**
- il punteggio può preferire l'identità e la liquidazione, perché tendono a ridurre la tensione stimata;
- se la catena si ferma sull'identità, L5 non passa, e lo si registra come esito negativo.

## 3. Limiti

- Le due operazioni sono scelte d'autore iniziali. Le durate si dimezzano soltanto, non c'è sincope.
- I valori di tensione 180 e 50 sono iniziali e non derivati da dati.
- Se la variazione suoni come sviluppo lo dice solo l'ascolto (prove B2–B5).
