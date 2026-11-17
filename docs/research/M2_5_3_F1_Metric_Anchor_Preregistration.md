# M2.5.3–4 — F1, àncora metrica dichiarata: pre-registrazione

- **Stato:** scritto il 2026-11-17, prima del codice e di ogni misura. L'hash SHA-256 (LF) è registrato in `progress.md`. Dopo l'hash il file non si modifica.
- **Decisioni di riferimento:** ADR-0007 D-M2-32 (perimetro), D-M2-38 (E3 ordine: F1 dopo F3), D-M2-39 (policy del planner 1.0.2.0 per la fase), D-M2-19 (metodo), D-M2-30 (verifica autonoma e ascolto cumulativo).
- **Regole:** R-MET-1 e R-MET-2 del [dossier del direttore musicale](M2_5_F5_Music_Director_Research.md) e delle [note sulle regole d'insieme](experiments/m25_f5_research_notes/regole_insieme.md). Da CP-2: F1 «si legge come stato metrico da dichiarare, non come difetto da eliminare».

## 1. Diagnosi dal codice e dal piano attuale

In `generateCandidates` la rotazione di Pulso vale `k % 4`, dove k è l'indice del candidato. Lo spostamento metrico è quindi derivato dall'indice del candidato, ed è proprio ciò che R-MET-1 vieta.

Sul piano attuale (commit `7287aed`, dati [F3](experiments/m25_3_f3_boundary/fixture_intents_policy1020_f3.csv)), la prima battuta di Pulso è questa:

| Frasi (da 0) | Attacchi di Pulso, in sedicesimi | Tempo 1 |
|---|---|---|
| 0–3 | 2, 5, 7, 10, 13, 15 | assente |
| 4–8 | 0, 4, 8, 12 | presente |
| 9–23 | 2, 6, 10, 14 | assente |

Il kick evita il tempo 1 in 19 frasi su 24, senza che nessuno l'abbia deciso. È la voce 7 del registro delle domande («kick fuori dal tempo forte in B e A′»).

## 2. Variabile

1. **Campo di frase** (R-MET-1): `PhrasePlan::metricState` ∈ {`Ancorato`, `Spostato`, `Ritorno`} e `PhrasePlan::pulsoDisplacementSteps`, dichiarati dall'espansore delle frasi come la maschera di Trama e la tabella delle ricette. Il default è `Ancorato` con spostamento 0. Ogni forma che non dichiara nulla, comprese la forma di prova e quella di `meter_change_v1`, è quindi ancorata.
2. **Planner:** la rotazione di Pulso di ogni candidato è `pulsoDisplacementSteps` se lo stato è `Spostato`, altrimenti 0. Il numero di impulsi di Pulso e tutti i parametri di Trama restano scelti come oggi; la continuità F3 continua a portare gli impulsi e i parametri di Trama. Uno stato incoerente (spostamento diverso da 0 fuori da `Spostato`, oppure `Spostato` con spostamento ≤ 0 o ≥ passi della battuta) è una `StructuralInvariantViolation` del piano.
3. **Profilo iniziale della fixture, `fixture-metric-v1`** (valore iniziale, non validato):
   - A (frasi 0–7) e A′ (16–23): `Ancorato`;
   - B: 8–11 `Ancorato` (esposizione del riferimento nel nuovo tempo), **12–13 `Spostato` di 2 sedicesimi** (un ottavo, R-MET-8: 214 ms a 140 BPM), 14 `Ritorno`, 15 `Ancorato`.

   Motivo: R-MET-6 chiede riferimento esposto, spostamento e ritorno su un confine ipermetrico entro la sezione. Lo spostamento resta nella sezione di contrasto, dove già c'era. Lo spostamento di 2 passi è quello del materiale attuale, quindi non è una scelta nuova. `Ritorno` si comporta come `Ancorato` per il planner, ma resta nel piano come marcatore del ciclo.
4. **Policy:** resta 1.0.2.0, la versione del planner per tutta la fase (D-M2-39). Il cambio si documenta nella storia dei golden.

## 3. Predizioni

| ID | Predizione |
|---|---|
| **P1, R-MET-1** | Nella fixture la rotazione di Pulso del vincitore è ≠ 0 solo nelle frasi 12 e 13, dove vale 2 |
| **P2, R-MET-2** | In ogni frase non `Spostato` della fixture, Pulso ha un attacco sul passo 0 di ogni battuta. Fra i quattro passi del quarto, la fase 0 ha il massimo degli attacchi di Pulso, eventualmente a pari merito |
| **P3, nessun effetto collaterale** | Il candidato vincente (indice) di ogni frase della fixture non cambia, e restano 0 i pareggi di punteggio fra candidati validi in testa. Corpo, Trama e Vettore restano identici in tutte le 24 frasi |
| **P4, dove cambia Pulso** | Pulso cambia nelle frasi 0–3, 9–11 e 14–23, e resta identico nelle frasi 4–8 e 12–13 |
| **P5, sezioni** | La novità di Foote su Corpo ha ancora i due picchi più alti alle frasi 9 e 17 (numerazione da 1), con M4, M5 e M5r |
| **P6, scenari** | La fixture resta PASS in ogni controllo, con i golden di piano, trace e PCM aggiornati col motivo. `meter_change_v1` resta PASS in M1–M6, con il golden del PCM aggiornato. Il golden del piano della forma di prova A→B cambia |

**Base di P3:** le caratteristiche del punteggio (voice leading di Corpo, tensione dal numero di impulsi, fedeltà del motivo, densità degli attacchi, furti di voce) non dipendono dalla rotazione di Pulso. L'unica eccezione possibile sono i furti di voce simulati: se P3 manca, la causa da verificare è quella.

Il valore musicale (si batte il piede a tempo? lo spostamento in B si sente come scelta?) non si misura qui: va all'ascolto cumulativo, voce 7 del registro, riformulata.

## 4. Limiti

- P1 e P2 sono vere per costruzione: verificano che l'implementazione faccia ciò che dichiara.
- R-MET-2 si verifica sul solo Pulso, il ruolo-àncora dichiarato. Il punteggio di fase pesato (R-MET-3) e la banda di sincope (R-MET-4) restano fuori.
- Il profilo `fixture-metric-v1` è una scelta d'autore iniziale, non derivata da dati: la sua posizione e la sua lunghezza si valutano all'ascolto cumulativo.
- La forma di gioco adattiva (M3 in poi) dovrà dichiarare lo stato dal direttore; qui vale solo il default ancorato.
