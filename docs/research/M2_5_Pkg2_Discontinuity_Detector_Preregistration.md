# Pacchetto di misura 2 — Rilevatore di discontinuità (click): pre-registrazione

- **Stato:** scritto il 2026-11-17, prima del codice e di ogni misura. L'hash SHA-256 (LF) è registrato in `progress.md`. Dopo l'hash il file non si modifica.
- **Autorizzazione:** pacchetto 2 (A1 + A2, §7) del [dossier delle lacune di misura](M2_5_Measurement_Gaps_Research.md), approvato il 2026-11-17, con la regola di validazione comune del dossier.
- **Lacuna:** nel PCM reso non c'è nessun descrittore dei click. `artifact_present(click)` dell'evaluation pack termina con `unverifiable`, e i rilevatori di R-M25-8 guardano il device, non il contenuto.

## 1. Strumento (offline, `engine/cacp/`, nessuna dipendenza)

**A1, per canale di ogni stem e del mix:**
1. predittore lineare di **ordine 16**, stimato per blocchi di **1024 campioni** con l'autocorrelazione della finestra di Hann del blocco (Levinson-Durbin); residuo e[n] calcolato con i campioni veri precedenti, anche oltre il bordo del blocco;
2. scala robusta del blocco: s = 1,4826 · mediana(|e|) sul blocco, con un minimo di 1·10⁻⁶;
3. **candidato** dove |e[n]| / s > **10**; candidati entro 2 ms fusi nel primo;
4. annotazione con l'evento più vicino entro ± 5 ms dell'event log dello stesso ruolo (per il mix: di qualunque ruolo): «attacco» (NoteOn applicato), «rilascio» (NoteOff applicato), «altro evento», altrimenti **«inspiegato»**.

Il risultato è una tabella di conteggi per stem, canale e categoria, mai un booleano.

**A2, differenziale:** A1 applicato alla differenza B − A di due render. Due render identici danno zero candidati per costruzione. In questo pacchetto A2 si consegna come funzione con i suoi test; il suo primo uso reale è l'A/B del rilascio (H-03).

**Escluso, dichiarato:** l'energia sopra 8 kHz in finestre di 1 ms proposta dal dossier non entra in questo pacchetto. Se il residuo predittivo basta ai controlli, non serve.

## 2. Validazione

| ID | Prova | Esito atteso |
|---|---|---|
| **W1, positivo** | Nel mix della fixture si iniettano 50 impulsi di un campione e 50 gradini (offset continuo per 10 ms, poi tolto) per ciascuna ampiezza −60, −40 e −20 dBFS, in posizioni scelte da un generatore deterministico con seed dichiarato nel codice | Richiamo (un candidato entro 2 ms dall'iniezione) non decrescente con l'ampiezza; **1,0 a −20 dBFS** e **≥ 0,9 a −40 dBFS**; −60 dBFS senza soglia |
| **W2, negativo** | Sinusoide a 440 Hz, −6 dBFS, 10 s, rampe di 50 ms all'inizio e alla fine | **0 candidati** |
| **W3, negativo** | Stem di Pulso della fixture: gli attacchi del kick sono transitori legittimi | **0 candidati «inspiegati»**: i candidati del kick sono tutti attribuiti ad attacchi o rilasci |
| **W4, invarianza** | Due esecuzioni dello scenario | Tabelle identiche |
| **W5, A2** | Due render identici; un render con un impulso a −40 dBFS iniettato | 0 candidati; 1 candidato nella posizione dell'impulso |

Oracolo: non c'è uno strumento di terzi dedicato (dossier §7). Lo si dichiara come limite.

## 3. Misura descrittiva sulla fixture attuale

Tabella dei candidati per stem e categoria sul render della fixture (seed 1337, golden PCM `18256941985354732249`). Senza predizioni, oltre a W3: è la prima misura dei click nel contenuto. Il titolare sospetta le dissolvenze di 2 ms, cioè i 190 tagli di rilascio di Corpo (H-03), e la tabella le conta senza dire se si sentano.

## 4. Limiti

- Una discontinuità rilevata non è per forza udibile: dipende da livello e mascheramento (pacchetto 4). A1 trova discontinuità, non decide l'udibilità.
- Soglia (10), ordine (16) e blocco (1024) sono fissati qui. Cambiarli richiede una nuova pre-registrazione.
- Esito `MEASURED`, mai `PASS`, finché il titolare non ratifica una soglia.
