# M2.5 CP-3 — I1 e I2, kick: risultati dell'indagine

- **Pre-registrazione:** [M2_5_CP3_I1_I2_Kick_Preregistration.md](M2_5_CP3_I1_I2_Kick_Preregistration.md), SHA-256 (LF) `c381dad6576dd40afa947d7f9ae6943df53d193acf29ef31f93d7f27ebca9d5e`.
- **Esecuzione:** 2026-11-23. **Dati:** [esperimento](experiments/m25_cp3_kick/README.md).
- **Stato:** proposta al titolare. Nessun cambiamento al gioco: gli override sono solo offline.

## 1. Previsioni

| ID | Misura | Esito |
|---|---|---|
| K1 | Oggi E_K: A −18,59, B −19,85, A′ −18,51 dB. B è **1,26 dB più debole** di A | Confermata |
| K2 | P_body +23,4 / +14,0 / +23,4 dB; P_click **−2,4** / +4,8 / **−2,0** dB | Confermata |
| K3 | Click +6 dB: P_click +6,0 dB in ogni sezione; mix +0,06 LU; limiter 0 dB | Confermata |
| K4 | Altezza fissa: differenza di E_K fra A e B ≤ 0,03 dB a ogni altezza | Confermata |
| K5 | Golden PCM PASS senza override; eventi, Trama e Vettore identici con ogni override | Confermata |

**Altezza prevista dalla regola (MIDI 38): mancata.** Il Re2 ha Q100 fra 0,71 e 0,72 in ogni sezione, sotto la soglia di 0,75. Anche il kick di oggi in A e A′ (Re2) ha 0,71.

## 2. Altezze fisse (click invariato)

| MIDI | Nota, Hz | E_K minimo | Q100 (minimo) | Mix | Ammessa |
|---:|---|---:|---:|---:|:-:|
| 31 | Sol1, 49,0 | −19,86 | 0,888 | −0,22 LU | sì |
| 33 | La1, 55,0 | −19,38 | 0,835 | −0,11 LU | sì |
| **35** | **Si1, 61,7** | **−19,00** | **0,773** | **0,00 LU** | **sì, scelta dalla regola** |
| 36 | Do2, 65,4 | −18,85 | 0,757 | +0,04 LU | sì |
| 38 | Re2, 73,4 | −18,59 | 0,710 | +0,12 LU | no (Q100) |

La regola sceglie **35**: il massimo è del 36, ma il 35 gli è a 0,14 dB, dentro la parità di 0,2 dB, e a parità vale la più grave.

**Riserva, fuori dalla regola:** il Si1 non appartiene alla scala di B (Sol eolio ha il Si♭) e ne dista un semitono. Il Do2 appartiene a tutte e due le scale ed è a 0,14 dB. Per l'esperto la tonalità non conta, ma la parità era nella mia regola e non era pensata per questo caso. La scelta fra 35 e 36 la fa il titolare, oppure la decide l'ascolto di conferma con entrambe.

## 3. Click (altezza invariata)

| Guadagno | P_click A / B / A′ | Mix | Energia sopra 6 kHz | Ammesso |
|---:|---|---:|---:|:-:|
| 0 dB | −2,4 / +4,8 / −2,0 | 0 | 0 | no |
| **+3 dB** | **+0,6 / +7,8 / +1,0** | **+0,02 LU** | **+1,7 dB** | **sì, scelto** |
| +6 dB | +3,6 / +10,8 / +4,0 | +0,06 LU | +3,9 dB | no (> 3 dB) |
| +9 dB | +6,6 / +13,8 / +7,0 | +0,13 LU | +6,4 dB | no |

## 4. Lettura

- **Il «cambio di nota» ha anche un effetto di livello:** al confine il kick perde 1,26 dB pesati K. La pesatura K considera solo la risposta d'ascolto media; su diffusori piccoli il divario potrebbe essere maggiore (direzione coerente con Deruty, misurata però sull'808).
- **La presenza manca nel transiente in A e A′, non in B.** Il corpo domina già la banda 40–160 Hz, perché nessun altro ruolo la occupa: alzare il livello generale non è la leva. Quando arriverà il basso (I3) questa riserva si ridurrà e P_body va rimisurato.
- **La soglia P_click ≥ 0 dB è ragionata, non validata.** Dice che il click pareggia gli altri strumenti in quella banda, non che piaccia.
- Le due scelte sono state misurate separatamente. La combinazione va misurata prima di adottarla.

## 5. Proposta

1. Adottare, dopo una misura della combinazione, l'altezza fissa (35 oppure 36) e il click a +3 dB.
2. Rimandare il giudizio finale all'ascolto di conferma, insieme a I3–I5.
3. Riaprire la scelta se I3 occupa la stessa banda.

## 6. Fonti mancanti

Hove e altri (PNAS 2014, PMC4104866) e il lavoro PMC5293018 erano dietro una verifica anti-automazione, quindi non letti. Di Deruty ho letto solo una parte. Il titolare può scaricarli; nessun risultato di questa indagine ne dipende.

## 7. Combinazione Do2 e click +3 dB (2026-11-23)

[Pre-registrazione](M2_5_CP3_I1_I2_Kick_Combination_Preregistration.md), con la deroga del titolare alla parità della regola. Le varianti già misurate danno gli stessi numeri.

| ID | Misura | Esito |
|---|---|---|
| C1 | P_click +0,61 / +7,81 / +0,97 dB | Confermata |
| C2 | E_K: A −18,79, B −18,77 dB; differenza 0,02 dB (oggi 1,26) | Confermata |
| C3 | Q100 0,754 / 0,761 / 0,759, sopra 0,75 di poco | Confermata |
| C4 | Mix +0,06 LU; limiter 0 dB; sopra 6 kHz +1,7 dB; true peak −5,81 dBTP | Confermata |
| C5 | Eventi, Trama e Vettore identici | Confermata |

**Esito: proponibile.** In A e A′ il kick scende da Re2 a Do2 e perde 0,2 dB di E_K; in B ne guadagna 1,1. Il margine su Q100 è di 0,004 in A: il kick è appena dentro la soglia.

**Fonti (sostituisce §6):** i tre articoli sono stati letti per intero dopo questa indagine, vedi il [dossier](M2_5_CP3_Kick_Bass_Form_Sources.md). La pre-registrazione dell'indagine resta com'era, con lo stato di lettura di allora.
