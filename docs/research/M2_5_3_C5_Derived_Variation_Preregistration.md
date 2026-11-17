# M2.5.3–4 — Contratto minimo, punto 5: variazione derivata e ripresa, «catena con limiti». Pre-registrazione

- **Stato:** scritto il 2026-11-17, prima del codice. L'hash SHA-256 (LF) è registrato in `progress.md`. Dopo l'hash il file non si modifica.
- **Decisione del titolare:** «Catena con limiti» (2026-11-17), dopo la diagnosi in `progress.md`. L'alternativa era «Solo ripresa».
- **Definizioni operative** dal [dossier R-M25-7](M2_5_F5_Music_Director_Research.md): una variazione è **sviluppo** se è derivata, cumulativa, dipendente dall'ordine e ripresa in seguito; è **fittizia** se non lascia conseguenze. Criterio del titolare: «ricordi, sviluppi, sorprenda e ritorni».
- **Misure:** M5r ammessa dal pacchetto 6 (D-M2-38), usata con la sua implementazione C++ (`developmentDistanceMatrix`) sul ruolo Vettore. Nessuna misura entra nel punteggio del planner.

## 1. Diagnosi (piano `656cf23`)

- In tutte le 24 frasi i candidati trasformano il motivo base della sezione: il lignaggio ha profondità 1, l'ordine non conta.
- La ripresa A′ riparte dal motivo base, non da ciò che si è sentito in A.
- Le trasformazioni non sono pensate per la catena: l'inversione è quasi un'involuzione; trasposizioni, ottave e alterazioni si sommano senza limite.

## 2. Variabile

1. **Origine del motivo, dichiarata per frase** dall'espansore: `PhrasePlan::motifSource` ∈ {`SectionBase` (default, come oggi), `PreviousWinner`, `RecallSectionOpening`}, con `recallSection` per il richiamo.
2. **Memoria a capacità fissa:**
   - il motivo vincente della frase precedente;
   - il motivo vincente della prima frase di ogni sezione, al massimo `kMaxSections` = 8.

   Entrambi stanno nel checkpoint del planner. Il motivo vincente è quello del trace, prima degli adattamenti per battuta dei punti 2 e 3.
3. **Limite:** se il motivo richiesto si allontana dal motivo della sezione di oltre **3 gradi** nel baricentro, la frase riparte dal motivo della sezione e il trace lo registra come `lineageReset`. Il baricentro è la media dei gradi, con 7 gradi per ottava. Il limite impedisce la deriva di registro e di trasposizione.
4. **Trace:**
   - `baseSourcePhrase` è la frase da cui viene il motivo base, −1 per il motivo della sezione;
   - `lineageReset` segnala la ripartenza.

   Entrambi entrano nell'hash canonico del piano (formato 4 → **5**) e nell'hash del trace. Il controllo d'invarianza del motivo accetta un motivo base uguale a quello della sezione, oppure al motivo vincente della frase indicata.
5. **Profilo iniziale `fixture-lineage-v1`** (valore d'autore, non validato):

   | Frasi | Origine |
   |---|---|
   | 0, 8 | motivo della sezione (apertura) |
   | 1–6, 9–14, 17–22 | vincitore della frase precedente (catena) |
   | 7, 15, 23 | motivo della sezione (ritorno alla cadenza) |
   | 16 | richiamo dell'apertura di A (frase 0) |

## 3. Criteri e regola d'adozione

| ID | Criterio | Superato se |
|---|---|---|
| **K1, struttura** | Le origini del trace seguono il profilo, salvo le ripartenze registrate; nessun motivo base oltre il limite; controllo d'invarianza superato | Tutto vero |
| **K2, contratto precedente** | Dopo il pacchetto devono reggere i punti 1–4 | Tutte vere: 0 battute di Corpo contraddette; note dell'accordo sui tempi 1 dei cambi 25/25 per sezione; 0 note di Vettore in classe 1 o 6 con Corpo; nessun `vettoreRegisterUnmet`; R-MET-2 |
| **K3, sviluppa** | Contenuti distinti di Vettore (gruppi di frasi a distanza M5r 0) e materiale nuovo riusato | (a) i gruppi sono più che in C4; (b) in almeno 2 sezioni c'è un contenuto di Vettore che compare per la prima volta in una frase derivata e ricompare in una frase successiva della stessa sezione |
| **K4, ricorda** | Frasi derivate dalla precedente, senza ripartenza | Distanza M5r dal genitore dichiarato ≤ mediana delle distanze dalle altre frasi precedenti della sezione, in almeno il 75% |
| **K5, ritorna** | La frase 16 richiama l'apertura di A | Il motivo base della 16 è il vincitore della 0 (trace), e fra le frasi 1–15 la 0 è la più vicina alla 16 per M5r (a pari merito ammessa) |
| **K6, sezioni** | Novità di Foote su Corpo | I due picchi più alti alle frasi 9 e 17 |
| **K7, scenari** | Fixture e `meter_change_v1` | PASS, con i golden aggiornati col motivo |

**Adozione:** il pacchetto si adotta se superano K1, K2, K6, K7 e almeno K3 e K5. K4 si riporta in ogni caso. Altrimenti l'esito è negativo, e il piano resta quello di C4.

**Predizioni qualitative:**
- cambia la melodia in quasi tutte le frasi in catena;
- con l'inversione nella tabella, le catene tendono ad alternare due stati;
- le ripartenze per il limite sono poche.

## 4. Limiti

- Le trasformazioni restano quelle delle tabelle attuali. Una catena davvero progressiva vorrebbe operazioni pensate per la catena (ornamento, frammento, estensione), che restano fuori dal pacchetto.
- K3 e K4 sono misure simboliche: lo sviluppo percepito si giudica all'ascolto, prove B2–B5 del dossier.
- Il limite di 3 gradi è un valore iniziale d'autore.
