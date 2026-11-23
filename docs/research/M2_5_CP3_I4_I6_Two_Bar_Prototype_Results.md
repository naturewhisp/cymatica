# M2.5 CP-3 — I4+I6, prototipo dei motivi di due battute: risultati

- **Pre-registrazione:** [documento](M2_5_CP3_I4_I6_Two_Bar_Prototype_Preregistration.md), SHA-256 (LF) `27106e98679cecb2075c92200b0c33c94881482dbfc4870c3612bb325e68dc19`.
- **Esecuzione:** 2026-11-23. **Dati:** `experiments/m25_cp3_melody/two_bar_prototype.{py,json}`. Solo offline, seed 1337; nulla adottato.

| ID | Esito |
|---|---|
| Z1, Z2 | p entro 0,0002; cambiano solo i ruoli previsti, teste identiche, scala rispettata: confermate |
| Z3 | Stesso contorno della battuta 1: 89 % → **33 %**: confermata |
| Z4 | Stesso ritmo della battuta precedente: 100 % → **0 %** (Vettore in m e b; Pulso e Trama in t e b): confermata |
| Z5 | Tonica a fine frase: pari 0 %, **dispari 100 %** (oggi 0 % e 0 %): confermata |
| Z6 | Respiri: 0 → **12**: confermata |
| Z7 | Conflitti 0; D-M2-43 mantenuta in tutte le frasi (minimo 0,59): confermata |
| Z8 | MEASURED, limiter 0 dB, nessuno scarto; loudness entro 0,07 LU: confermata |

**Peso:** Z3–Z6 erano quasi garantite dalla costruzione: mostrano che la leva si può fare, non che sia migliore. Contenuto informativo medio: 2,03 → 2,32 bit (descrittivo).

**Osservato dopo, non previsto:**
- 14 note nuove della coda in conflitto con Corpo restano all'altezza di oggi.
- Salti di melodia oltre 12 semitoni: oggi 4 (battute 30, 52, 64, 94), in m 3 (32, 52, 64). Ma la risposta finale di A è Re6: il salto **A→B passa da −12 a −19 semitoni** (battuta 32); B→A′ scende da 19 a 14. Il passaggio A→B, già critico in I5, peggiora: da risolvere prima di proporre m.
- La regola della tonica preferisce le note sopra Corpo e può finire molto in alto (battuta 31).
- Nella forma a b a c la domanda e la risposta tornano simili: il prototipo non sviluppa fra le frasi più di oggi.

**Non verificati:** K4, planner, altri seed, sezione B all'ascolto.

**Ascolto:** in attesa.

## Ascolto cieco del titolare (2026-11-23)

Risposte trascritte prima di aprire la chiave:

> frase 1: ripetitività 6
> suona come la stessa melodia che si ripete, cambia solo alla fine negli ultimi secondi
>
> frase 2: ripetitività 4
> le parti ripetute sono più lunghe e variano nel corso del brano, anche se in alcuni punti suonano un po' forzati i cambiamenti: 0:15, 0:31, 0:47, 0:59
>
> frase 3: ripetitività 3
> qui ho notato come delle "pause" ad esempio a 0:15 0:27 e 0:31 che sono interessanti, potrebbero aiutare nella varietà ma rischiano di sembrare innaturali, ma forse danno invece carattere, non mi spingo oltre nel giudizio in merito.
>
> frase 4: ripetitività 4
> varietà simile alla frase 2 ma suona meno forzato
>
> Forse il preferito è proprio il 2, seguito dal 3, i difetti (pause e cambi) se cadono nel tempo giusto secondo me sono belli da sentire e interessanti. Ad esempio nella frase due in alcuni punti sono molto orecchiabili secondo me come ad esempio 0:47

**Chiave, aperta dopo:** 1 = p (oggi), 2 = m (melodia), 3 = b (entrambi), 4 = t (Pulso e Trama). Le risposte sopra non sono state modificate.

| Versione | Ripetitività |
|---|---:|
| p | 6 |
| m | 4 |
| t | 4 |
| b | 3 |

| Previsione | Esito |
|---|---|
| (a) m e b meno ripetitivi di p | Confermata (4 e 3 contro 6) |
| (b) m e b «frase con inizio e fine» più di p e t | **Non valutabile:** la domanda non ha ricevuto risposta esplicita per m, t e b; p «la stessa melodia che si ripete» |
| (c) t meno ripetitivo di p, meno ridotto di m e b | **Mancata in parte:** t < p, ma t = m |

- **La falsificazione non scatta:** L2 resta proponibile. Il 6 di p coincide con il giudizio dell'esperto (6/7), ma su materiale diverso: non è una conferma.
- **I punti citati, controllati sulle note:** 0:15, 0:31 e 0:47 in m cadono nelle battute 7, 15 e 23, le **risposte con respiro** (81 77 74 e pausa); 0:59 è la battuta 29 (coda invertita con ornamento). In b, 0:15 e 0:31 sono gli stessi respiri; 0:27 è la battuta 13, dove la coda della melodia è accorciata e Pulso perde l'ultimo attacco. Lo stesso respiro è «un po' forzato» in m e «interessante» in b, e a 0:47 «orecchiabile» in m.
- **t dà varietà simile a m senza toccare una nota di melodia, e suona «meno forzato».**
- **Preferito:** m, poi b («forse»). Il meno forzato è t, il meno ripetitivo è b: nessuna versione vince su tutto.
- **Ipotesi post hoc:** i cambi piacciono «se cadono nel tempo giusto». Da tradurre in una regola (respiro in quali frasi, con quale sostegno ritmico) e pre-registrare prima di provarla.
- **Limiti:** un solo ascoltatore, che conosce il brano e l'ipotesi; giudizi ravvicinati (scarti di un punto non significativi); solo A; il finale alto a 1:02 resta da correggere (salto A→B di 19).

**Esito:** b è il candidato di lavoro per I4+I6, con due correzioni da pre-registrare: il salto A→B e la collocazione dei respiri. Nulla adottato; conferma con l'esperto.

**Precisazione del titolare, non cieca (dopo la chiave, 2026-11-23):**

> soprattutto nella frase 3 noto una progressione che può avere un inizio e una fine, riascoltandola è molto orecchiabile, anche più della 2, diventa la mia preferita.

- **Preferito aggiornato:** b, poi m. Il giudizio cieco sopra (m, poi b) resta agli atti e non si sostituisce.
- **Previsione (b):** ora ha una risposta favorevole per b («può avere un inizio e una fine»), ma non cieca e dopo il mio commento che proponeva b come candidato: sostegno debole. Per m e t resta senza risposta esplicita. L'esito cieco rimane «non valutabile».
- **Riascolto:** può contare anche la familiarità (un secondo ascolto tende a piacere di più). Da chiedere all'esperto senza anticipazioni.
