# M2.5.3–4 — Contratto minimo, punto 3: `dominantRole` guida il registro. Risultati

- **Pre-registrazione:** [M2_5_3_C3_Dominant_Register_Preregistration.md](M2_5_3_C3_Dominant_Register_Preregistration.md), SHA-256 (LF) `a146be539ef786ffa13b89b8e9585e022d9b7e6ce6aa6078daf234a6073019a0`.
- **Scelta del titolare:** «Melodia sopra Corpo».
- **Esecuzione:** 2026-11-17.
- **Dati:** [esperimento](experiments/m25_3_c3_dominant_register/README.md).
- **Decisione:** D-M2-43.

## 1. Cosa cambia

Quando il ruolo dominante dichiarato è Vettore, il vincitore sposta tutta la sua melodia della frase di 0, +12 o +24 semitoni. Vale il primo spostamento che mette almeno metà delle note strettamente sopra la voce alta di Corpo che suona al loro attacco, pad tenuti dalla frase precedente inclusi. Classi di altezza, attacchi e durate non cambiano. Lo spostamento è nel trace (hash canonico del piano, formato 4), e il controllo d'invarianza del motivo lo ricostruisce.

## 2. Esiti

| ID | Predizione | Risultato | Esito |
|---|---|---|---|
| D1 | +12 nelle frasi 0–7, 0 nelle 8–15, +24 nelle 16–23 | Esattamente così; selezione invariata | Confermata |
| D2 | Sopra Corpo: A 124, B 71, A′ 120 | A 124 / 4 uguali / 0 sotto; B 71 / 40 / 17; A′ 120 / 8 / 0 | Confermata |
| D3 | Vettore 72–81 in A e A′, 60–67 in B | Sì | Confermata |
| D4 | Pulso, Corpo e Trama identici; in Vettore cambiano solo le ottave | Sì | Confermata |
| D5 | Vicinanza alla Trama: A 16, A′ 32, B 3 | 16, 32, 3 | Confermata |
| D6 | Distanze M5 di Vettore identiche; picchi di Corpo alle frasi 9 e 17 | Successioni di intervalli e di attacchi identiche in 24 frasi su 24; picchi a 9 e 17 con M4, M5 e M5r, ancora ammesse | Confermata |
| D7 | Fixture PASS con golden aggiornati; `meter_change_v1` PASS in M1–M5 | Sì; anche il PCM di `meter_change_v1` cambia | Confermata |

**Nota su D2:** la diagnosi contava 127 note di Vettore in B. Lo script del punto 3 tiene conto anche dei pad della frase precedente e ne conta 128. Il numero di note sopra Corpo, 71, coincide.

**Non misurato, previsto come descrittivo:** gli spostamenti scelti in `meter_change_v1` e nella forma di prova. Sono nei rispettivi trace, ma non li ho estratti.

## 3. Lettura

- **In A e A′ la melodia guida:** è sopra il supporto in 124 e 120 attacchi su 128, contro 24 e 0 di prima. A′ torna nello stesso registro di A (72–81): la ripresa riprende anche l'altezza.
- **Il prezzo:**
  - la melodia è ora vicina alla Trama in 16 note su 128 in A e 32 in A′, contro 0 di prima;
  - al confine A→B la melodia scende di un'ottava, e a B→A′ risale.

  Sono gli elementi da sentire: voce 11 del registro.

## 4. Cosa resta

- R-ROL-2: altri segni di salienza del dominante, cioè attacco, densità e timbro.
- Il mascheramento percettivo fra Vettore e Trama sul render, con il pacchetto 4.
- La fragilità dei margini di selezione (punto 6).

## 5. Golden e build

| Golden | Prima | Dopo |
|---|---|---|
| Trace della fixture | 7802016689135067568 | 7620653259624784472 |
| Piano della fixture (formato 4) | 0x18cc6d32dc2fa60c | 0x17aade206b9ac268 |
| PCM della fixture | 9396686444379387342 | 11240606778404684750 |
| Piano della forma di prova A→B | 0x078e7ac77d936659 | 0xb13b4a4e01a8fa6d |
| PCM di `meter_change_v1` | 16485786595596680340 | 16313085494776772472 |

Release e Debug `/WX` 301/301, con un test nuovo, `[dominant_register]`; `full_m2_suite` verde salvo `acao02_v1`, il limite noto.
