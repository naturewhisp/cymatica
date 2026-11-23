# M2.5 CP-3 — I4+I6, prototipo dei motivi di due battute. Pre-registrazione

- **Stato:** scritto il 2026-11-23, **prima** di codice, render e misure. L'hash SHA-256 (LF) è registrato in `progress.md`. Dopo l'hash non si modifica. Il piano di riferimento (seed 1337) era già stato letto.
- **Origine:** leve L1 e L2 di [I4](M2_5_CP3_I4_Form_Investigation_Results.md) e [I6](M2_5_CP3_I6_Melody_Investigation_Results.md), soglie già dichiarate in I6 §3.
- **Solo offline** sul seed 1337; golden NOT_APPLICABLE, nulla adottato. L'ordine domanda/risposta (pari/dispari) è un'assunzione sperimentale di questo prototipo.

## Versioni

| Versione | Vettore (L2) | Pulso e Trama (L1) |
|---|---|---|
| **p** | oggi | oggi |
| **m** | due battute | oggi |
| **t** | oggi | due battute |
| **b** | due battute | due battute |

**Vettore**, per ogni frase di 4 battute. La testa (attacchi nei primi due quarti) non cambia mai; la coda è il resto.
- Battute 1 e 3: come oggi.
- Battuta 2 (*a′*): coda invertita sui gradi della scala attorno all'ultima nota della testa; poi **liquidazione** (via l'ultima nota, la precedente ne copre la durata) se la battuta ha almeno 5 note, altrimenti **ornamento** (l'ultima nota si divide a metà, la seconda parte è il grado di scala verso la nota seguente, o quello superiore se sono uguali).
- Battuta 4, **domanda** (frasi pari): come la battuta 2, ma l'ultima nota è la terza o la quinta dell'accordo di tonica più vicina.
- Battuta 4, **risposta** (frasi dispari): testa più una sola nota lunga un quarto sull'ottavo sedicesimo: la tonica più vicina a quella che sostituisce, preferendo quelle sopra Corpo. Poi pausa.
- Una nota nuova che dista di classe 1, 6 o 11 da una nota di Corpo che suona non si cambia: resta l'altezza di oggi (contata). Per la tonica e per la domanda si passa al candidato successivo.

**Pulso e Trama**, forma a b a c: battuta 2 senza l'ultimo attacco; battuta 4 con, per Pulso, un attacco in più sul sedicesimo 14 (o 15 se occupato) a velocity 0,7; per Trama, l'ultimo attacco anticipato di 2 sedicesimi (di 1 se occupato, altrimenti invariato). Le battute con meno di 2 attacchi non cambiano.

## Criteri

| ID | Criterio |
|---|---|
| Z1 | p uguale al riferimento entro 0,001 |
| Z2 | m e b cambiano solo il ruolo 3, t e b solo 0 e 2 oltre a questo; teste identiche; altezze nuove nella scala della sezione |
| Z3 | Stesso contorno della battuta 1 (M1) < 60 % in m e b (oggi 89 %) |
| Z4 | Stesso ritmo della battuta precedente (G1): Vettore < 60 % in m e b; Pulso e Trama < 50 % in t e b (oggi 100 %) |
| Z5 | Finali sulla tonica: dispari meno pari ≥ 50 punti in m e b |
| Z6 | Almeno 8 respiri in m e b |
| Z7 | Conflitti di classe 1/6/11 non più di oggi; D-M2-43: nessuna frase che oggi ha almeno metà delle note sopra Corpo scende sotto metà |
| Z8 | Render MEASURED, limiter 0 dB, nessun evento rimasto né scarti, loudness entro 0,5 LU da p |

Non verificati: lignaggio K4, contratti del planner e altri seed, perché il prototipo non passa dal planner. Contenuto informativo: descrittivo, senza soglia.
Se Z1, Z2 o Z8 falliscono non si generano estratti. Gli altri esiti si riportano senza ritocchi; una variante corretta richiede una nuova pre-registrazione.

## Ascolto

Quattro estratti ciechi: tutta la sezione A (64 s), loudness pareggiata, ordine casuale. Domande: ripetitività 1–7; «frase con inizio e fine» o «figura che si ripete»; che cosa disturba e a che secondo; preferito.

**Previsioni:** (a) m e b meno ripetitivi di p; (b) m e b «frase con inizio e fine» più di p e t; (c) t meno ripetitivo di p, meno di quanto lo siano m e b. Nessuna previsione sul preferito: il seed 34, preferito da due ascoltatori, è il più ripetitivo sulle misure.

**Falsificazione:** se m e b non risultano meno ripetitivi di p, la diagnosi simbolica non basta a spiegare il giudizio e L2 non si propone. **Limiti:** un ascoltatore che conosce il brano; una sezione; la ripetitività dell'esperto riguardava il brano intero.
