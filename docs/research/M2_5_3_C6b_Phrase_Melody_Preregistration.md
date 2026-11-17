# M2.5.3–4 — Pacchetto 6b: melodia lunga una frase, per progressione sugli accordi. Pre-registrazione

- **Stato:** scritto il 2026-11-17, prima del codice. L'hash SHA-256 (LF) è registrato in `progress.md`. Dopo l'hash il file non si modifica.
- **Decisione del titolare:** «6b, melodia lunga» (2026-11-17).
- **Fonte:** [note su AURA](AURA_Music_Source_Notes.md), voci A2 e A3. Il tema di AURA è una cella di una battuta che si ripete trasposta sulla fondamentale di ogni accordo (progressione melodica); su più battute ne viene una melodia che segue l'armonia.
- **Diagnosi:** il motivo di Vettore dura una battuta e si ripete identico in tutte e quattro le battute della frase, come un ostinato: 96 volte nel brano. È la ragione principale per cui la melodia suona «sempre la stessa».

## 1. Variabile

1. **Campo di frase** `PhrasePlan::melodyFollowsChords`, dichiarato dall'espansore. Quando è vero, nella battuta b tutte le note del motivo di Vettore si spostano di **o(b) gradi della scala**. o(b) è la fondamentale dell'accordo della battuta meno quella della battuta 0, ridotta all'intervallo più vicino in [−3, +3] gradi, così la melodia si muove per gradi vicini invece di saltare.
   - In A e A′ (accordi Re m, Sol, La m, Re m): o = 0, +3, −3, 0.
   - In B (Sol m, Mi♭, Fa, Sol m): o = 0, −2, −1, 0.
2. **Il motivo resta di una battuta.** La progressione si applica alla realizzazione, come lo spostamento di registro del punto 3. La catena, le trasformazioni e la memoria dei motivi non cambiano.
3. **Coerenza con i punti precedenti:**
   - la riparazione della diade di Corpo e la lettura dell'accordo leggono le altezze già spostate;
   - il trace registra `melodyFollowsChords`, che entra nell'hash canonico del piano (formato 5 → **6**) e nell'hash del trace;
   - il controllo d'invarianza del motivo ricostruisce le altezze con lo spostamento di ogni battuta.
4. **Profilo:** `melodyFollowsChords` è vero in tutte le frasi della fixture e delle sue varianti. Le altre forme (prova, `meter_change_v1`) non lo dichiarano, e il default è falso.

Melodie che ne risultano nella prima frase, prima degli adattamenti dei punti 2–5:

| Motivo | A | B |
|---|---|---|
| Attuale | `D F A G ∣ G B D C ∣ A C E D ∣ D F A G` | `G D E♭ D ∣ E♭ B♭ C B♭ ∣ F C D C ∣ G D E♭ D` |
| Cella di AURA | `D D F A G F ∣ G G B D C B ∣ A A C E D C ∣ D D F A G F` | `G G B♭ D C B♭ ∣ E♭ E♭ G B♭ A G ∣ F F A C B♭ A ∣ G G B♭ D C B♭` |

## 2. Criteri e regola d'adozione

| ID | Criterio | Superato se |
|---|---|---|
| **T1, struttura** | Invarianza del motivo e trace | Il controllo d'invarianza di `planner_probe_v1` e `fixture_probe_v1` passa; il campo è nel trace |
| **T2, melodia di frase** | Battute di Vettore dentro la frase | In almeno 22 frasi su 24 della fixture, almeno 3 delle 4 battute hanno successioni di altezze diverse. Con C6 erano 0: la battuta si ripeteva |
| **T3, contratto precedente** | Punti 1–4 | Note dell'accordo sui tempi 1 dei cambi 25/25 per sezione; 0 note di Vettore in classe 1 o 6 con Corpo; nessun `vettoreRegisterUnmet`; R-MET-2. Battute di Corpo contraddette: descrittivo |
| **T4, sviluppo** | Misure ammesse | Picchi di novità di Corpo alle frasi 9 e 17; lignaggio K4 ≥ 75% e nessuna alternanza A-B-A |
| **T5, adattamenti** | Lettura dell'accordo | Le note spostate dal punto 2 sono meno delle 47 attuali: la cella e il motivo partono dalla fondamentale di ogni accordo |
| **T6, diversità** | Seed da 1 a 64 | Resta `DISTRIBUTIONS` |
| **T7, scenari** | Fixture, forma di prova, `meter_change_v1` | PASS, con i golden della fixture aggiornati col motivo; forma di prova e `meter_change_v1` invariati negli eventi |

**Adozione:** superano T1, T2, T3, T4 e T7. T5 e T6 si riportano. Altrimenti l'esito è negativo, e il piano resta quello di C6.

## 3. Limiti

- È una melodia di frase per progressione: la stessa figura su accordi diversi. Non è ancora una melodia libera con domanda e risposta, che richiederebbe motivi più lunghi di una battuta.
- Il tema di AURA non si importa alla lettera: il suo giro (Do m – La♭ – Fa m – Sol) e la sua cadenza sono diversi da quelli di CYMATICA. Se ne prende il principio.
- Le melodie possono salire o scendere molto. Il registro lo regola il punto 3, ma la gamma della frase si allarga: lo si misura come descrittivo.
