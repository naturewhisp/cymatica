# M2.5.3–4 — Contratto minimo, punto 2: Vettore legge l'accordo, con ablazione. Pre-registrazione

- **Stato:** scritto il 2026-11-17, prima del codice e di ogni misura del cambiamento. L'hash SHA-256 (LF) è registrato in `progress.md`. Dopo l'hash il file non si modifica.
- **Decisioni di riferimento:**
  - ADR-0007 D-M2-32: contratto minimo, punto 2, «Vettore che legge l'accordo, con ablazione»;
  - D-M2-41: diade riparata dopo la scelta;
  - D-M2-39: policy 1.0.2.0;
  - D-M2-19, metodo.
- **Regole:**
  - R-HAR-1: sul tempo 1 di un cambio d'accordo il ruolo dominante suona una nota dell'accordo;
  - R-HAR-5, secondo passo: dopo la diade, il grado di Vettore.

  Ordine del [dossier](M2_5_F5_Music_Director_Research.md): Vettore è un *cantus firmus*, «si stabilisce prima e si adatta per ultimo».

## 1. Diagnosi sul piano attuale

Piano di riferimento: commit `b11b9ff`, [intent di C1c](experiments/m25_3_c1_corpo_dyad/fixture_intents_c1c.csv). Accordi ricavati dal basso di Corpo, triadi diatoniche del modo della sezione.

| Sezione | Tempi 1 con cambio d'accordo e nota di Vettore | Note fuori dall'accordo su quei tempi | Note di Vettore in classe 1 o 6 con una voce di Corpo |
|---|---|---|---|
| A | 25 | 8: Re su La minore (4), La su Sol (4) | 0 |
| B | 25 | 8: Sol su Fa | 16: Re contro il basso Mib |
| A′ | 25 | 8: Re su La minore | 0 |

## 2. Variabile

Si aggiunge a C1c, nella stessa rigenerazione del solo vincitore dopo la scelta, e **dopo** la riparazione della diade. Per ogni nota del motivo di Vettore, in ogni battuta:

1. **R-HAR-1:** se la nota attacca sul passo 0 di una battuta con cambio d'accordo e la sua classe di altezza non è nell'accordo, la si sposta sulla nota dell'accordo più vicina in semitoni. A parità si va verso il basso, come nella risoluzione di un'appoggiatura.
2. **Conflitto con Corpo:** se la nota forma classe d'intervallo 1 o 6 con una delle due voci di Corpo della battuta, la si sposta allo stesso modo sulla nota dell'accordo più vicina.
3. Tutte le altre note restano come sono: le note di passaggio senza conflitto non si toccano.

**Definizioni:**
- C'è **cambio d'accordo** quando l'accordo della battuta differisce da quello della battuta precedente della frase. Per la battuta 0 si confronta con l'ultima battuta della stessa frase, perché la forma ripete il giro; la prima frase di ogni sezione conta sempre come cambio.
- La durata e l'attacco delle note non cambiano, l'ottava resta la più vicina.
- Il trace conta le note spostate in `vettoreAdaptedNotes`, che entra nell'hash canonico del piano (formato 2 → **3**) e nell'hash del trace della fixture.

**Ablazione:** `MusicPlannerConfig::vettoreReadsChord` vale `true` per default. Con `false` il planner si comporta esattamente come C1c.

## 3. Predizioni

| ID | Predizione |
|---|---|
| **V1, selezione** | Vincitori, punteggi e pareggi identici a C1c in tutte le 24 frasi |
| **V2, R-HAR-1** | Sui tempi 1 con cambio d'accordo, la quota di note dell'accordo passa da 17/25 a **25/25** in ogni sezione |
| **V3, conflitti** | Note di Vettore in classe 1 o 6 con una voce di Corpo: 0 in ogni sezione. In `role_clashes`, Corpo–Vettore ic1 in B va da 6,2% a 0 |
| **V4, quante e quali** | Note spostate: 8 in A (Re → Do su La minore, La → Sol su Sol), 24 in B (Sol → Fa su Fa; Re → Mib su Mib), 8 in A′ (Re → Do): 40 in tutto, come somma di `vettoreAdaptedNotes` |
| **V5, cosa non cambia** | Pulso, Corpo e Trama identici a C1c. In Vettore cambiano solo altezze, mai attacchi né durate |
| **V6, ablazione** | Con `vettoreReadsChord = false` gli intent sono identici a quelli di C1c, e `vettoreAdaptedNotes` = 0 ovunque. L'esito simbolico dell'ablazione è «attivo»: il fingerprint di Vettore cambia in tutte le frasi con note spostate |
| **V7, sezioni** | Novità di Foote su Corpo: i due picchi più alti alle frasi 9 e 17, con M4, M5 e M5r |
| **V8, scenari** | Fixture PASS con i golden aggiornati col motivo; `meter_change_v1` PASS in M1–M5 |

**Descrittivi, senza predizione:**
- CTnCTR e PCS di Vettore per sezione;
- distanze M5 e M5r di Vettore fra le frasi;
- Trama–Vettore per classe d'intervallo.

## 4. Limiti

- Lo spostamento cambia la melodia, ed è voluto: è la prima variazione di Vettore guidata dall'armonia. Se suoni come sviluppo o come correzione lo decide l'ascolto cumulativo (prova B6 di ablazione, che richiede prima B1).
- La regola non etichetta le note estranee (R-HAR-2) e non tratta la durata: un'appoggiatura voluta sul tempo 1 viene risolta d'ufficio.
- Il punteggio non vede l'adattamento: la scelta del candidato non cambia, coerente con C1c.
