# M2.5 CP-3 — I5c, ingresso di A′ senza salto. Pre-registrazione

- **Stato:** scritto il 2026-11-23, prima delle versioni e dell'ascolto. L'hash SHA-256 (LF) è registrato in `progress.md`. Dopo l'hash il file non si modifica.
- **Da:** [I5b](M2_5_CP3_I5b_BAprime_Harmony_Results.md): al confine il Vettore salta di 19 semitoni. Il titolare: «Commit e prova».
- **Vincolo:** rispettare D-M2-43 (almeno metà delle note di Vettore sopra la voce alta di Corpo) e non cambiare le classi d'altezza, così accordi e conflitti restano quelli di oggi.

## Versioni

Piano di riferimento, seed 1337, tempo e kick di oggi.

| Versione | Cambiamento |
|---|---|
| **p** | nessuno |
| **u** | frase 15 (ultima di B): da un certo punto in poi il Vettore sale di un'ottava. Il punto è quello che rende minimo il salto più grande fra note consecutive, dalla frase 15 alla prima nota di A′; a parità, il più precoce |
| **v** | frase 16: Vettore e Corpo (tutte e due le voci) scendono di un'ottava |
| **r** | frase 16: solo Vettore un'ottava sotto, come in I5b (ancora nota, **viola D-M2-43**) |

Estratti come in I5b: 16 s + 16 s, confine a 0:16, loudness pareggiata, nomi mescolati con la chiave a parte.

## Verifiche

| ID | Previsione |
|---|---|
| X1 | p uguale al riferimento entro 0,001 |
| X2 | Cambiano solo le note dichiarate; nessuna classe d'altezza |
| X3 | Salto più grande di Vettore da frase 15 alla prima nota di A′: u ≤ 9 semitoni; al confine in v ≤ 9 (p: 19) |
| X4 | Note sopra Corpo ≥ 50 % in frase 15 per u e in frase 16 per v |
| X5 | Conflitti Vettore–Corpo (classe 1 e 6) nelle frasi 15 e 16 uguali a p |
| X6 | Limiter fermo, nessun evento perso; loudness entro ± 0,5 LU da p |

Se X1, X2 o X6 falliscono, gli estratti non si ascoltano.

## Previsioni d'ascolto

- u e v meno forzate di p a 0:16; r meno forzata di p (replica di I5b).
- Se p non è la più forzata, il giudizio di I5b non si replica.

## Limiti

- Un solo ascoltatore, che conosce l'ipotesi.
- In **u** la salita è un cambio d'ottava scelto da un calcolo, non una melodia composta.
- In **v** Corpo scende nella zona del basso (I3).
