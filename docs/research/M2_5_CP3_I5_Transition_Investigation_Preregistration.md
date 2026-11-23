# M2.5 CP-3 — I5, raccordi: indagine. Pre-registrazione

- **Stato:** scritto il 2026-11-23, prima delle misure. L'hash SHA-256 (LF) è registrato in `progress.md`. Dopo l'hash il file non si modifica.
- **Decisione:** D-M2-51, [piano di CP-3](../M2_5_CP3_Expert_Plan.md), I5; il titolare: «Commit e I5».
- **Giudizio da avvalorare:** passaggio A→B «brusco» (esperto il 2026-11-03 e il 2026-11-19, ascoltatore A).
- **Già noto:** al confine cambiano insieme tempo (120 → 140), centro (Re → Sol), modo, motivo e nota del kick; F3 tiene Pulso e Trama uguali per una frase; nessun riempimento (I4); il pad di Corpo prosegue un tempo dentro B.
- **Domanda:** è brusco **per quanto cambia** o perché **nulla lo prepara**? Da questo dipende se servono una rampa di tempo o un raccordo.

## 1. Misure

Audio: brano di riferimento (`base_mix`), brano del seed 34, mix dell'esperto decodificato (fuori dal repository). Finestre di 2 battute prima e dopo ogni linea di battuta, con le griglie note (esperto a 162 BPM; confini dichiarati nella sua nota: 0:47, 2:01, 2:45, 3:50, portati alla battuta più vicina).

- **Spettro:** distanza euclidea fra i livelli medi in dB di 24 bande logaritmiche (40 Hz–16 kHz), tolta la media.
- **Croma:** 1 − coseno fra profili di 12 classi (80–2000 Hz).
- **Livello:** differenza di energia pesata K, in dB.
- **Preparazione:** differenza fra le ultime 2 battute prima del confine e le 2 precedenti, per livello e per energia sopra 4 kHz (in dB). «Preparato» se almeno una delle due supera 1,5 dB in valore assoluto.

| ID | Previsione |
|---|---|
| T1 | A→B è fra i 3 confini con la croma più distante, sui 92 interni del riferimento |
| T2 | Nello spettro A→B non supera il 95° percentile: gli strumenti sono gli stessi |
| T3 | A→B e B→A′ **non** preparati, nel riferimento e nel seed 34 |
| T4 | Almeno 3 dei 4 confini dell'esperto preparati |
| T5 | Quota di linee di battuta «preparate» per caso, non di confine: esperto minore di 50 % (altrimenti T4 non dice nulla) |
| T6 | Gradino di livello ad A→B ≥ 1 dB in valore assoluto, riferimento |
| T7 | Il pad tenuto di A forma almeno una classe d'intervallo 1 con note che attaccano nel primo tempo di B, riferimento |

## 2. Lettura stabilita prima

- **T1 vera e T2 falsa o vera:** dice che cosa cambia (armonia, timbro o entrambi).
- **T3 e T4 vere con T5 vera:** manca la preparazione; la prima alternativa è un raccordo, e la rampa di tempo (che estende `MusicClock`) non è giustificata da questa indagine.
- **T3 falsa o T4 falsa:** il paragone non sostiene il raccordo.
- **T7 vera:** il pad va trattato comunque, perché scontra.

## 3. Limiti

- Un brano d'esempio, quattro confini; il suo tempo non cambia, il nostro sì.
- Le distanze non sono un modello di «bruschezza»: nessuna soglia è validata.
