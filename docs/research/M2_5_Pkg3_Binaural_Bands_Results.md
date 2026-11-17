# Pacchetto di misura 3 — ILD, ITD e IACC per banda: risultati

- **Pre-registrazione:** [M2_5_Pkg3_Binaural_Bands_Preregistration.md](M2_5_Pkg3_Binaural_Bands_Preregistration.md), SHA-256 (LF) `cc028388ea8a44e1f3f457dd07029905a78fcbe338f6111c40fd32708cd8d2bf`.
- **Esecuzione, 2026-11-17:** Release, scenario `binaural_v1` (golden PCM PASS); oracolo scipy 1.15.3 in `tools/pyenv`. Dati: [esperimento](experiments/m25_pkg3_binaural/README.md).
- **Esito:** strumento **validato** (C1–C6). Le misure sulla Trama confermano T1–T3.

## 1. Validazione

| ID | Atteso | Risultato | Esito |
|---|---|---|---|
| C1 | ILD = pan ± 0,2 dB, monotono | −6,000 / −3,000 / 0,000 / +3,000 / +6,000 dB | Confermata |
| C2 | ITD = ritardo ± 1 campione | −29 / −14 / +14 / +29, esatti | Confermata |
| C3 | Rumore decorrelato: ILD ± 0,5 dB, IACC ≤ 0,3 | ILD +0,024 dB, IACC 0,031 | Confermata |
| C4 | Precedenza: primo fronte a destra, nota a sinistra | −6,00 dB e +7,11 dB | Confermata |
| C5 | Oracolo scipy: ILD ± 0,3 dB, ITD ± 1 | Concorda su C1–C3 (scarto massimo 0,002 dB; ITD esatti) | Confermata |
| C6 | Ripetizione identica | Identica | Confermata |

**Due correzioni, entrambe prima di leggere gli esiti T1–T3 e dichiarate:**
1. **Normalizzazione della correlazione:** la prima esecuzione normalizzava con l'energia a ritardo zero. Sulle finestre di 5 ms, all'attacco, l'IACC superava 1 (fino a 1,49) e l'ITD era spurio. Ora ogni ritardo usa le energie dello stesso tratto di campioni, come richiede la definizione di correlazione normalizzata. C1–C4 non cambiano.
2. **Variante dell'oracolo:** la pre-registrazione indicava il gammatone IIR di scipy. Quella progettazione mette poli fuori dal cerchio unitario sotto circa 250 Hz a 48 kHz (|p| fino a 1,0095), in forma diretta come in sezioni del secondo ordine, e restituisce valori non definiti. L'oracolo usa la variante FIR dello stesso gammatone di scipy.

C5 resta un oracolo debole: con pan e ritardi uniformi in frequenza qualunque banco di filtri dà gli stessi valori.

## 2. Trama (fixture)

| ID | Predizione | Risultato | Esito |
|---|---|---|---|
| T1 | Primo fronte: ILD ± 0,2 dB, ITD 0, IACC ≥ 0,99 | ILD −0,001 dB, ITD 0, IACC 1,000; banda per banda ILD entro ± 0,03 dB e IACC ≥ 0,994 | Confermata |
| T2 | Nota: ILD −1,5 ± 1,0 dB; IACC < 0,95 | ILD **−1,33 dB**, IACC **0,888** | Confermata |
| T3 | Dispersione dell'ILD fra le bande ≥ 1 dB | **2,72 dB**: da +1,8 dB a 100 Hz a −3,0 dB a 530 Hz | Confermata |

**Lettura:**
- Il primo fronte della Trama è centrato in ogni banda, non solo in banda larga.
- Durante la nota il delay asimmetrico dà uno spostamento medio verso destra di 1,3 dB, con la direzione che cambia fra le bande, e una correlazione 0,89: è la sorgente della larghezza. L'ITD mediano delle bande sotto 1,5 kHz è −2 campioni, cioè il canale sinistro anticipa di circa 0,04 ms per il tap a 11 ms.
- Se questa combinazione suoni centrata e larga, o diffusa in modo sgradevole, lo dice solo l'ascolto (D-M2-30). Il seguito F2b, delay simmetrico, ora ha una misura per banda con cui verificarsi.

## 3. Codice

`binaural.{h,cpp}`: banco gammatone (traslazione in banda base, 4 poli complessi), ILD/ITD/IACC per banda e finestre, scenario `binaural_v1`. Test `[binaural]`: centri ERB, guadagno unitario a fc e reiezione a due ottave, pan di 6 dB. Release 289/289.
