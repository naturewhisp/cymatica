# M2.5.2 — F2, immagine stereo della Trama: risultati

- **Pre-registrazione:** [M2_5_2_F2_Trama_Stereo_Preregistration.md](M2_5_2_F2_Trama_Stereo_Preregistration.md), SHA-256 (LF) `26649356ba28e7c1550cf875e67d63cd2f523cac27818b83876769d51455ec1d`, registrata prima del codice di misura e del cambio.
- **Esecuzione, 2026-11-16:** nuovo scenario `trama_stereo_v1`, misurato prima e dopo il cambio; Release e Debug; `full_m2_suite --pack-seed 1618033988`. Dati: [esperimento](experiments/m25_2_f2_trama_stereo/README.md).
- **Decisione:** voce della Trama **centrata a potenza costante** (`kTramaCentrePan` 0,7906 su entrambi i canali). Prima era L 1,0 e R 0,5 sullo slot 4, l'unico usato. Delay invariato. Verifica autonoma (ADR-0007 D-M2-30), registrata come D-M2-34.

## 1. Esiti

| ID | Predizione | Prima | Dopo | Esito |
|---|---|---|---|---|
| S1 | ILD del primo fronte, prima, +6,0 ± 0,5 dB | **+6,02 dB** | — | Confermata |
| S2 | ILD del primo fronte, dopo, 0,0 ± 0,5 dB | — | **+0,00006 dB** | Confermata |
| S3 | ILD dell'intero stem: +6,0 ± 0,5 prima, 0,0 ± 0,5 dopo | +4,55 dB | **−1,47 dB** | **Mancata** in entrambe le parti |
| S4 | Loudness della Trama ± 0,2 LU | −34,122 LUFS | −33,776 LUFS (+0,35 LU) | **Mancata** |
| S5 | Piano, trace ed eventi invariati; altri stem identici; mix ± 0,2 LU; limiter 0 | — | Piano e trace PASS, hash degli eventi uguale; Pulso, Corpo e Vettore con loudness identica a tutte le cifre; mix +0,014 LU; limiter 0 dB; true peak −6,866 dBTP | Confermata |
| S6 | Correlazione L/R della Trama < 1 | 0,802 | 0,802 | Confermata |

Senza predizione:
- **Side/Mid:** da −7,60 a −9,33 dB.
- **ILD per sezione:** da +4,79 / +4,19 / +4,79 dB prima a −1,23 / −1,83 / −1,23 dB dopo.
- **Hash degli eventi:** quello del nuovo scenario usa una formula diversa da quello di `kick_level_ab_v1`, quindi i due valori non si confrontano fra loro. Dentro `trama_stereo_v1`, prima e dopo, è uguale.

## 2. Perché S3 e S4 mancano

- **Causa:** il delay della Trama somma al suono diretto un'eco ritardata: 11 ms a sinistra e 17 ms a destra, wet 0,325, con feedback. Per note tonali la somma diretto + eco dipende dall'autocorrelazione della nota a quel ritardo, e a 11 ms e a 17 ms è diversa. La differenza di filtraggio a pettine fra i due canali vale circa 1,5 dB di energia.
- **Effetto prima del cambio:** il pan 6,02 dB a sinistra più questa differenza danno +4,55 dB.
- **Effetto dopo:** resta la sola differenza, −1,47 dB.
- La pre-registrazione la ignorava: il §1 era corretto per il primo fronte, incompleto per l'intero stem.
- **S4:** a parità di potenza in ingresso, la quota di segnale che passa per il canale a 17 ms, meno distruttivo, cresce. L'energia totale dello stem sale di circa 0,46 dB e la loudness di 0,35 LU. Come pre-registrato, non si compensa senza una nuova pre-registrazione: 0,35 LU di una voce che sta circa 13 LU sotto il bus è una variazione piccola, e resta registrata.

## 3. Lettura

- La domanda di F2, «la Trama è spostata a sinistra?», si risolve sul punto che governa la localizzazione: il primo fronte, prima di ogni eco, è ora centrato (0,00 dB). Nel resto della nota resta uno scarto di circa 1,5 dB **verso destra**, dovuto al delay asimmetrico. Non è una lateralizzazione voluta, ma una conseguenza dei due ritardi.
- **Seguito possibile (F2b), non fatto qui:** rendere simmetrico il contributo del delay. Le opzioni sono ritardi più vicini fra loro, tap scambiati a note alterne o modulazione dei ritardi. È un'altra variabile e richiede la sua pre-registrazione. Il [dossier delle lacune di misura](M2_5_Measurement_Gaps_Research.md) §2 propone di misurarla per banda uditiva (ILD, ITD, IACC), oltre la banda larga usata qui.
- **All'ascolto cumulativo** (D-M2-30) restano due domande: la Trama sembra centrata, e la larghezza data dal delay basta?

## 4. Codice e test

- `voice_allocator.cpp`: pan della Trama centrato (`kTramaCentrePan`).
- `trama_stereo.{h,cpp}` e `cymatica_game --cacp-run trama_stereo_v1`: misure di lateralizzazione; test `[trama_stereo]`, con controlli del pan a 6,02 dB, del centro e del primo fronte contro l'intera finestra.
- `test_voice_allocator.cpp`: il test di continuità L/R per voci stereo usava la Trama come voce asimmetrica. Dopo la centratura usa Corpo (slot 2, L 1,0 / R 0,85), con le stesse asserzioni.
- **Golden PCM:** `7790666370008029299` → **`18256941985354732249`**, con il motivo in `audio_probe.h`.
- **Build e suite:** Release e Debug headless `/WX` 272/272; `full_m2_suite` tutto verde salvo `acao02_v1`, il limite noto.
