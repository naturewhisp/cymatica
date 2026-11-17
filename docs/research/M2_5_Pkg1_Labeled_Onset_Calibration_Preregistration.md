# Pacchetto di misura 1 — Calibrazione degli onset con eventi etichettati e offset vincolato: pre-registrazione

- **Stato:** scritto il 2026-11-17, prima del codice e di ogni misura. L'hash SHA-256 (LF) è registrato in `progress.md`. Dopo l'hash il file non si modifica.
- **Autorizzazione:** pacchetti 1–5 del [dossier delle lacune di misura](M2_5_Measurement_Gaps_Research.md), approvati dal titolare il 2026-11-17. Questo è il pacchetto 1 (O1 + O2, §5 del dossier), con la regola di validazione comune del dossier.
- **Difetto da correggere:** con la Trama a rapporto FM 1:1, a δ 0,3 la calibrazione di `acao02_v1` sceglie un offset di 95 ms con precisione e richiamo pari a 1: si allinea ai rilasci invece che agli attacchi ([risultati FM](M2_5_2_FM_Ratio_Results.md) §3). Il motivo è nel codice: l'offset è la mediana degli scarti di *ogni* riferimento dalla rilevazione più vicina, che a δ 0,3 è quella del rilascio.

## 1. Procedura nuova (offline, `engine/cacp/`, nessuna dipendenza nel codice C++)

Per ogni ruolo e ogni δ della griglia esistente:

1. **O1, etichette.** La verità viene dall'event log in due classi: **attacchi** (NoteOn applicati, gli stessi riferimenti di oggi) e **rilasci** (NoteOff applicati). Ogni rilevazione si confronta con gli eventi dopo una correzione provvisoria pari al centro della finestra di O2. È etichettata «attacco» o «rilascio» secondo l'evento più vicino entro 50 ms, altrimenti «falso positivo».
2. **Offset:** mediana degli scarti firmati delle sole rilevazioni etichettate «attacco» rispetto all'attacco più vicino.
3. **O2, finestra.** L'offset è ammesso solo dentro **[latenza intrinseca − 15 ms, latenza intrinseca + 15 ms]**. La latenza intrinseca è la mediana degli scarti del passo (a) di `acao02_v1`, sull'insieme sintetico A. 15 ms valgono circa tre hop dell'STFT (256 campioni, 5,33 ms) più la dispersione p05–p95 del passo (a), che è al più 4 ms. Un punto fuori finestra **non è ammissibile**. Se nessun punto è ammissibile, il ruolo è `INCONCLUSIVE` e non riceve nessun offset.
4. **Punteggio:** abbinamento uno a uno a 50 ms (`matchOnsetsMaxCardinality`) fra attacchi e rilevazioni corrette. Le rilevazioni «rilascio» sono escluse dal denominatore della precisione; i falsi positivi restano. Si riportano richiamo degli attacchi, rilevazioni sui rilasci, falsi positivi e F.
5. **Scelta:** F massima fra i punti ammissibili; a parità, il δ maggiore (la regola attuale).

La procedura attuale resta riportata nel manifest come diagnostica. Il profilo congelato e la sua regola non cambiano: il ricongelamento resta compito di M2.5.5.

## 2. Validazione dello strumento (regola comune del dossier)

| ID | Prova | Esito atteso |
|---|---|---|
| **V1, positivo** | Liste sintetiche nei test: attacchi, rilasci a +100 ms, rilevazioni su entrambi con latenza nota di 10 ms | Offset 10 ms ± 0,1; rilevazioni sui rilasci contate come tali; precisione e richiamo 1 |
| **V2, negativo** | Stesse liste con le rilevazioni solo sui rilasci (attacchi mancati), cioè il caso dei 95 ms | Nessun punto ammissibile, oppure richiamo degli attacchi 0: mai precisione 1 con offset fuori finestra |
| **V3, negativo** | Riferimenti spostati di 95 ms | `INCONCLUSIVE` per finestra |
| **V4, invarianza** | Due esecuzioni di `acao02_v1` | Misure identiche |
| **V5, oracolo** | mir_eval (`mir_eval.onset.f_measure`, finestra 0,05 s) sulle stesse liste di attacchi e rilevazioni corrette, esportate dallo scenario per ogni ruolo al δ scelto. Ambiente Python locale con versioni fissate, approvato il 2026-11-17 | Stesso numero di abbinamenti, quindi stessa F a 1·10⁻⁹ |

Tolleranze e la finestra di 15 ms sono fissate qui, prima di qualunque misura con la nuova procedura.

## 3. Predizioni sulla fixture attuale (seed 1337, golden PCM `18256941985354732249`)

| ID | Predizione |
|---|---|
| **Q1** | Trama: nessun punto con offset fuori da [−4,33; +25,67] ms viene scelto. δ scelto fra 0,03 e 0,15, offset fra 10 e 12 ms, richiamo degli attacchi ≥ 0,98 |
| **Q2** | Pulso: scelta invariata rispetto alla procedura attuale (δ 0,3, offset −12,67 ms) |
| **Q3** | Vettore: scelta invariata (δ e offset della procedura attuale) |
| **Q4** | Corpo: offset invariato (−10,67 ms); δ senza predizione |
| **Q5** | `acao02_v1` resta `INCONCLUSIVE`: il profilo calibrato continua a differire da quello congelato del 2026-10-29, perché il segnale è cambiato (F4a, FM, F2). Il pacchetto rende impossibile la degenerazione, non ricongela il profilo |

## 4. Limiti

- Le etichette dipendono dall'event log: un rilascio che cade entro 50 ms dall'attacco successivo viene attribuito all'evento più vicino, e se ne riporta il numero.
- La finestra di 15 ms vale per questa STFT (hop 256). Cambiare i descrittori richiede una nuova pre-registrazione.
