# M2.5.3–4 — Contratto minimo, punto 1: diade di Corpo. Risultati di C1, C1b e C1c

- **Pre-registrazioni:**

  | Variante | SHA-256 (LF) |
  |---|---|
  | [C1](M2_5_3_C1_Corpo_Dyad_Preregistration.md) | `73123df7420311c4a998490d3416f1ede622bc943f176ac133a4aab858191eba` |
  | [C1b](M2_5_3_C1b_Corpo_Dyad_Symmetric_Preregistration.md) | `5e2ec7c5e9675c8a47bba5ae562fd3764823bb5643ab60a3fe98f8232b845251` |
  | [C1c](M2_5_3_C1c_Corpo_Dyad_After_Selection_Preregistration.md) | `d8ada8f860854ef1acaad48a1fec679f413ba8b22e5556be737c609c0094f46a` |

  C1b e C1c sono dichiarate come informate dagli esiti precedenti.
- **Dati e approfondimento:** [esperimento](experiments/m25_3_c1_corpo_dyad/README.md).
- **Decisione:** si adotta **C1c**, scelta del titolare il 2026-11-17 (D-M2-41).

## 1. Il problema

Sull'accordo di Sol, in A e A′, Corpo suonava Sol + Si contro il Fa di Vettore: un tritono di 8,0 s in 16 battute. La regola R-HAR-5 indica di riparare prima la diade di Corpo, poi il grado di Vettore, infine di scartare il candidato.

## 2. Le tre varianti

| | C1: solo la terza | C1b: simmetrica, nella selezione | C1c: simmetrica, dopo la selezione |
|---|---|---|---|
| Battute contraddette (A, B, A′) | 8, 0, 8 (Mi contro Fa) | **0, 0, 0** | **0, 0, 0** |
| Vincitori diversi da F1 | frasi 0–3 | frasi 0–3 e 16–23 | **nessuno** |
| Melodia (Vettore) | cambia in 0–3 | cambia in 0–3 e 16–23 | **identica** |
| Esito | negativo | ripara, ma cambia la melodia per un margine dello 0,4% | adottata |

## 3. Esiti di C1c

| ID | Predizione | Risultato | Esito |
|---|---|---|---|
| R1 | Selezione identica a F1: vincitori, punteggi, nessun pareggio | Sì, nelle 24 frasi | Confermata |
| R2 | 0 battute contraddette; Corpo–Vettore ic6 e ic1 = 0 in A e A′; ic1 in B 6,2% | 0, 0, 0; ic6 e ic1 = 0 in A e A′; B 6,2% | Confermata |
| R3 | Pulso, Vettore e basso identici; Corpo di B identico; voce alta cambiata solo nelle 16 battute di Sol | Esattamente così: battuta 1 di ogni frase di A e A′, Si → Re | Confermata |
| R4 | 2 riparazioni per frase in A e A′, 0 in B; nessuna respinta | Sì | Confermata |
| R5 | Trama cambia solo nelle battute di Sol con Trama (frasi 4–7, 16–23) | Sì, solo la battuta 1 di quelle frasi | Confermata |
| R6 | Picchi di novità di Corpo alle frasi 9 e 17 | 9 e 17 con M4, M5 e M5r; M5 e M5r ancora ammesse | Confermata |
| R7 | Fixture PASS con golden aggiornati; `meter_change_v1` PASS in M1–M5 | Sì; anche il PCM di `meter_change_v1` cambia | Confermata |

**Effetto descrittivo, non predetto:** anche il tritono Trama–Vettore di A e A′ sparisce. Era al 10% e ora è a 0, perché nelle battute di Sol la Trama legge la diade riparata.

## 4. Cosa resta

- La settima maggiore Mib–Re di B (6,9 s) è fra il basso di Corpo e Vettore. Spetta al punto 2, Vettore che legge l'accordo.
- **Fragilità della selezione:** in A e A′ il vincitore supera il secondo dello 0,4%. È un rilievo aperto per il punto 6.
- **Ipotesi per il punto 3:** «Vettore sopra Corpo in A′», emersa per caso con C1b, va dichiarata e misurata insieme alla separazione da Trama.

## 5. Golden e build

| Golden | Prima | Dopo |
|---|---|---|
| Trace della fixture | 17158827938453439384 | 7595762444317258244 |
| Piano della fixture (formato canonico 2) | 0x7aa477a5519f9cf8 | 0xc5fc6a007f0a0a34 |
| PCM della fixture | 8239929959925757461 | 795499972965883485 |
| Piano della forma di prova A→B | 0x6d73bd838ab32526 | 0xa3ca0dbd6a6f9d0e |
| PCM di `meter_change_v1` | 58770584194502935 | 9026598872691749635 |

- **Formato canonico 2:** il trace di ogni frase aggiunge `corpoDyadRepairedBars` e `corpoDyadRepairRejected`. Le due voci entrano anche nell'hash del trace della fixture.
- **Build:** Release e Debug `/WX` 299/299; c'è un test nuovo, `[corpo_dyad]`.
- **Suite:** `full_m2_suite` verde salvo `acao02_v1`, il limite noto.
