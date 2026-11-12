# CP-1 — Parte anticipata di R-M25-3: risultati

- **Stato:** evidenza per CP-1, non normativa. Nessun prototipo entra nel motore; la scelta di integrare LFO, saturazione o supersaw resta del titolare a CP-1, con A/B e ascolto in M2.5.2.
- **Pre-registrazione:** [CP1_RM25_3_Early_Preregistration.md](CP1_RM25_3_Early_Preregistration.md), SHA-256 (LF) `dc40d45a…`, registrata prima dei prototipi.
- **Esecuzione:** [`test_rm25_3_early.cpp`](../../tests/unit/test_rm25_3_early.cpp), tag `[perf][rm25_3]`, escluso da CI e preset headless; `cymatica_tests "[rm25_3]"` in Release, AMD Ryzen 7 7700X, 2026-11-12. Dati: [results.json](experiments/cp1_rm25_3/results.json); la prima esecuzione, con i due errori sotto, è conservata in [results_first_run_with_errors.json](experiments/cp1_rm25_3/results_first_run_with_errors.json).
- **Budget di riferimento:** margine indicativo ≈ 3,1 µs per frame per tutte le aggiunte (D-M2-10, 2,0 ms per 512 frame, meno il p99.9 di W, 0,27–0,43 ms).

## Due errori della prima esecuzione, corretti e dichiarati

1. **Costo del sovracampionamento sbagliato di 100 volte** per un errore di codice: tempo diviso due volte. Corretto.
2. **Parametro pre-registrato inadatto:** le bande di ±3 bin intorno alle armoniche sono più strette del lobo principale della finestra di Blackman-Harris (±4 bin), quindi i bordi delle armoniche forti finivano contati come aliasing e tutte le varianti risultavano fra −68 e −70 dB. Si riportano entrambe le bande; la lettura usa ±6 bin. È uno scostamento dalla pre-registrazione, non un ritocco della misura.

## Risultati

### A. Saturazione `tanh(4·x)` su un seno a 1.244,5 Hz

| Variante | Alias/segnale, ±3 bin (pre-registrato) | Alias/segnale, ±6 bin | ns per campione |
|---|---:|---:|---:|
| Ingenua | −68,0 dB | −71,7 dB | 11,0 |
| ADAA di primo ordine | −69,6 dB | −76,8 dB | 29,5 |
| Sovracampionamento 2× | −70,5 dB | −93,4 dB | 184 |
| Sovracampionamento 4× | −70,5 dB | −93,4 dB | 669 |

- **A1, ordine dell'aliasing:** ingenua > ADAA1 > 2×, come previsto. **4× e 2× coincidono a −93,4 dB**: è con ogni probabilità il pavimento della misura (finestra e FIR), quindi «4× sotto 2×» **non è verificabile** con questo metodo.
- **A2, ordine dei costi:** ingenua < ADAA1 < 2× < 4×, come previsto. ADAA1 è in doppia precisione con `log` ed `exp`; il sovracampionamento usa un FIR diretto da 127 o 255 prese, un **limite superiore**: un filtro polifase o IIR a semibanda costerebbe molto meno.
- **Lettura:** con una saturazione per ruolo (8 canali), ADAA1 costa ≈ 0,24 µs per frame, il 2× diretto ≈ 1,5 µs, cioè metà del margine. L'ADAA riduce l'aliasing di circa 5 dB su questo segnale, il 2× di circa 22 dB.

### B. Supersaw con l'oscillatore PolyBLEP del motore, 8 voci

| Oscillatori per nota | 1 | 3 | 5 | 7 |
|---|---:|---:|---:|---:|
| ns per frame (8 voci) | 20,0 | 62,0 | 103,0 | 148,7 |

- **B1 confermata:** costo lineare (≈ 21 ns per oscillatore per frame, su 8 voci) e 7 oscillatori su 8 voci costano 0,15 µs per frame, circa il 5 % del margine.
- **Voci (consolidamento §6.3):** la supersaw sta dentro una voce, quindi non consuma voci del budget D-M2-03; il vincolo resta che le note di Vettore si sovrappongano al più in due.
- **Determinismo:** nella JP-8000 le fasi dei sette oscillatori sono casuali a ogni nota (Szabo 2010, sotto). In CYMATICA andrebbero tratte da uno stream deterministico isolato, altrimenti si perdono golden e replay.

### C. LFO sul cutoff dell'SVF del motore

| Variante | ns per campione |
|---|---:|
| Cutoff costante | 6,3 |
| LFO ricalcolato ogni 32 campioni | 9,9 |
| LFO ricalcolato ogni campione | 17,8 |

- **C1 mancata:** il cutoff modulato ogni campione costa 2,8 volte il cutoff costante, non entro il 20 %. Causa probabile, **non verificata**: con un cutoff costante il compilatore può calcolare `tan` una volta sola fuori dal ciclo, e la variante modulata aggiunge un `sin` per campione per l'LFO. Il costo assoluto resta piccolo: 18 ns per campione per filtro.

### D. Coda di un filtro passa-basso dopo un impulso (50 Hz, Q 0,7)

| Variante | ns per campione |
|---|---:|
| SVF del motore, con `flushSubnormal` | 2,1 |
| Senza azzeramento | 16,9 |
| Senza azzeramento, con FTZ/DAZ | 7,7 |

- Senza azzeramento, il 96 % dei campioni della coda (462.493 su 480.000) ha stati subnormali.
- **D1 confermata in parte:** senza azzeramento la coda è 8 volte più lenta. Con FTZ/DAZ migliora (7,7 ns) ma **non torna simile** all'azzeramento del motore (2,1 ns), contro la predizione. Il motore azzera già nei filtri; strutture nuove con retroazione (riverbero, delay) andranno scritte con lo stesso azzeramento, oppure misurate con FTZ/DAZ per thread.

## Fonti

- J. D. Parker, V. Zavalishin, E. Le Bivic, «Reducing the aliasing of nonlinear waveshaping using continuous-time convolution», DAFx-16, Brno, 2016 ([archivio DAFx](https://dafx.de/paper-archive/details/vem_XXF5qBbfiWOH2RVVAA)). Letto il record, non il testo integrale.
- S. Bilbao, F. Esqueda, J. D. Parker, V. Välimäki, «Antiderivative antialiasing for memoryless nonlinearities», IEEE Signal Processing Letters 24(7), 1049–1053, 2017, DOI 10.1109/LSP.2017.2675541 ([dblp](https://dblp.org/rec/journals/spl/BilbaoEPV17)). Letto il record.
- V. Välimäki, A. Huovilainen, «Antialiasing oscillators in subtractive synthesis», IEEE Signal Processing Magazine 24(2), 116–125, 2007, DOI 10.1109/MSP.2007.323276. Letto il record.
- A. Szabo, *How to Emulate the Super Saw*, tesi di laurea, KTH Stoccolma, 2010 (TRITA-CSC-E 2010:131). **Letta** nel testo integrale (PDF fornito dal titolare, 36 pagine, SHA-256 `a354c676…`, non versionato per i diritti): sette oscillatori a dente di sega; curva di detune non lineare approssimata con un polinomio di 11° ordine; mix con il centrale che cala linearmente e i sei laterali che crescono a parabola; passa-alto che segue la fondamentale contro l'aliasing; fasi casuali a ogni nota.
- Intel, *oneMKL Developer Guide*, «Operating on Denormals» ([pagina](https://www.intel.com/content/www/us/en/docs/onemkl/developer-guide-windows/2024-0/operating-on-denormals.html)): FTZ e DAZ nel registro MXCSR evitano il rallentamento dei subnormali, gestiti in software dalla CPU.

## Limiti

- Una macchina, un thread, prototipi semplici; i costi sono ordini di grandezza, non budget.
- L'aliasing è misurato su un seno, non sul materiale del gioco, e con un pavimento di misura intorno a −93 dB.
- Nessun ascolto: non dice se le differenze di aliasing si sentono nel mix.
