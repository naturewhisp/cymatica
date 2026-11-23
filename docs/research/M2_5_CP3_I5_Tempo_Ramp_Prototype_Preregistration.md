# M2.5 CP-3 — I5, prototipo della rampa di tempo. Pre-registrazione

- **Stato:** scritto il 2026-11-23, prima del codice e degli ascolti. L'hash SHA-256 (LF) è registrato in `progress.md`. Dopo l'hash il file non si modifica.
- **Decisione del titolare:** «fai commit e procedi con la rampa».
- **Ipotesi:** il passaggio A→B suona brusco per il salto di tempo (esperto il 2026-11-03, titolare il 2026-11-23).

## 1. Metodo

Si prende il piano di riferimento (seed 1337, kick attuale) e si spostano nel tempo i suoi eventi, conservando la posizione in battute e tempi. Stesse note, altezze e dinamiche. Render offline con il motore reale; `MusicClock` e il gioco non cambiano.

| Versione | Tempo |
|---|---|
| **a** | com'è oggi: 120, poi 140 di colpo alla battuta 32 |
| **b** | B a 120 (nessun cambio) |
| **c** | una battuta di raccordo in più prima di B, a 120: solo Pulso e Trama della battuta 31 e il pad di Corpo prolungato; poi 140 |
| **d** | rampa lineare nel tempo metronomico da 120 a 140 lungo le battute 30–31 (8 tempi); B parte già a 140 |
| **e** | B→A′ com'è oggi, riferimento |

Estratti di 8 battute prima e 8 dopo il confine, dissolvenza di 10 ms, loudness pareggiata alla più debole.

## 2. Verifiche

| ID | Previsione |
|---|---|
| V1 | Il render di **a** col nuovo strumento, sul brano intero, differisce dal mix di riferimento al massimo di 0,001 (picco) |
| V2 | Salto massimo fra durate di tempi consecutivi: a e c 14,3 %; b 0; d ≤ 2,5 % |
| V3 | Limiter fermo (0 dB) e nessun evento perso in ogni versione |
| V4 | Loudness degli estratti entro ± 0,5 LU prima del pareggio |

Se V1 fallisce, gli estratti non sono validi e non si ascoltano.

## 3. Previsioni d'ascolto

- **Se la causa è il salto di tempo:** b e d non brusche, a e c brusche.
- **Se è la mancanza di preparazione:** c meno brusca di a; d forse no.
- **Se b è ancora brusca:** il tempo non è la causa, o non è l'unica.

Il **titolare** può ascoltare a, b, c, d **alla cieca**: nomi mescolati con un seed che sta solo nella chiave. È un ascoltatore che conosce l'ipotesi; la conferma resta all'esperto.

## 4. Limiti

- In **b** e **d** cambia anche la durata del brano e dell'estratto; in **b** B suona più lento per tutto l'estratto, quindi meno eccitata: è la versione che isola il tempo, non una proposta.
- Il raccordo **c** è un minimo, non una composizione.
