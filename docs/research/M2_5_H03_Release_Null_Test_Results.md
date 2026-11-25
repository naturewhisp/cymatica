# H-03 — Null test del rilascio per ruolo: risultati

- **Pre-registrazione:** [documento](M2_5_H03_Release_Null_Test_Preregistration.md), seconda firma, SHA-256 (LF) `bb45600631f0229bd21012fd29e4a7d54a8b4a0e98fec082ebfe2afa4d63f6e7`.
- **Esecuzione, 2026-11-25:** Release, `release_null_v1`, fixture di oggi, seed 1337. Dati: [esperimento](experiments/m25_h03_release_null/README.md).
- **Esito:** controlli C1–C7 superati, stato `MEASURED`. **P1–P9 confermate, P10 mancata.** Nessuna adozione: la scelta è di CP-4.

## 1. Controlli

| ID | Risultato | Esito |
|---|---|---|
| C1 | Ripetizione: stem e mix identici | Superato |
| C2 | −18,271 dB | Superato |
| C3 | Energia nella finestra 100 %, slot uguali (nota 708) | Superato |
| C4 | Stessi eventi, golden PCM `PASS` | Superato |
| C5 | Differenza fuori dai 96 campioni: 0 | Superato |
| C6 | 0 tagli, 0 furti in ogni mezzo-render | Superato |
| C7 | 0 senza iniezione; 4 / 20 / 190 / 190 note a −60 / −40 / −20 / −10 dBFS | Superato |

## 2. Predizioni

| ID | Risultato | Esito |
|---|---|---|
| P1 | Residuo di Pulso 0 | Confermata |
| P2 | 190 tagli per ogni R; energia di Corpo nelle finestre 100 % (2 NoteOff liberi) | Confermata |
| P3 | Trama e Vettore 100 %, 0 tagli e furti | Confermata |
| P4 | Crescente (tabella §3) | Confermata |
| P5 | Vettore 94,3–94,8 % dell'energia dei residui | Confermata |
| P6 | Limiter 0 dB; mix +0,02 / +0,05 / +0,09 LU a 15 / 30 / 50 ms | Confermata |
| P7 | Trama 212 rilevazioni sui rilasci su 212 NoteOff liberi; Vettore 1 | Confermata |
| P8 | B0: −29,6 dB | Confermata |
| P9 | B1: −27,5 / −24,1 / −21,8 / −19,6 dB | Confermata |
| P10 | 24 note segnalate (12 accordi su 95), previste meno di 19 | **Mancata** |

## 3. Parte A

Residuo rispetto allo stem; rilevazioni sui rilasci della Trama con il profilo ricalibrato (δ 0,15, +10,3 ms) e, solo per confronto, con quello congelato.

| R | Trama | Vettore | Mix, Δ LU | Trama, Δ LU | Vettore, Δ LU | Rilevazioni Trama, ricalibrato | congelato |
|---|---:|---:|---:|---:|---:|---:|---:|
| 2 ms | | | | | | 212 | 212 |
| 15 ms | −19,4 dB | −20,0 dB | +0,02 | +0,05 | +0,05 | 212 | 212 |
| 30 ms | −16,1 dB | −16,4 dB | +0,05 | +0,11 | +0,11 | 212 | 206 |
| 50 ms | −13,7 dB | −13,9 dB | +0,09 | +0,19 | +0,20 | 132 | 22 |
| 80 ms | −11,4 dB | −11,7 dB | +0,14 | +0,33 | +0,31 | **11** | 1 |

Il richiamo degli attacchi della Trama (ricalibrato) resta 1,0 a ogni R. Copertura delle finestre: al massimo 27 % (Trama) e 24 % (Vettore), quindi la quota del 100 % è informativa.

**Origine del residuo:** Vettore, con il 95 % dell'energia. Sul brano intero di oggi il residuo del mix a 80 ms è −16,6 dB; i −14 dB del 2026-11-03 erano misurati su clip del brano di allora e non si riproducono come numero.

## 4. Parte B, tagli di Corpo

| Confronto | Residuo rispetto a Corpo | Note segnalate da M_hi |
|---|---:|---:|
| Produzione − B0 (taglio contro rilascio vero di 2 ms) | −29,6 dB | 18 (9 accordi) |
| Produzione − B1, 15 ms | | 24 (12 accordi) |
| Produzione − B1, 30 ms | | **24 (12 accordi)** |
| Produzione − B1, 50 e 80 ms | | 24 (12 accordi) |

**Diagnostica non pre-registrata (descrittiva):** i 12 accordi segnalati a 30 ms sono **esattamente** i 12 cambi d'accordo senza un attacco di Pulso; nessuno degli 83 con il kick è segnalato. Otto stanno in B, fra 91,4 e 103,4 s, con Trama e Vettore; quattro verso la fine, fra 166,9 e 172,9 s, con Vettore. Il kick porta alte frequenze nella finestra di 3 ms e fa scendere il rapporto: M_hi segue quindi la presenza del kick almeno quanto il taglio. È coerente con una copertura da parte del kick, ma non la dimostra.

## 5. Regola applicata

| Ruolo | Esito della regola |
|---|---|
| Pulso | Nessuna modifica |
| Trama | **Candidato R = 80 ms:** 11 rilevazioni su 212 (5 %, soglia 20 %); Trama +0,33 LU, mix +0,14 LU; limiter, tagli e furti 0 |
| Vettore | `NOT_APPLICABLE` per costruzione (1 rilevazione): nessun candidato. Non significa che 2 ms sia il valore migliore |
| Corpo | 24 ≥ 19: **si propone un A/B d'ascolto**, e solo dopo un ADR sulle voci (D-M2-03) |

## 6. Cautele per CP-4

- **80 ms è l'ultimo punto della griglia**, e il valore dipende dal profilo: con quello congelato la stessa regola darebbe 50 ms (22 su 212). La regola usa il ricalibrato, come firmato.
- Il candidato della Trama serve al **rivelatore**, non all'orecchio: il 2026-11-03 nessuna differenza fra 2 e 80 ms. La variazione del mix (+0,14 LU) comprende il rilascio lungo anche di Vettore, quindi per la sola Trama è una sovrastima.
- **Corpo:** soglia superata di poco (12,6 % contro 10 %). In 3 ms il residuo a 30 ms contiene anche la coda legittima dell'accordo precedente, non solo l'eventuale click. Il confronto più mirato (produzione − B0) dà 18 note, sotto soglia.
- C7 segnala 4 note già a −60 dBFS: dove il mix ha pochissime alte frequenze M_hi è molto sensibile.
- Se CP-3 cambia il contenuto, la misura si ripete prima del congelamento. Nessun ascolto in questa fase.

## 7. Verifica

Release e Debug headless `/WX` **322/322** (3 test nuovi `[release_null]`). Seconda corsa identica alla prima, tolta la lista diagnostica. `full_m2_suite --pack-seed 987654321`: unica voce non valida `acao02_v1`, limite noto fino a M2.5.5, quindi verdetto `FAIL`. Una prima corsa della suite senza `--pack-seed` ha dato FAIL anche su `evaluation_check_selftest_v1`, comportamento già documentato. Produzione e golden invariati.

## 8. Correzioni dopo la review (2026-11-25)

| ID | Correzione |
|---|---|
| `DIF-H03-01` (MED) | `median_db` era lo 0,5° percentile (q su 0–100): ora è la mediana, con un test su vettori asimmetrici. Manifest rigenerato: cambiano solo i 10 `median_db` della parte B, che nessun controllo, predizione o regola usa |
| `DIF-H03-02` (LOW) | Lo scenario crea la cartella del manifest. Provato da una cartella vuota: manifest scritto |
| `DIF-H03-03` (LOW) | `calibrate()` usa la latenza intrinseca solo se lo stato è `Ok` e la mediana è finita; altrimenti il profilo resta assente (`INCONCLUSIVE`). Sulla fixture nulla cambia; il ramo di errore non ha un test |

Release e Debug headless `/WX` **323/323**. `full_m2_suite` non ripetuta: cambia solo `release_null`, che non ne fa parte.
