# M2.5.3–4 — F3, continuità dei pattern ai confini di sezione: pre-registrazione

- **Stato:** scritto il 2026-11-17, prima del codice e di ogni misura. L'hash SHA-256 (LF) è registrato in `progress.md`. Dopo l'hash il file non si modifica.
- **Decisioni di riferimento:** ADR-0007 D-M2-32 (perimetro), D-M2-38 (E2 versione della policy, E3 ordine), D-M2-19 (metodo), D-M2-30 (verifica autonoma e ascolto cumulativo).
- **Giudizio umano già raccolto** (2026-11-03): la transizione A→B è «molto brusca», B→A′ «più fluida», dove i pattern proseguono. Indicazione della [proposta d'ingresso](M2_5_Entry_Music_Quality_Proposal.md): «non cambiare i pattern di Pulso e Trama sullo stesso confine del tempo, prima di valutare rampe di BPM».

## 1. Diagnosi dal codice

In `generateCandidates` i parametri euclidei di Pulso (impulsi, rotazione) e di Trama dipendono solo dall'indice del candidato. A ogni frase il candidato vincente li sceglie di nuovo, anche nella prima frase di una sezione, dove cambiano insieme anche tempo, modo, motivo e ricette. Al confine A→B cambia quindi tutto nello stesso istante.

## 2. Variabili, in due passi

1. **Passo 0, versione della policy (D-M2-38, E2):** `DEFAULT_POLICY_VERSION_ID` da 1.0.1.0 a **1.0.2.0**. La versione entra nelle chiavi dei pareggi; il seed agisce solo sui pareggi, e nella fixture se ne contano 0 (Teorema D).
2. **Passo 1, continuità (F3):** nella **prima frase di ogni sezione dopo la prima** (frasi 8 e 16 della fixture, numerazione da 0), tutti i candidati usano i parametri euclidei di Pulso e Trama della frase vincente precedente. Dalla seconda frase della sezione si torna alla scelta normale. Tempo, modo, motivo, ricette, Corpo e Vettore non cambiano. Lo stato si salva nel checkpoint del planner, come la voce di Corpo.

## 3. Predizioni

| ID | Predizione |
|---|---|
| **P0** | Dopo il passo 0 gli intent della fixture (frame, ruolo, altezza, velocity, durata) sono identici a prima, in tutte le 24 frasi. Cambiano solo il campo `policyVersion` del trace, gli hash che lo includono e quindi il golden del trace |
| **P1** | Dopo il passo 1 cambiano solo le frasi che seguono un confine: la 8 sicuramente; la 16 solo se i parametri della 15 e di quella che avrebbe vinto in 16 differiscono; la 9 e la 17 al più per lo stato della voce di Corpo. Tutte le altre frasi restano identiche |
| **P2** | Nella frase 8, gli attacchi di Pulso e di Trama nella prima battuta coincidono, in passi della griglia, con quelli dell'ultima battuta della frase 7. Prima del cambio non coincidono |
| **P3** | Le sezioni restano distinguibili per le misure ammesse: la novità di Foote su Corpo (M5, M5r e M4) ha ancora i due picchi più alti alle frasi 9 e 17 (numerazione da 1) |
| **P4** | La fixture resta PASS in ogni controllo tranne i golden, aggiornati con il motivo: piano, trace e PCM. `meter_change_v1` resta PASS e invariato |

Il valore musicale (se A→B risulti meno brusca) non si misura qui: va all'ascolto cumulativo, nel registro delle domande.

## 4. Limiti

- P2 è vera per costruzione: verifica che l'implementazione faccia ciò che dichiara, non che il risultato sia migliore.
- La continuità vale per una frase sola. Una transizione più graduale, con rampa di tempo o modulazioni ritmiche, è un'altra variabile (CP-3 o CP-4).
