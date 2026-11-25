# H-03 — Null test del rilascio per ruolo: pre-registrazione

- **Data:** 2026-11-24; **seconda firma 2026-11-25**, dopo la review del titolare e prima di codice e misure. Sostituisce la versione con SHA-256 `7bf78349…`. Non si modifica più.
- **Cambi della seconda firma:** regola di Corpo senza A1; C3 su un NoteOff dello slot 7; profilo degli onset dichiarato e P7 corretta; diadi, controllo sui mezzi-render, C5 a 96 campioni, finestra della Trama a 120 ms; nuovi C6, C7 e P10.
- **Domanda:** dove si trova, per ruolo e nel tempo, la differenza fra un rilascio di 2 ms e uno più lungo? Quanto pesano i tagli di Corpo?
- **Origine:** ADR-0007 §8.1 (null test a −14 dB su clip giudicate uguali; ipotesi del titolare sui 2 ms), scadenza CP-4.
- **Non decide:** il valore del rilascio. Produce misure e una raccomandazione per CP-4. Nessuna modifica al runtime di produzione, nessun golden aggiornato.
- **Materiale:** fixture di oggi, seed 1337. I candidati di CP-3 non sono adottati: se il contenuto cambia, la misura si ripete prima del congelamento.

## 1. Cosa è già noto (dichiarato)

- Pulso è one-shot (D-M2-35): il NoteOff non lo interrompe.
- `fade_events_v1` (2026-11-17, prima di alcuni cambi di piano): tagli 0 / 190 / 0 / 0.
- Letto oggi sul piano esportato, prima di scrivere le predizioni: note 436 / 192 / 252 / 516; NoteOff sullo stesso frame di un attacco dello stesso ruolo 0 / 190 / 40 / 515; nessun attacco fra 0 e 80 ms dopo un NoteOff non coincidente.
- Letto nel codice: Corpo usa due voci per accordo, quindi il nuovo accordo riprende entrambe. Trama e Vettore hanno una seconda voce libera.
- Letto nel codice: la coda dei tagli **non continua l'oscillazione**. Sfuma in 2 ms l'ultimo campione tenuto fermo. Allungarla produrrebbe una rampa continua, non un rilascio: **non si usa** come A/B.
- Letto nel codice il 2026-11-25: pan di Vettore 1,0/0,9 sullo slot 6 e 0,9/1,0 sul 7, con lo slot libero 6 scelto per primo. Anticipare il NoteOff di una nota dello slot 6 farebbe ricadere sul 6 la nota seguente e invertirebbe l'alternanza.
- A1 è `TOOL_NOT_VALIDATED` (pacchetto 2): 51 157 candidati su Corpo, dovuti ai reset del dente di sega.
- `release_ab_v1` (2026-11-03): H1 non sostenuta, H2 sostenuta, sul brano di allora.
- Ascolto del 2026-11-03: nessuna differenza percepita fra 2 e 80 ms.

## 2. Parte A, null test per ruolo

Variante di rilascio R = 15, 30, 50, 80 ms (override offline esistente), contro 2 ms. Per ogni ruolo: residuo = stem(R) − stem(2 ms), in virgola mobile, prima del limiter. L'override è globale, ma gli stem sono separati. Si registrano:
- livello del residuo rispetto allo stem (RMS, dB) e, in banda K, del residuo rispetto al mix (LU);
- quota d'energia del residuo entro la finestra [NoteOff, NoteOff + R + 5 ms]; per la Trama [NoteOff, NoteOff + R + 120 ms] (delay). Si riporta anche la copertura temporale delle finestre: se è alta, la quota dice poco;
- quota di ogni ruolo nell'energia totale dei quattro residui;
- differenza di loudness dello stem e del mix; riduzione massima del limiter; tagli e furti di voce;
- rilevazioni sui rilasci: rilevazioni non abbinate a un attacco e distanti al più 50 ms da un NoteOff del ruolo (`evaluateRoleOnsets`).
  - **Profilo usato dalla regola:** calibrazione etichettata del pacchetto 1, ricalcolata **una sola volta** sulla fixture di oggi a 2 ms e poi **tenuta fissa** per ogni R.
  - **Solo confronto:** `frozenStemProfileV1()`.
  - Se un ruolo non ha un punto ammissibile, le sue rilevazioni sono `INCONCLUSIVE` e non si propone un candidato;
- candidati A2 (`differentialCandidates`), solo descrittivi: A2 non ha ancora un W1 differenziale.

## 3. Parte B, code di Corpo

I tagli si evitano senza toccare il motore: le **diadi** di Corpo (note con lo stesso frame d'attacco, mai separate, nell'ordine del piano) si dividono in due gruppi alterni, pari e dispari, resi in due esecuzioni offline separate che contengono entrambe tutti gli eventi di Pulso, così il ducking è lo stesso. I due stem di Corpo si sommano. Ogni accordo ha così le sue voci, e la coda non viene tagliata.
- B0: somma con R = 2 ms contro produzione. Isola «taglio con campione fermo» contro «rilascio vero di 2 ms».
- B1: somma con R = 15, 30, 50, 80 ms contro B0. Isola la durata della coda.
- Si registrano residui e finestre come nella parte A. **A1 non si usa.**
- **Misura ai tagli, M_hi:** per ogni taglio, residuo D = produzione − B1(30 ms) del solo Corpo, filtrato passa-alto a 4 kHz (due sezioni Butterworth, 24 dB/ottava). M_hi = energia di D su [taglio, taglio + 3 ms) divisa per l'energia del mix di bus di produzione, filtrato allo stesso modo, sulla stessa finestra, in dB. Un taglio è **segnalato** se M_hi ≥ −10 dB. L'attacco nuovo è uguale nei due render e si cancella. Si riportano anche D = produzione − B0 e la banda intera.

## 4. Controlli (se uno fallisce, l'esito è `TOOL_NOT_VALIDATED`)

| ID | Controllo | Atteso |
|---|---|---|
| C1 | Due esecuzioni identiche, ogni ruolo | Residuo esattamente 0 |
| C2 | Vettore scalato di +1 dB dopo il render | Residuo −18,27 ± 0,05 dB rispetto allo stem |
| C3 | Un solo NoteOff di Vettore anticipato di 10 ms, scelto fra le note suonate dallo **slot 7** (quella più vicina a metà brano), così l'alternanza dei due slot non cambia; assegnazione degli slot verificata uguale | Energia del residuo ≥ 99,9 % entro la finestra che va da 10 ms prima a 20 ms dopo quel NoteOff |
| C4 | Eventi applicati uguali in ogni variante; golden PCM della produzione invariato | Sì |
| C5 | B0: campioni fuori da [taglio, taglio + 2 ms), 96 campioni esatti | Differenza massima ≤ 1e-6 rispetto alla produzione |
| C6 | Mezzi-render della parte B, ogni R | 0 tagli e 0 furti di voce per Corpo |
| C7 | Iniezione su M_hi: un impulso di un campione, su entrambi i canali, aggiunto a D = 0 in ogni taglio, a −60, −40, −20 e −10 dBFS | Senza iniezione 0 segnalati; conteggio non decrescente; 190 su 190 a −10 dBFS |

## 5. Predizioni

| ID | Predizione |
|---|---|
| P1 | Residuo di Pulso esattamente 0 per ogni R |
| P2 | Parte A, Corpo: i tagli restano 190 per ogni R, e l'energia del residuo di Corpo è confinata per ≥ 99,9 % dopo i NoteOff non coincidenti con un attacco |
| P3 | Trama e Vettore: energia del residuo entro le finestre ≥ 99,9 %, tagli e furti 0, per ogni R |
| P4 | Residuo di Trama e Vettore strettamente crescente con R |
| P5 | Vettore porta ≥ 80 % dell'energia totale dei residui, per ogni R |
| P6 | Limiter fermo (0 dB) per ogni R; loudness del mix entro ± 0,2 LU fino a 50 ms |
| P7 | Trama a 2 ms, profilo ricalibrato: rilevazioni sui rilasci ≥ 90 % dei **212** NoteOff non coincidenti con un attacco (≥ 191). I 40 coincidenti sono coperti dall'attacco. Vettore: meno di 20, perché 515 NoteOff su 516 coincidono |
| P8 | B0: residuo fra −60 e −25 dB rispetto allo stem di Corpo |
| P9 | B1: residuo fra −30 e −15 dB a 30 ms, fra −25 e −10 dB a 80 ms, strettamente crescente con R |
| P10 | Tagli segnalati da M_hi (produzione − B1 a 30 ms): meno di 19 su 190 |

P1, P2 e parte di P3 seguono in gran parte dal codice letto: confermarle esclude errori, non dimostra qualcosa sul suono.

## 6. Lettura e regola per CP-4 (fissata ora)

- **Origine dei −14 dB:** si attribuisce al ruolo con la quota maggiore. È una spiegazione sul brano di oggi; il materiale di allora non si rimisura.
- **Candidato per un ruolo (solo Trama e Vettore):** il più piccolo R che soddisfa tutte queste condizioni: rilevazioni sui rilasci del ruolo ≤ 20 % di quelle a 2 ms; tagli e furti 0; loudness del ruolo entro ± 0,5 LU e del mix entro ± 0,2 LU; limiter fermo. Se a 2 ms il ruolo ha meno di 20 rilevazioni sui rilasci, la condizione sulle rilevazioni è `NOT_APPLICABLE` e nessun candidato è proposto. **Per Vettore è atteso per costruzione** (515 NoteOff su 516 coincidono con un attacco): la regola non può proporre un rilascio per Vettore, e questo non significa che 2 ms sia il valore migliore.
- **Nessun candidato:** si propone di lasciare 2 ms. Il 2026-11-03 non è emersa differenza percepita, quindi la sola motivazione è tecnica.
- **Corpo:** se i tagli segnalati da M_hi (produzione − B1 a 30 ms) sono almeno 19 su 190 (10 %), si propone un A/B d'ascolto e poi un ADR sulle voci (D-M2-03). Altrimenti Corpo resta com'è. M_hi confronta energie in una banda: non è un modello di mascheramento, e un esito sotto soglia non prova che il taglio sia inudibile.
- **Pulso:** nessuna modifica. Pause, Stop e rilasci di sicurezza restano a 2 ms.
- **Predizioni mancate:** si riportano; nessun valore si ritocca dopo.

## 7. Esecuzione prevista

Nuovo scenario offline `release_null_v1` in `engine/cacp`, test dei controlli, dati in `docs/research/experiments/m25_h03_release_null/`. Nessuna dipendenza nuova. Build Release e Debug con `/WX` e suite completa. Nessun ascolto in questa fase.

## 8. Limiti

Un brano, un seed, un renderer. Le misure non dicono cosa si sente. Le finestre, i 4 kHz, i −10 dB, il 20 % e il 10 % sono soglie ragionate, non validate.
