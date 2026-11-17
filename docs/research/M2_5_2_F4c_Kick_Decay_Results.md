# M2.5.2 — F4c, decadimento del kick: risultati

- **Pre-registrazione:** [M2_5_2_F4c_Kick_Decay_Preregistration.md](M2_5_2_F4c_Kick_Decay_Preregistration.md), SHA-256 (LF) `badc6fd6f4fde8a9d8e5d6775e54963e644efd521270bc8e1336189840ff5feb`.
- **Esecuzione, 2026-11-17:** Release `kick_decay_ab_v1`, poi `kick_decay_criterion.py`; adozione, Release e Debug, `trama_stereo_v1`, `fade_events_v1`, `full_m2_suite`. Dati: [esperimento](experiments/m25_2_f4c_kick_decay/README.md).
- **Decisione:** decadimento di Pulso **×1,5**, cioè 75 + 105 · `timbreParam` ms (D-M2-37), scelto dal criterio pre-registrato.

## 1. Scala e criterio

| k | Sovrapposizioni | Durata mediana | Quota < 100 Hz | Perdita di Corpo A / B / A′ | Mix | Pulso | N | C | S |
|---:|---:|---:|---:|---|---:|---:|:-:|:-:|:-:|
| 1,00 | 0 | 76,1 ms | 0,741 | 0,778 / 0,822 / 0,758 | 0 | 0 | sì | sì | sì |
| 1,25 | 0 | 96,1 ms | 0,787 | 0,781 / 0,825 / 0,759 | +0,18 LU | +0,81 LU | sì | sì | sì |
| **1,50** | **0** | **115,9 ms** | **0,819** | 0,785 / 0,828 / 0,761 | **+0,35 LU** | **+1,47 LU** | sì | sì | sì |
| 1,75 | 1 | 135,6 ms | 0,843 | 0,788 / 0,830 / 0,762 | +0,51 LU | +2,03 LU | no | sì | no |
| 2,00 | 1 | 154,9 ms | 0,861 | 0,792 / 0,833 / 0,763 | +0,66 LU | +2,52 LU | no | sì | no |

Limiter 0 dB e nessun furto di voce di Pulso a ogni k.

| ID | Predizione | Risultato | Esito |
|---|---|---|---|
| D1 | N soddisfatta per k ≤ 1,50 e violata per k ≥ 1,75 | Esatto: la sovrapposizione è quella dei due kick a 125 ms | Confermata |
| D2 | k scelto = 1,50 | 1,50 | Confermata |
| D3 | A 1,50: durata 100–125 ms; Pulso +0,8…+1,8 LU; quota sotto 100 Hz in aumento | 115,9 ms; +1,47 LU; da 0,741 a 0,819 | Confermata |
| D4 | Eventi invariati; Trama e Vettore identici | Identici a ogni k | Confermata |

## 2. Adozione

- **Costanti:** `kPulsoDecayBaseSec` 0,075 s e `kPulsoDecayTimbreSec` 0,105 s. La produzione coincide con la misura a k = 1,5 a tutte le cifre: mix −19,988 LUFS, Pulso −24,973 LUFS, true peak −6,62 dBTP, limiter 0 dB.
- **Golden PCM:** `8159713709400020277` → **`15689200359660949538`**. Piano, trace ed eventi invariati.
- **Registro delle dissolvenze:** 1 166 eventi, nessuno di Pulso.
- **Test:** quello del one-shot assumeva il vecchio decadimento di 71 ms. Ora rende fino a 150 ms e verifica in più che a 100 ms la voce suoni ancora.
- **Build e suite:** Release e Debug headless `/WX` 293/293; `full_m2_suite` verde salvo `acao02_v1`, il limite noto.
- **ADR-0004 §2.1:** l'intervallo di decadimento di Pulso passa da 50–120 ms a 75–180 ms (D-M2-37).

## 3. Lettura

- Dall'inizio di M2.5 il kick è passato da 54–62 ms troncati, −6 dB di bus, a 116 ms di inviluppo completo, +2 dB di bus. La loudness del kick è salita di circa 9,5 LU e la quota d'energia sotto 100 Hz da 0,74 a 0,82. Il suggerimento dell'esperto, «più peso nelle basse, durata oltre i 54–62 ms», ha ora una risposta misurata.
- Il limite oltre cui il kick si sovrapporrebbe al successivo, e alzerebbe il mix oltre +0,5 LU, è k = 1,75. Il valore scelto lascia quel margine.
- Se il kick ora sia «giusto» resta una domanda per l'ascolto cumulativo ([registro delle domande](../m2_5_cumulative_listening_questions.md), voce 5).
