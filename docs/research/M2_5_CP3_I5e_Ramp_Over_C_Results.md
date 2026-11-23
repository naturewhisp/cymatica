# M2.5 CP-3 — I5e, rampa di tempo A→B sopra c: risultati

- **Pre-registrazione:** [documento](M2_5_CP3_I5e_Ramp_Over_C_Preregistration.md), SHA-256 (LF) `84e86a35ab675652011d12490467b21c4baae165a7f332841ace9359381eb6c9`.
- **Dati:** `experiments/m25_cp3_transitions/ramp_over_c.{py,json}`.

| ID | Esito |
|---|---|
| R1 | n identico al render di c (0); prima della rampa r8 e r16 identici a n (0): confermata |
| R2 | Salto fra battiti vicini: n 14,3 %, r8 2,0 %, r16 1,0 %: confermata |
| R3 | MEASURED, limiter 0 dB, nessuno scarto: confermata |
| R4 | Loudness entro 0,11 LU, nessun fondo scala: confermata |

Dopo la misura è stata tolta dallo script un'espressione di R2 ridondante, poi sovrascritta dal criterio corretto: l'esito non cambia.

**Ascolto:** in attesa.

## Risposte del titolare (2026-11-23), trascritte prima di aprire la chiave

> 01: = / 02: 2 / 03: = / 04: = / 05: 1 / 06: 2 / 07: 1 / 08: 1 / 09: 2 / 10: = / 11: = / 12: 2

**Chiave, aperta dopo:** 01 r8/n, 02 n/r8, 03 r8/r16, 04 n/r8, 05 r8/n, 06 r8/r16, 07 r16/r8, 08 r8/n, 09 n/n, 10 r16/r8, 11 r8/r8, 12 n/r8.

| Confronto | Esito |
|---|---|
| n contro r8 (6) | r8 **4**, uguali 2, n 0 |
| r8 contro r16 (4) | r16 **2**, uguali 2, r8 0 |
| Identiche (2) | 11 «uguali», corretta; 09 (n/n) scelto il secondo: **rumore** |

- **Regola registrata:** 4 su 6 è sotto 5, quindi **n e r8 non distinguibili**. La previsione (5 o 6) è **mancata**. Fra r8 e r16 nessun 4 su 4: per regola si tiene **r8**.
- **Identiche:** un solo lato su due, quindi la clausola «esiti deboli» non scatta, ma il rumore c'è: in ter e qui una identica su due riceve una preferenza.
- **Osservazione post hoc, non prova:** nessuna risposta è andata contro la rampa; le 6 espresse nei confronti veri vanno tutte verso il tempo più graduale. Non era la regola, unisce due confronti e una coppia identica ha dato lo stesso tipo di risposta. Quattro «=» su 10 coppie reali indicano un effetto piccolo o poco udibile a questo confine.
- **Rispetto a I5:** lì la rampa era preferita su estratti con altra melodia, senza ripetizioni né coppie identiche; quel sostegno va ridimensionato, non annullato.
- **Lettura:** salto della melodia (ter) e cambio di tempo (qui) non bastano, ciascuno, a distinguere con sicurezza il passaggio. Cause non separate qui: cambio di centro, tonalità, kick, Corpo, entrate e uscite.
- **Limiti:** un ascoltatore, che conosce l'ipotesi; una sola coppia identica per tipo.

**Esito:** la rampa resta un candidato **non dimostrato**; non giustifica da sola un ADR su `MusicClock`. Decisione insieme alle altre, con l'ascolto dell'esperto. Nulla adottato.
