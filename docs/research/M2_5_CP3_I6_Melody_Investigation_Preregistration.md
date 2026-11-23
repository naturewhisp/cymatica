# M2.5 CP-3 — I6, melodia: indagine. Pre-registrazione

- **Stato:** scritto il 2026-11-23, prima delle misure. L'hash SHA-256 (LF) è registrato in `progress.md`. Dopo l'hash il file non si modifica.
- **Decisione:** D-M2-51; il titolare lascia scegliere fra I5 e I6. Si sceglie I6: ha la stessa causa di I4 (pattern di una battuta).
- **Giudizi da avvalorare:** «una figura che si ripete» (entrambi gli ascoltatori); «un pattern di 3 note» (ascoltatore A); prevedibilità 5/7 (esperto) e 3/7 (A).
- **Già noto:** il ritmo di Vettore è identico nelle quattro battute (G1 di I4, 100 %); la progressione di 6b sposta la figura sugli accordi; il 100 % delle frasi ha almeno tre battute di altezze diverse.
- **IDyOM:** il piano lo prevede ed è approvato, ma non è installato e non c'è un corpus di melodie. Al suo posto un **modello a breve termine autoaddestrato**: PPM di ordine 0–2 con esclusione semplice, alfabeto di intervalli in semitoni fra note successive, addestrato solo sulle note precedenti dello stesso brano. Non è IDyOM e non rappresenta un ascoltatore con esperienza musicale.

## Misure (Vettore; token di `form_investigation_v1`)

**Contorno** di una battuta: segni (−, 0, +) degli intervalli fra note successive. **Figura:** intervalli esatti.

| ID | Misura | Previsione |
|---|---|---|
| M1 | Battute 2–4 con lo stesso contorno della battuta 1 della frase, riferimento | ≥ 70 % |
| M2 | Lo stesso, mediana sui 64 seed | ≥ 60 % |
| M3 | Contorni diversi usati in una sezione, media delle tre, riferimento | ≤ 6 |
| M4 | Contenuto informativo medio (bit per nota) della seconda metà di ogni sezione meno quello della prima, riferimento | < 0 in almeno due sezioni su tre: la melodia diventa più prevedibile |
| M5 | Frasi la cui ultima nota è la tonica della sezione, frasi pari e dispari (candidate domanda e risposta), riferimento | differenza fra i due gruppi < 25 punti: nessuna domanda-risposta |
| M6 | Attacchi di Vettore per battuta: frasi in cui la battuta 4 ha meno attacchi delle altre (respiro a fine frase), riferimento | 0 |

## Lettura stabilita prima

- **M1 e M2 vere:** la progressione è percepita come figura perché il contorno resta uguale: la leva è un motivo di due battute con **contorno diverso** nella seconda.
- **M5 e M6 vere:** mancano chiusura e respiro: la leva comprende una cadenza alla fine della seconda battuta.
- **M1 falsa:** il contorno varia già, e «figura» dipende da altro, per primo il ritmo.
- Le leve restano proposte, con una pre-registrazione propria.

## Limiti

- Nessun corpus umano di confronto: le soglie sono ragionate.
- Il modello a breve termine misura la prevedibilità **interna** al brano.
