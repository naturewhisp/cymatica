# M2.5 CP-3 — I5e, rampa di tempo A→B sopra c: coppie cieche ripetute. Pre-registrazione

- **Stato:** scritto il 2026-11-23, dopo le [coppie b/c](M2_5_CP3_I4_I6c_Boundary_Pairs_Results.md) e **prima** di codice, render e misure. L'hash SHA-256 (LF) è registrato in `progress.md`. Dopo l'hash non si modifica.
- **Perché:** al confine il titolare sente ugualmente bruschi b e c; il tempo, uguale in entrambe, resta il candidato. Solo offline, seed 1337; nulla adottato. `MusicClock` non cambia.

## Versioni, tutte sul piano di c

| Versione | Tempo |
|---|---|
| **n** | oggi: 120 BPM, salto a 140 alla battuta 32 |
| **r8** | da 120 a 140 lineare nelle 8 battiti (2 battute) prima del confine |
| **r16** | da 120 a 140 lineare nei 16 battiti (4 battute) prima del confine |

Le posizioni in battiti, le altezze e le velocity non cambiano; cambiano solo i tempi (integrale esatto di 60/T).

## Criteri

| ID | Criterio |
|---|---|
| R1 | n uguale al render di c già fatto entro 0,001; prima della rampa r8 e r16 uguali a n entro 0,001 |
| R2 | Salto relativo massimo fra battiti vicini: n 1/7 (±0,01); r8 ≤ 2,5 %; r16 ≤ 1,3 % |
| R3 | Render MEASURED, limiter 0 dB, nessuno scarto né evento rimasto |
| R4 | Loudness degli estratti entro 0,5 LU; 12 coppie con ordini bilanciati; nessun campione a fondo scala |

Se uno fallisce non si consegna. Nessun guadagno viene applicato agli estratti.

## Ascolto

12 file da 29 s: due passaggi di 14 s (10 s prima e 4 s dopo il confine, dissolvenze di 10 ms), 1 s di silenzio. Il confine cade a 0:10 nel primo e a 0:25 nel secondo. Ordine casuale, seme segreto.
- **6 coppie n/r8**, 3 per ordine; **4 coppie r8/r16**, 2 per ordine; **2 identiche** (n/n, r8/r8).
- Domanda **unica**: quale passaggio A→B è meno brusco **nel momento in cui cambia**: 1, 2 o uguali. Non conta quale melodia piace.

**Regole:**
- n/r8: **r8 confermata** con 6 su 6 (p bilaterale 0,03), **favorita** con 5 su 6 (debole). Simmetrica per n. Altrimenti non distinguibili.
- r8/r16: 4 su 4 **favorita** (p 0,13, mai confermata). Altrimenti si tiene **r8**, la più corta.
- Se entrambe le identiche ricevono un lato, i risultati n/r8 si dichiarano deboli.

**Previsione:** r8 favorita o confermata su n (5 o 6). Nessuna previsione fra r8 e r16.

**Limiti:** la rampa si sente, quindi non è davvero cieca; il titolare conosce l'ipotesi e ha già preferito una rampa in I5; un ascoltatore; un confine.
