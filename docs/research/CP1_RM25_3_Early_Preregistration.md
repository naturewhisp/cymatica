# CP-1 — Parte anticipata di R-M25-3: pre-registrazione delle misure

- **Stato:** pre-registrazione, scritta il 2026-11-12 **prima** di scrivere i prototipi e di misurare. Non normativa; nessun prototipo entra nel motore. L'hash è registrato in `docs/progress.md` prima della prima misura.
- **Origine:** ADR-0007 §8.1 (proposte dell'esperto: LFO sul filtro, saturazione, supersaw, a CP-1), [consolidamento §6.3](CYMATICA_Analysis_Consolidation_2026-11-09.md) (denormali, aliasing della saturazione, voci). Il resto di R-M25-3 (riverbero, chorus, delay) resta dopo CP-1.
- **Budget di riferimento:** D-M2-10, p99.9 ≤ 2,0 ms per blocco da 512 frame. Carico peggiore misurato oggi (W): p99.9 0,27–0,43 ms. Margine indicativo ≈ 1,6 ms per 512 frame, cioè ≈ 3,1 µs per frame per tutte le aggiunte insieme.

## Metodo

Prototipi offline in un test `[perf]` (Release, un thread, 48 kHz), fuori dal motore. Tempi: 5 ripetizioni su 10 s di segnale, si riporta la mediana in ns per campione. Aliasing: FFT di 65.536 punti con finestra di Blackman-Harris su un seno a 1.244,5 Hz saturato; potenza nelle bande ±3 bin delle armoniche vere sotto Nyquist contro tutto il resto escluso il DC; rapporto alias/segnale in dB.

| Domanda | Varianti |
|---|---|
| A. Saturazione `tanh(4·x)` | ingenua; ADAA di primo ordine (antiderivata `log cosh`); sovracampionamento 2× e 4× con FIR a semibanda |
| B. Supersaw | N = 1, 3, 5, 7 oscillatori PolyBLEP del motore per nota; costo per 8 voci |
| C. LFO sul cutoff dell'SVF del motore | cutoff costante; cutoff modulato ogni campione; modulato ogni 32 campioni |
| D. Denormali | coda di un SVF dopo un impulso: con `flushSubnormal`, senza, senza ma con FTZ/DAZ |

## Predizioni (senza soglie di qualità)

| ID | Predizione | Motivo |
|---|---|---|
| A1 | Rapporto alias/segnale: ingenua > ADAA1 e ingenua > 2×; 4× < 2× | Letteratura ADAA (Parker et al. 2016; Bilbao et al. 2017) e teoria del sovracampionamento |
| A2 | Costo: ingenua < ADAA1 < 2× < 4× | Una `tanh` per campione contro più `tanh` e filtri |
| B1 | Costo lineare in N; 8 voci × 7 oscillatori entro il margine di ≈ 3,1 µs per frame | PolyBLEP a 2 punti costa poche operazioni |
| C1 | Cutoff modulato ogni campione costa quanto il cutoff costante (entro il 20 %) | L'SVF del motore calcola già `tan` a ogni campione |
| D1 | Senza `flushSubnormal` e senza FTZ/DAZ la coda è più lenta che con `flushSubnormal`; con FTZ/DAZ torna simile | Penalità dei subnormali documentata (Intel oneMKL) |

Ogni predizione mancata si riporta. Il dossier per CP-1 elenca costi e aliasing, non decide: la scelta di integrare LFO, saturazione o supersaw resta del titolare a CP-1, con A/B e ascolto in M2.5.2.
