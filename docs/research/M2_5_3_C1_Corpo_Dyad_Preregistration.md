# M2.5.3–4 — Contratto minimo, punto 1: diade di Corpo sui punti contraddetti. Pre-registrazione

- **Stato:** scritto il 2026-11-17, prima del codice e di ogni misura. L'hash SHA-256 (LF) è registrato in `progress.md`. Dopo l'hash il file non si modifica.
- **Decisioni di riferimento:** ADR-0007 D-M2-32 (perimetro; contratto minimo di R-M25-7, opzione B, punto 1), D-M2-38 (ordine), D-M2-39 (policy del planner 1.0.2.0 per la fase), D-M2-19 (metodo), D-M2-30.
- **Regola:** R-HAR-5 delle [note sulle regole d'insieme](experiments/m25_f5_research_notes/regole_insieme.md), riparazione a costo minimo: prima la diade di Corpo, poi il grado di Vettore, infine lo scarto del candidato. Questo pacchetto fa solo il primo passo.

## 1. Diagnosi

Il [dossier R-M25-7](M2_5_F5_Music_Director_Research.md) misura un tritono Fa–Si di 8,0 s fra Corpo e Vettore in A e A′: sull'accordo di Sol Corpo suona Sol + Si, e Vettore suona Fa. Con fondamentale e quinta (Sol + Re) il tritono sparirebbe senza toccare il motivo.

Misura mia sul piano attuale (commit `97d07e7`, [intent di F1](experiments/m25_3_f1_metric_anchor/fixture_intents_f1.csv)): delle 96 battute di Corpo, **16** hanno la voce alta in classe d'intervallo 1 o 6 con almeno una nota di Vettore della stessa battuta. Sono tutte in A e A′, tutte sull'accordo di Sol, tutte con la terza (Si) in Corpo. In B nessuna. La settima maggiore Mib–Re di B (6,9 s) è fra **il basso** di Corpo e Vettore, e la diade non la può riparare.

## 2. Variabile

In `generateCandidates`, per ogni battuta di ogni candidato:
- si prende il voicing di Corpo come oggi, cioè per condotta delle parti o fondamentale e quinta, secondo la politica del candidato;
- **se la voce alta di Corpo è la terza dell'accordo** e forma classe d'intervallo 1 o 6 con l'altezza di almeno una nota di Vettore che attacca nella battuta, **e se la quinta non la forma con nessuna**, Corpo passa a fondamentale e quinta e la terza va al ruolo superiore, come nel voicing `RootFifth`.

Il controllo usa le note del motivo trasformato del candidato. È deterministico, non dipende da k e non entra nel punteggio come caratteristica nuova. La catena della condotta delle parti prosegue dal voicing riparato. La policy resta 1.0.2.0 (D-M2-39).

## 3. Predizioni

| ID | Predizione |
|---|---|
| **P1, riparazione** | Nessuna battuta della fixture ha più la voce alta di Corpo in classe 1 o 6 con Vettore. Le 16 battute di Sol in A e A′ hanno Corpo Sol + Re. Nel calcolo di `role_clashes` la quota di tritono Corpo–Vettore va da 3,1% a 0 in A e in A′ |
| **P2, cosa non cambia** | Il basso di Corpo, Pulso e Vettore restano identici in tutte le 24 frasi. Corpo di B resta identico |
| **P3, il limite** | La seconda minore / settima maggiore Corpo–Vettore di B resta (6,2% nel calcolo di `role_clashes`, ± 0,5 punti): è col basso e spetta al punto successivo |
| **P4, catena** | Fuori dalle 16 battute la voce alta di Corpo può cambiare per la condotta delle parti (dopo Sol + Re, la battuta seguente parte da Re). Nessuna battuta cambiata introduce una classe 1 o 6 con Vettore (segue da P1) |
| **P5, Trama** | Trama cambia solo nelle battute in cui cambia la voce alta di Corpo, perché ne legge i gradi. Il tritono Trama–Vettore in A e A′ non scompare: Trama suona comunque terza e quinta |
| **P6, vincitori** | Il candidato vincente di ogni frase non cambia, e restano 0 i pareggi in testa |
| **P7, sezioni** | La novità di Foote su Corpo ha ancora i due picchi più alti alle frasi 9 e 17, con M4, M5 e M5r |
| **P8, scenari** | Fixture PASS con i golden di piano, trace e PCM aggiornati col motivo; `meter_change_v1` PASS in M1–M6 |

**Base di P6:** la riparazione cambia la distanza di condotta delle parti dei candidati con la terza, quindi il punteggio. Il vincitore resta lo stesso se il suo vantaggio supera questo scarto: non l'ho calcolato, ed è la predizione più a rischio.

Il valore musicale (Corpo suona più «vuoto» sull'accordo di Sol? si perde colore?) va all'ascolto cumulativo, con una voce nuova del registro.

## 4. Limiti

- La regola guarda le altezze, non le durate né le posizioni metriche: una nota breve di passaggio basta a riparare. È voluto: la diade costa poco e non toglie nulla al motivo.
- Le note estranee di Vettore sui tempi 2 e 4 (CTnCTR) e la seconda minore di B restano: sono il punto 2 del contratto (Vettore che legge l'accordo).
- La regola non tocca il voicing `RootFifth` dei candidati che lo usano già.
