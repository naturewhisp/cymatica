# Proposta D-M2-32 — Anticipare struttura e sviluppo del brano

- **Stato:** **approvata** dal titolare il 2026-11-17, nella variante «prima si chiude M2.5.2» (F4b e livello della Trama). Registro: ADR-0007 §11, D-M2-32; sotto-fasi in ADR-0007 §3. Testo originale della proposta invariato qui sotto.
- **Origine:** domanda del titolare del 2026-11-16: perché far ascoltare sempre lo stesso brano, senza variazioni apparenti? Vedi `progress.md` e ADR-0007 D-M2-30.
- **Fonti:** [ADR-0007](adr/0007-m2-5-entry-decisions.md) §3 (sotto-fasi), §4 (CP-3), §5 (P-M25-06); [dossier R-M25-7](research/M2_5_F5_Music_Director_Research.md) (contratto minimo, opzione B; definizioni operative di sviluppo; batteria d'ascolto); [dossier delle evidenze](m2_procedural_evidence.md) (`NULL_DIVERSITY`, Teorema D); [risultati di M2.5.1c](research/M2_5_1c_Ensemble_Tools_Results.md) (metrica M4 non validata).

## 1. Problema

- Il brano della fixture è lo stesso da M2.4. Il planner sceglie da una tabella fissa di ricette e il seed agisce solo sui pareggi: sui seed da 1 a 64 la diversità misurata è nulla (`NULL_DIVERSITY`).
- Il ritmo di Pulso, Trama e Vettore è fisso per 64 battute; la ripetitività data dall'esperto è 7 su 7 (2026-10-28).
- In ADR-0007 lo sviluppo, M2.5.4, è **condizionale** a CP-3 e arriva dopo mix (M2.5.2) e struttura (M2.5.3). Così ogni ascolto umano prima di M2.5.4 riguarda lo stesso brano con poche costanti cambiate, e D-M2-30 esclude proprio questo tipo di ascolto.
- La sessione alla cieca di M2.5.5 richiede materiale diverso: oggi la sua specificità non è misurabile (Teorema D, `INSUFFICIENT_DATA`).

## 2. Proposta

1. **M2.5.3 e M2.5.4 diventano una sola fase, «Struttura e sviluppo», non più condizionale.** Parte quando M2.5.2 è chiusa (F4b, F2, rapporto FM). Il rapporto FM va prima perché il timbro della Trama condiziona la scelta delle sue note (R-M25-7).
2. **Perimetro:** F3 (continuità dei pattern ai confini, transizioni), F1 (àncora metrica dichiarata) e il **contratto minimo** di R-M25-7, opzione B, nell'architettura attuale. Un cambiamento per pacchetto, in quest'ordine:
   1. diade di Corpo sui punti contraddetti;
   2. lettura dell'accordo da parte di Vettore, con la sua ablazione;
   3. `dominantRole` che guida, con il registro di Vettore in A′;
   4. entrate e uscite dei ruoli;
   5. variazione **derivata e ripresa** (lignaggio dei motivi a capacità fissa);
   6. infine il seed che agisce oltre i pareggi, su una tabella chiusa d'autore.
3. **Fuori perimetro:** §16.3–§16.4 completo (form planner con memoria, budget di novità, Pacing) resta a M6, come raccomanda R-M25-7. Varietà autentica fra partite e sessione infinita restano a M6–M7. D-M2-19 resta: niente riscrittura del planner.
4. **Verifica:**
   - per ogni pacchetto, golden di piano, trace e PCM aggiornati con il motivo e predizioni registrate prima;
   - per lo sviluppo, le prove simboliche di R-M25-7: rimescolamento, ablazione, fixture avverse, variogramma contro nullo per permutazione;
   - il controllo F7, la melodia dell'esperto, quando arriva;
   - nessuna metrica usata nello scoring del planner vale come prova (Sturm 2016, Yang e Lerch 2020).
5. **Ascolto:** uno solo, umano e cumulativo, a fine fase, sul brano nuovo (D-M2-30). Riprende le domande rimaste aperte di CP-2 (Q3 per sezione, F1) e di CP-3, con la batteria B0, B1, B2, B5 e B6 di R-M25-7. Il protocollo P-M25-03 resta valido.
6. **CP-3:** la domanda «dove va F5» è risolta da questa proposta, con il contratto minimo in M2.5 e il resto a M6. CP-3 si tiene a fine fase, sull'esito dell'ascolto.

## 3. Alternative

| Alternativa | Pro | Contro |
|---|---|---|
| **A. Piano attuale** (M2.5.3, poi CP-3, poi M2.5.4 condizionale) | Meno rischio, nessuna modifica all'ADR | Altri ascolti sullo stesso brano, oppure nessun ascolto fino a M2.5.4; il planner si tocca due volte; la specificità della sessione alla cieca resta non misurabile |
| **B. Questa proposta** | Un solo passaggio sul planner; materiale nuovo per l'ascolto; sblocca M2.5.5 | Cambia l'ordine di ADR-0007; rischio di varietà fittizia, mitigato dalle prove del §2.4 |
| C. §16.3–§16.4 completo ora | Copre tutto | Escluso da D-M2-19 e da R-M25-7: il Pacing Director non esiste ancora |

## 4. Rischi e limiti

- **Varietà fittizia:** lo sviluppo non ha una metrica validata (M4 `ORDER_FREE` anche sui controlli Bach e F9). Il giudizio finale resta umano, come D-M2-30 prevede per le domande musicali.
- **Golden:** il brano di riferimento cambia. Il golden storico `15126969907236845529` (M2.4) e quello attuale `365661633327826884` restano come baseline documentali (D-M2-19).
- **Calibrazione AC-AO-02:** gli stem cambiano ancora; la ricalibrazione resta a M2.5.5, dopo il congelamento del contenuto.
- **Aspettative:** in M2.5 si ottiene sviluppo *dentro* il brano e una varietà limitata fra seed, non brani radicalmente diversi fra partite.

## 5. Contratti toccati, se approvata

ADR-0007 §3 (sotto-fasi), §4 (CP-3), §5 (P-M25-06, F5); specifica §32 solo se cambia il deliverable di M2.5 (da verificare all'approvazione); D-M2-03 (due voci per ruolo) invariato.
