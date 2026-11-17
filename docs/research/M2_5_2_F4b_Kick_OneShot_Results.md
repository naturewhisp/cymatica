# M2.5.2 — F4b, kick one-shot: risultati

- **Pre-registrazione:** [M2_5_2_F4b_Kick_OneShot_Preregistration.md](M2_5_2_F4b_Kick_OneShot_Preregistration.md), SHA-256 (LF) `c57ad2173ca412ad88cb6503a43ce0f0a8f960bad76e2c4e626927e87263c853`, registrata prima del codice e delle misure.
- **Esecuzione, 2026-11-17:** misure prima del cambio, poi Release e Debug, `stem_export_v2`, `trama_stereo_v1` (loudness per stem), `fade_events_v1` e `full_m2_suite`. Dati: [esperimento](experiments/m25_2_f4b_kick_oneshot/README.md).
- **Decisione:** Pulso **one-shot** (D-M2-35). Verifica autonoma (D-M2-30).

## 1. Esiti

| ID | Predizione | Risultato | Esito |
|---|---|---|---|
| B1 | Prima: durata mediana 52–66 ms | 64,2 ms (p05 55,0; p95 64,3) | Confermata |
| B2 | Dopo: durata mediana 68–84 ms | **76,1 ms** (p05 69,4; p95 76,2) | Confermata |
| B3 | 0 `release` di Pulso (prima 416); nessun `steal` di Pulso | 0 e 0; il registro passa da 1 582 a 1 166 eventi | Confermata |
| B4 | Piano, trace ed eventi invariati; Trama e Vettore identici; Corpo cambia; golden PCM cambia | Piano e trace PASS, hash degli eventi uguale; Trama e Vettore identici; Corpo −24,819 → −24,825 LUFS (ducking più lungo); golden `18256941985354732249` → **`8159713709400020277`** | Confermata |
| B5 | Pulso +0,3…+2 LU; energia sotto 100 Hz in aumento | **+0,05 LU** (−26,487 → −26,441); quota sotto 100 Hz 0,736 → 0,741 | **Mancata** per la loudness; confermata per l'energia grave |
| B6 | Limiter 0 dB; true peak ± 0,5 dB | 0 dB; −6,866 → −6,867 dBTP | Confermata |

**Perché B5 manca:** la coda aggiunta (da circa 64 a 76 ms) è la parte finale di un decadimento lineare, con ampiezza sotto il 20 % del picco: aggiunge poca energia. La predizione aveva sovrastimato l'effetto sulla loudness di una coda debole.

## 2. Lettura

- Il kick ora suona **l'inviluppo progettato** (ADR-0004 §2.1): un decadimento di 71–82 ms che arriva a zero da solo. Non c'è più la dissolvenza di 2 ms a −26 dBFS che lo interrompeva a ogni colpo.
- La durata cresce di circa il 19 %, mentre la loudness e il «peso nelle basse» cambiano di poco. Se il suggerimento dell'esperto («più peso nelle basse, durata oltre i 54–62 ms») chiede di più, serve **F4c**: un decadimento più lungo, o un corpo più grave, con una sua pre-registrazione e un criterio. Le misure di questo passo dicono già che la sola durata, a questo inviluppo lineare, sposta poco la loudness.
- Pulso non lascia più eventi di dissolvenza: le 1 166 interruzioni rimaste sono di Corpo, Trama e Vettore (H-03).

## 3. Codice

- **`voice_allocator.cpp`, `isOneShotVoice`:** voce di Pulso con sustain 0 e timbro di sintesi.
  - Il suo NoteOff viene riconosciuto (`Applied`, catena degli eventi invariata) ma non la interrompe.
  - La scadenza di durata di `checkLifetimes` non la tronca.
  - Furto di voce, Pause, Stop e la scadenza dei pad aperti restano invariati.
- **Test:** `[pulso_one_shot]`.
- **Golden PCM:** aggiornato con il motivo in `audio_probe.h`.
- **Build e suite:** Release e Debug headless `/WX` 293/293; `full_m2_suite` verde salvo `acao02_v1`, il limite noto.
