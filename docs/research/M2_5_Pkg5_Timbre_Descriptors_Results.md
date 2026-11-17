# Pacchetto di misura 5 — Descrittori timbrici e separabilità: risultati

- **Pre-registrazione:** [M2_5_Pkg5_Timbre_Descriptors_Preregistration.md](M2_5_Pkg5_Timbre_Descriptors_Preregistration.md), SHA-256 (LF) `0f90c18d842754c1cd0244c22a45c72aa4e0925e25b13611d0237f4f0167c2e6`.
- **Esecuzione, 2026-11-17:** Release, scenario `timbre_descriptors_v1`. Note isolate di Corpo, Trama e Vettore dal motore offline, con i `timbreParam` del planner (0,40 / 0,50 / 0,60). Dati: [esperimento](experiments/m25_pkg5_timbre/README.md).
- **Esito:** strumento **validato** (D1–D5). P1 e P2 confermate, **P3 mancata**.

## 1. Validazione

| ID | Atteso | Risultato | Esito |
|---|---|---|---|
| D1 | Corpo diviso per altezze alterne: ≤ 0,67 | 0,50 | Confermata |
| D2 | Sinusoide contro dente di sega: 1,0 | 1,0 | Confermata |
| D3 | FM 1,88 contro dente di sega: 1,0 | 1,0 | Confermata |
| D4 | Oracolo numpy indipendente, centroide e dispersione ± 2 % | Scarto massimo 4,8·10⁻⁹ su 18 note | Confermata |
| D5 | Ripetizione | Identica | Confermata |

**Correzioni dei test unitari, prima della misura:**
- Il rapporto dispari/pari atteso per il dente di sega era sbagliato: sulle prime 10 armoniche vale analiticamente +5,10 dB, e ora il test verifica quel valore.
- Il test del classificatore usava dati in cui cinque descrittori variano dentro i gruppi in modo identico nei due gruppi. Con 6 elementi, l'esclusione del singolo sposta la media del proprio gruppo e porta a sottostimare la separabilità: è un limite del leave-one-out su campioni piccoli, e vale anche per le misure reali.

## 2. Misure

Medie per ruolo, sulle 6 altezze:

| Ruolo | Centroide armonico | Dispersione / f0 | Dispari/pari | Attacco | Pendenza del centroide | Flusso |
|---|---:|---:|---:|---:|---:|---:|
| Corpo | 1,94 | 1,76 | +4,94 dB | 12,3 ms | −0,36 /s | 0,01 |
| Trama | 2,15 | **0,71** | **−3,01 dB** | 20,5 ms | **+1,61 /s** | 0,02 |
| Vettore | 1,87 | 1,58 | +4,92 dB | 5,5 ms | −0,05 /s | 0,02 |

| ID | Predizione | Risultato | Esito |
|---|---|---|---|
| P1 | Trama contro Corpo ≥ 0,9 | **1,0** (distanza standardizzata 4,21) | Confermata |
| P2 | Trama contro Vettore ≥ 0,9 | **1,0** (4,39) | Confermata |
| P3 | Dispari/pari della Trama entro ± 3 dB da Corpo | −3,01 contro +4,94 dB: **7,95 dB** di scarto | **Mancata** |

Senza predizione: Corpo contro Vettore 0,917 (distanza 2,53). Sono i due ruoli più simili, entrambi denti di sega filtrati.

## 3. Lettura

- **La Trama a FM 1:1 è separabile da Corpo e da Vettore** in tutte le note. La differenza non è nel centroide (2,15 contro 1,94), ma nella **forma dello spettro**: dispersione molto più stretta (0,71 contro 1,76), armoniche pari più forti delle dispari, attacco più lento, centroide che sale nella nota.
- **P3 mancata.** Avevo dedotto «tutte le armoniche, quindi simile al dente di sega»: era sbagliato. Con portante e modulante uguali le bande laterali si ripiegano con segno alterno (come già visto in D-M2-33 per la loudness). La distribuzione risultante, all'indice che decade con l'inviluppo, favorisce le armoniche pari. La preoccupazione di D-M2-33, «spettro armonico come Corpo», risulta più debole del previsto sul piano dei descrittori.
- Separabilità nello spazio dei descrittori non vuol dire distinguibilità percepita: la domanda resta all'ascolto cumulativo (D-M2-30), che ora ha un'ipotesi misurata da verificare.

## 4. Codice

`timbre_descriptors.{h,cpp}`: sei descrittori, classificatore leave-one-out, distanze, scenario `timbre_descriptors_v1`. Usa `renderSyntheticWithRelease` per le note e `encodeWavStereoFloat32` per i WAV dell'oracolo. Test `[timbre_descriptors]`.
