# Pacchetto di misura 4b — Pendenza confrontabile, sensibilità della Trama e sezione A: pre-registrazione

- **Stato:** scritto il 2026-11-17, prima del codice e di ogni misura nuova. L'hash SHA-256 (LF) è registrato in `progress.md`. Dopo l'hash il file non si modifica.
- **Origine:** il titolare il 2026-11-17 ha chiesto di approfondire le evidenze del pacchetto 4 prima di intervenire («Sì, preparalo»). Perimetro: pacchetti 1–5 approvati.
- **Perché:** tre limiti emersi dopo il pacchetto 4 ([risultati](M2_5_Pkg4_Specific_Loudness_Masking_Results.md)):
  1. la pendenza del mix (−8,7 dB/ottava) è stata calcolata fra 50 Hz e 16 kHz con i bin lineari di Welch. In una regressione su log2(f) la maggior parte dei punti cade sopra i 4 kHz, mentre Pestana et al. 2013 misurano fra 100 e 4000 Hz. Il confronto «più scuro di Pestana» non è quindi valido e **va ritirato**. I controlli L5 non potevano accorgersene: bianco e rosa sono rette;
  2. la perdita per mascheramento della Trama (0,85–0,92) mescola mascheramento e compressione della loudness; per decidere serve la curva in funzione del livello;
  3. in A l'estratto pre-registrato cade nella metà in cui la Trama tace.

## 1. Pendenza confrontabile

- **Metodo:** densità spettrale di potenza (Welch, Hann di 8192 campioni) mediata dentro bande di terzo d'ottava con centri fc = 1000 · 2^(k/3); limiti di banda fc · 2^(±1/6).
  - **Gamma di Pestana:** k = −10…6, cioè 99,2–4000 Hz, 17 bande. Regressione lineare del livello di banda (dB) su log2(fc), **un punto per banda**.
  - Si riportano anche le sottogamme 99–500 Hz, 500–2000 Hz e 2000–4000 Hz, e il metodo vecchio sugli stessi segnali.
- **Rappresentazione:** densità per Hz, la stessa in cui il rosa vale −3 dB/ottava e Pestana circa −5 dB/ottava, come nel dossier R-M25-7.
- **Segnali:** mix e stem del **brano intero** (render della fixture, golden PCM `18256941985354732249`) e i tre estratti del §3.

| ID | Prova | Esito atteso |
|---|---|---|
| **G1** | Rumore bianco e rosa | 0 ± 0,3 e −3 ± 0,3 dB/ottava |
| **G2, curva** | Rumore sintetico con densità −3 dB/ottava sotto 1 kHz e −12 sopra | Pendenza **−5,93 ± 0,3** (valore analitico della regressione sui 17 centri). Il metodo vecchio sullo stesso segnale si discosta di almeno 1 dB/ottava: la distorsione del metodo vecchio è dimostrata, non supposta |
| **G3, predizione** | Mix intero, 99–4000 Hz | Differisce dal −8,7 del metodo vecchio di almeno 1 dB/ottava. Nessuna predizione sul segno rispetto a Pestana |

## 2. Sensibilità della Trama al livello

- **Override offline** del guadagno di bus della Trama, come quello di Pulso in F4a: solo motore offline, rifiutato con un device. Il delay della Trama segue il guadagno, perché sta dopo di esso.
- **Offset:** 0, +3, +6, +9 dB. Per ogni offset e sezione: loudness tempo-variante (MoSQITo 1.2.1, stessa calibrazione e stesso mono del pacchetto 4) della Trama da sola, del mix e del mix senza Trama; **incremento** ΔN e **perdita**.

| ID | Predizione |
|---|---|
| **K1** | Il mix senza Trama è identico a tutti gli offset (hash del PCM dell'estratto): l'override tocca solo la Trama |
| **K2** | ΔN cresce in modo strettamente monotono con l'offset in ogni sezione |
| **K3** | La perdita decresce in modo strettamente monotono con l'offset in ogni sezione |

Senza predizione: ΔN per dB, cioè quanta loudness della Trama si recupera per ogni dB nel mix. È la grandezza che deciderebbe un intervento.

## 3. Estratti

- **B e A′:** gli stessi del pacchetto 4 (frasi 3–4 della sezione).
- **A:** frasi 5–6 della sezione (battute 17–24), dove la Trama suona. Nelle frasi 1–4 di A tace per costruzione (`acao02_v1`, presenza per frase).

Il pacchetto 4 resta com'è. Questi estratti valgono per il 4b.

## 4. Limiti

- Pestana misura singoli commerciali al numero uno, cioè un altro genere e un altro uso: il confronto resta debole anche con il metodo giusto, ed è una descrizione, non un obiettivo.
- ΔN non è il modello di loudness parziale di Glasberg e Moore (pacchetto 4, limiti).
