# M2.5 CP-3 — I9 quinquies, il salto della frase V. Pre-registrazione

- **Stato:** scritto il 2026-11-25, **prima** di codice, render e misure. L'hash SHA-256 (LF) è registrato in `progress.md`. Dopo l'hash non si modifica.
- **Origine:** [I9 quater](M2_5_CP3_I9d_Breath_Confirmation_Results.md), rilievo spontaneo: «forzato» in sei estratti su otto, cinque esattamente sul salto Sol5–Mi6 (9 semitoni). Spiegazione post hoc, qui messa alla prova. Solo offline, seed 1337; stesso kick, basso e tempo; nulla adottato.
- **Classe (D-M2-52):** difetto percepito.

## 1. Versioni (tutte con il respiro pieno di R1)

Cambia solo la frase V di A e A′ (frasi 2, 6, 18, 22 da 0). In B la V scende e non si tocca. Formato (sedicesimo, durata, grado).

| Versione | Battuta 2, nota 24 | Battuta 3 | Salto scritto | Nota più alta |
|---|---|---|---|---|
| **R1** | (24,8,3) | (32,12,8)(44,4,6) | 79 → 88: 9 | 88 |
| **V1**, più bassa | (24,8,3) | (32,12,7)(44,4,5) | 79 → 86: 7 | 86 |
| **V2**, preparata | (24,8,6) | (32,12,8)(44,4,6) | 84 → 88: 4 | 88 |

V1 abbassa la cima e il salto. V2 tiene la cima e prepara il salto. Se il difetto è il **salto**, V2 lo toglie e V1 meno; se è l'**altezza**, V2 non lo toglie. Conflitti con Corpo: regola di I9.

## 2. Criteri

| ID | Criterio | Previsione |
|---|---|---|
| Y1 | Fuori dalle quattro frasi V tutto identico a R1; R1 rigenerata uguale a `k3_R1` (CSV identico, render entro 0,001) | Sì |
| Y2 | Conflitti 0; note spostate ≤ 20 % per sezione | Sì |
| Y3 | Almeno metà delle note sopra Corpo in ogni frase | Sì |
| Y4 | Salto d'ingresso in V dopo gli spostamenti: V1 ≤ 7, V2 ≤ 5, in tutte e quattro le frasi; salto massimo del brano < 9 in entrambe | Sì |
| Y5 | V resta diversa da S′: almeno 2 note diverse in ogni V | Sì |
| Y6 | Render MEASURED, limiter 0 dB, nessuno scarto, picco ≤ −1 dBFS; estratti entro 0,1 LU | Sì |

Y1 e Y6 bloccano la consegna; gli altri si riportano senza ritocchi.

## 3. Ascolto cieco del titolare

- **Dodici estratti di 20 s:** 3 versioni × 4 punti (battute 10, 26, 74, 90). Inizio della battuta a un secondo casuale fra 6 e 14 in ogni estratto. Ordine casuale, mai lo stesso punto o la stessa versione di seguito. Loudness alla più debole, dissolvenze di 0,5 s. Circa 4 minuti.
- **Una domanda:** c'è un passaggio che suona forzato? Se sì, a che secondo?
- **Conteggio:** vale se il secondo indicato dista al massimo 1 dal secondo intero in cui cade il salto. Altri secondi si riportano ma non contano.

| ID | Previsione |
|---|---|
| S1 | R1 segnalata in almeno 3 punti su 4 (il difetto si riproduce) |
| S2 | V2 segnalata in 0 su 4 |
| S3 | V1 segnalata in meno punti di R1 |

**Regola**
1. **S1 mancata:** la spiegazione non si riproduce; nessuna versione è candidata e R1 resta.
2. **S1 confermata:** è candidata una versione segnalata in **0** punti su 4. Con 1 su 4 l'indicazione è debole e non basta.
3. Se entrambe sono a 0, sceglie il titolare con un confronto semplice fra le due, dichiarato come scelta artistica.
4. Se nessuna è a 0, R1 resta e V si riscrive solo con una nuova pre-registrazione.

Se ogni estratto fosse segnalato con la stessa probabilità, S1 insieme a 0 su 4 in una data versione avrebbe probabilità al massimo del 2,3 % (4,6 % considerando due versioni). Suppone risposte indipendenti.

## 4. Limiti

- Un ascoltatore che sa che si prova il salto; l'istante no. Battute 26 e 90 già sentite in I9 quater, 10 e 74 no.
- Battute 88–91 senza kick in tutte le versioni: il punto 90 cade lì, e il contesto è diverso dagli altri tre.
- Motivi d'autore, un seed, timbri provvisori. Il ritmo uguale delle frasi non si tocca.
- V1 e V2 cambiano la melodia senza garanzia che piaccia di più: la prova riguarda solo il «forzato».
