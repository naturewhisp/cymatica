# Pacchetto di misura 4b — Pendenza confrontabile, sensibilità della Trama e sezione A: risultati

- **Pre-registrazione:** [M2_5_Pkg4b_Slope_And_Trama_Sensitivity_Preregistration.md](M2_5_Pkg4b_Slope_And_Trama_Sensitivity_Preregistration.md), SHA-256 (LF) `3a59681f8615e870b232331a16e2dc841733e87257cdbf43ae26d1c1f40d9546`.
- **Esecuzione, 2026-11-17:** Release `stem_export_v2` (golden PCM PASS a offset 0), poi `slope_bands.py` e `trama_sensitivity.py` nell'ambiente fissato. Dati: [esperimento](experiments/m25_pkg4b_slope_trama/README.md).
- **Esito:** tutte le prove e le predizioni confermate (G1–G3, K1–K3). **L'affermazione del pacchetto 4 «mix più scuro di Pestana» è ritirata e contraddetta** dalla misura confrontabile.

## 1. Pendenza

| ID | Atteso | Risultato | Esito |
|---|---|---|---|
| G1 | Bianco 0 ± 0,3; rosa −3 ± 0,3 dB/ottava | +0,002; −3,000 | Confermata |
| G2 | Curva: −5,93 ± 0,3; metodo vecchio scostato ≥ 1 | **−5,91**; metodo vecchio **−10,36** | Confermata: la distorsione del metodo vecchio è dimostrata |
| G3 | Mix intero, 99–4000 Hz, scostato ≥ 1 dal metodo vecchio | **−5,42** contro −9,16 | Confermata |

Mix intero, metodo nuovo: −2,75 dB/ottava fra 99 e 500 Hz, −5,48 fra 500 e 2000 Hz, **−12,40** fra 2000 e 4000 Hz. Gli estratti stanno fra −4,77 e −5,41.

**Lettura:**
- Nella gamma di Pestana il mix scende di **−5,4 dB/ottava**, in linea con il valore di circa −5 dei singoli commerciali. Il mix **non** è più scuro in questa gamma. Il −8,7 del pacchetto 4 era una distorsione del metodo: la maggior parte dei punti sopra i 4 kHz pesava la caduta degli acuti.
- Dove una differenza c'è, è **sopra i 2 kHz**: −12,4 dB/ottava fra 2 e 4 kHz. È la zona in cui i denti di sega filtrati di Corpo e Vettore e la Trama cadono rapidamente (fra −11 e −16 dB/ottava). Se il mix suoni «spento» negli acuti è un'ipotesi per l'ascolto cumulativo. Un confronto con riferimenti sopra i 4 kHz richiede una fonte che lo misuri in quella gamma: Pestana non lo fa.
- Le pendenze per ruolo nella gamma bassa (per esempio la Trama a +15 dB/ottava fra 99 e 500 Hz) descrivono stem con poca energia sotto la fondamentale. Non sono giudizi.

## 2. Sensibilità della Trama al livello

| Sezione | Senza Trama (sone) | Incremento a 0 / +3 / +6 / +9 dB | Perdita a 0 / +9 dB | ΔN per dB |
|---|---:|---|---|---:|
| A (frasi 5–6) | 35,10 | 0,75 / 1,27 / 2,05 / 3,15 | 0,865 → 0,681 | 0,27 |
| B | 33,26 | 0,46 / 0,81 / 1,38 / 2,25 | 0,916 → 0,767 | 0,20 |
| A′ | 33,33 | 0,84 / 1,42 / 2,26 / 3,43 | 0,848 → 0,653 | 0,29 |

- **K1:** il mix senza Trama è identico al bit a tutti gli offset. Confermata.
- **K2, K3:** incremento strettamente crescente e perdita strettamente decrescente in ogni sezione. Confermate.
- **Sezione A** misurata dove la Trama suona: perdita 0,865, coerente con B (0,916) e A′ (0,848).

**Lettura:**
- L'incremento della Trama cresce più che proporzionalmente con il livello: ×2,7 a +6 dB, ×4,2 a +9 dB, perché alzarla la libera progressivamente dal mascheramento. Il mix nel suo insieme cresce di poco: a +9 dB l'incremento vale circa il 9 % della loudness del mix senza Trama.
- La misura dice quanto si guadagna in udibilità per ogni dB, non a quale livello la Trama sia «giusta». Quella è una scelta di bilanciamento: si può fare con un criterio pre-registrato (D-M2-30), oppure portare all'ascolto cumulativo un intervallo di livelli già misurati.

## 3. Codice

- Override offline del bus della Trama: `AudioEngine::setTramaBusGainScale`, `RoleBusProcessor::setTramaGainScale`, `ProbeAnalysisCapture::tramaBusGainScale`. Lo stato golden passa a `NOT_APPLICABLE` con l'override attivo. Test `[trama_gain_override]`.
- Scenario `stem_export_v2`.
- Release e Debug headless `/WX` 292/292.
