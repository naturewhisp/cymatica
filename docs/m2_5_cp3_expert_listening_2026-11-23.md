# M2.5 CP-3 — Ascolto di conferma dell'esperto (2026-11-23)

- **Stato:** evidenza qualitativa di **un ascoltatore**. Non è un gate. Nessuna adozione.
- **Disegno e ipotesi:** [documento](research/M2_5_CP3_Expert_Listening_Hypotheses.md), SHA-256 (LF) `394b66a1…95d7b0bf`, registrato prima del materiale. Pacchetto `cymatica_ascolto_cp3_2026-11-23`, zip SHA-256 `da5c11a4…5ba2417`, codice `fec1a70`.
- **Risposte grezze:** [trascrizione](research/experiments/m25_cp3_listening/risposte_2026-11-23_esperto.txt), salvata prima di aprire la chiave. Oscurati il nome di una persona e il titolo della produzione dell'esperto; il resto è testuale.
- **Condizioni:** ATH-M50x e UMC204HD, volume come il 2026-11-19; circa 30 minuti (previsti 20).
- **Chiave (seme 3650933458):** 1 K (candidato, oggi), 2 M (c, oggi), 3 V (V2m, V1), 4 T (senza, r8), 5 I (identica), 6 R (x, oggi), 7 D (V1, V2m).

## 1. Controlli B0

| Controllo | Risposta | Esito |
|---|---|---|
| Coppia identica (5) | differenza «sì», preferita la seconda («coda più lunga» dell'abbellimento FM) | **Fallito** (i due file sono uguali campione per campione, verificato) |
| Duplicato di V (3 e 7) | V2m preferita entrambe le volte, in ordini opposti, con lo stesso motivo | Coerente |

**Regola registrata:** con la coppia identica giudicata diversa, **tutti i confronti a coppie valgono come indicazione debole**. Il 2026-11-19 i controlli dello stesso esperto erano superati.

## 2. Ipotesi

| ID | Esito |
|---|---|
| E1, ripetitività ≤ 4 | 4 (era 6): **confermata**. Ma **coerenza 3** (era 5), interesse 4 (invariato): «molto più frenetico e a tratti un po' randomico» |
| E2, frase con inizio e fine | «una figura che si ripete»: **mancata** |
| E3, kick | «è giusto»; in K preferito il candidato («kick più presente»): **confermata**, K debole |
| E4, basso distinto | «lo sento ed è distinto dalla cassa»: **confermata**; il livello non ha risposta, ma nelle osservazioni «un po' più presente sulle basse» |
| E5, in M si preferisce c | differenza «non so», preferita **oggi**: **mancata**; senza rilevazione la preferenza non conta |
| E6, in R si preferisce x | **senza risposta**: non valutabile |
| E7, A→B non brusco | «brusco», e «brusco» anche B→A′: **mancata** |

**V:** V2m preferita con motivo (due volte): per la regola **V2m**, con sostegno debole. **T:** «non so», nessuna preferenza: **nessun ADR per la rampa**.

**Materiale di M verificato** perché la risposta era inattesa: la prima è c (correlazione 0,9996), le due differiscono di −6,4 dB. Non è un errore del pacchetto.

## 3. Lettura

- **Il basso è il risultato più chiaro** («UN BASSO!!!!!»), ma con una critica di sostanza: basso e batteria devono parlarsi e ora «c'è troppa sincope».
- **Le misure sono compatibili con quella critica** (post hoc, `verify_answers.json`): in Z solo il **22–25 %** dei kick ha un attacco di basso insieme, il 29–37 % degli attacchi di basso cade su un kick, e solo il 40–60 % dei kick cade sui quarti.
- **Contraddizione da dichiarare:** avevo scelto il 36 % di coincidenza copiandolo dalla sua produzione (pre-registrazione di I3). Nella sua linea il dato era vero, ma **non bastava come criterio**: lì la percussione è più ricca e gli accenti sono condivisi. Il criterio P7 era sbagliato come obiettivo.
- **c non passa dall'esperto:** la ripetitività scende in Z, ma coerenza e forma della melodia no, e in coppia c non si distingue. **E1 ed E5 danno indicazioni opposte**: I4+I6 **non** si propone per l'adozione, la leva si rivede. E1 era su Z, che unisce quattro candidati.
- **Disaccordo con il titolare,** che sentiva c con un inizio e una fine: vale l'esperto (D-M2-51), con il limite dei controlli. I giudizi del titolare erano su estratti e con l'ipotesi nota.
- **B→A′ resta brusco in Z, che contiene x;** R è senza risposta. I5d non è confermata.
- **Istanti di calo, post hoc:** 0:47 battuta 23 (un respiro), 1:24 battuta 43, 2:03 **battuta 66, dove x torna al registro di oggi**, 2:09 battuta 69. Quattro istanti di un ascoltatore, con ritardo di reazione ignoto: indizi.
- **Stanchezza:** «sentire sempre più o meno le stesse melodie diventa molto stancante tra una sessione e l'altra». Può spiegare parte dei giudizi (familiarità). Per la prossima sessione serve **materiale melodico nuovo**.

## 4. Osservazioni e richieste dell'esperto

Nel [registro](adr/0007-m2-5-entry-decisions.md) §8.1. In breve: più suoni percussivi (batteria e percussioni); basso più presente nelle basse e legato alla batteria; **un sistema di accenti** scelti per misura e seguiti almeno da batteria e basso; se uno è complesso l'altro è semplice; ritmi più «squadrati»; un motivo del tutto diverso. Due risorse in rete sul rapporto basso-batteria, **non lette** in questa analisi (i link sono nella trascrizione).

## 5. Limiti

- Un ascoltatore, controllo identico fallito, due risposte incomplete (R, livello del basso), ascolto della sesta coppia forse saltato.
- Z unisce c, x, kick e basso: il calo di coerenza non si attribuisce.
