# M2.5.3–4 — Contratto minimo, punto 2: Vettore legge l'accordo. Risultati

- **Pre-registrazione:** [M2_5_3_C2_Vettore_Reads_Chord_Preregistration.md](M2_5_3_C2_Vettore_Reads_Chord_Preregistration.md), SHA-256 (LF) `32dc6ca5de83f5780458b09d3859a39270e2dae39a0460d1c91fcaf0a4de0666`.
- **Esecuzione:** 2026-11-17.
- **Dati:** [esperimento](experiments/m25_3_c2_vettore_chord/README.md).
- **Decisione:** D-M2-42.

## 1. Cosa cambia

Nel solo vincitore, dopo la scelta e dopo la diade di Corpo, Vettore sposta alcune note sulla nota dell'accordo più vicina, a parità verso il basso. Le note interessate sono di due tipi:
- quelle fuori dall'accordo sul tempo 1 di un cambio d'accordo;
- quelle in tritono o seconda minore con una voce di Corpo.

Attacchi e durate non cambiano. Con `vettoreReadsChord = false` il piano è quello di C1c: è l'ablazione.

## 2. Esiti

| ID | Predizione | Risultato | Esito |
|---|---|---|---|
| V1 | Selezione identica con e senza | Vincitori, punteggi e riparazioni della diade identici | Confermata |
| V2 | Note dell'accordo sui tempi 1 dei cambi: da 17/25 a 25/25 per sezione | 25/25 in A, B e A′ | Confermata |
| V3 | 0 note di Vettore in classe 1 o 6 con Corpo; Corpo–Vettore ic1 in B da 6,2% a 0 | 0 in ogni sezione; ic1 = 0 e ic6 = 0 ovunque | Confermata |
| V4 | 40 note spostate: 8, 24, 8 | 8, 24, 8 | Confermata |
| V5 | Pulso, Corpo e Trama identici; in Vettore cambiano solo le altezze | Sì | Confermata |
| V6 | Ablazione: intent di C1c, 0 adattamenti; esito «attivo» | Sì: con l'ablazione le altezze differiscono esattamente nelle 40 note; Vettore cambia in tutte le 24 frasi | Confermata |
| V7 | Picchi di novità di Corpo alle frasi 9 e 17 | 9 e 17 con M4, M5 e M5r; M5 e M5r ancora ammesse | Confermata |
| V8 | Fixture PASS con golden aggiornati; `meter_change_v1` PASS in M1–M5 | Sì | Confermata |

**Conseguenza non prevista: il controllo di invarianza del motivo.** `planner_probe_v1` verifica che le note reali di Vettore coincidano con il motivo vincente del trace, così che il piano resti ricostruibile. La prima implementazione cambiava le altezze senza registrarlo, e il controllo è fallito. A catena sono falliti anche 11 test dei pacchetti di valutazione, che lo richiedono.

Il controllo non è stato allentato. Il trace registra ogni adattamento (battuta, indice della nota nel motivo, semitoni), nell'hash canonico del piano (formato 3) e nell'hash del trace. Il controllo ricostruisce le altezze dal motivo più gli adattamenti, e l'oracolo indipendente delle altezze verifica in più che ogni adattamento sia di 1 o 2 semitoni. La pre-registrazione non lo aveva previsto.

## 3. Misure descrittive

| Sezione | CTnCTR (C1c → C2) | Note dell'accordo | PCS | Riferimento umano (Yeh) |
|---|---|---|---|---|
| A | 0,63 → 0,70 | 0,56 → 0,63 | 1,06 → 1,31 | CTnCTR 0,74, PCS 1,42 |
| B | 0,81 → 0,875 | 0,50 → 0,69 | 1,19 → 1,50 | |
| A′ | 0,63 → 0,69 | 0,56 → 0,63 | 1,00 → 1,31 | |

- **A e A′:** l'armonicità sale verso il riferimento umano senza superarlo.
- **B:** supera il riferimento su CTnCTR e PCS. Il dossier avverte che valori troppo alti suonano conservativi: serve una banda, non un massimo (R-HAR-3, non implementata). È un rischio da sentire.
- **Varietà:** l'ostinato di Vettore ha ora varianti per battuta guidate dall'accordo. In A compaiono `G F D E` sull'accordo di Sol (4 battute) e `C F A G` sul La minore (4); in A′ `C F A G` (8). È una variazione motivata dall'armonia, non dal caso. Se suoni come sviluppo o come correzione lo dirà l'ascolto.

## 4. Cosa resta

- R-HAR-2 (etichetta e risoluzione delle note estranee) e R-HAR-3 (banda di armonicità).
- La prova d'ascolto dell'ablazione (B6), che richiede prima la rilevazione (B1). Due versioni sono già disponibili con l'interruttore.
- La fragilità dei margini di selezione (punto 6) e l'ipotesi «Vettore sopra Corpo in A′» (punto 3).

## 5. Golden e build

| Golden | Prima | Dopo |
|---|---|---|
| Trace della fixture | 7595762444317258244 | 7802016689135067568 |
| Piano della fixture (formato 3) | 0xc5fc6a007f0a0a34 | 0x18cc6d32dc2fa60c |
| PCM della fixture | 795499972965883485 | 9396686444379387342 |
| Piano della forma di prova A→B | 0xa3ca0dbd6a6f9d0e | 0x078e7ac77d936659 |
| PCM di `meter_change_v1` | 9026598872691749635 | 16485786595596680340 |

Release e Debug `/WX` 300/300, con un test nuovo, `[vettore_chord]`; `full_m2_suite` verde salvo `acao02_v1`, il limite noto.
