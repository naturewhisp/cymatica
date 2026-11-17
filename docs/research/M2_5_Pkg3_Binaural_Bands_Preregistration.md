# Pacchetto di misura 3 — ILD, ITD e IACC per banda uditiva: pre-registrazione

- **Stato:** scritto il 2026-11-17, prima del codice e di ogni misura. L'hash SHA-256 (LF) è registrato in `progress.md`. Dopo l'hash il file non si modifica.
- **Autorizzazione:** pacchetto 3 (S-B1, §2) del [dossier delle lacune di misura](M2_5_Measurement_Gaps_Research.md), approvato il 2026-11-17, con la regola di validazione comune.
- **Lacuna:** F2 ha misurato la lateralizzazione con l'ILD a banda larga ([risultati](M2_5_2_F2_Trama_Stereo_Results.md)). Il dossier chiede ILD, ITD e IACC per banda, separati fra primo fronte e nota intera, perché il Side/Mid e la banda larga possono contraddire il percepito.

## 1. Strumento (offline, `engine/cacp/`, nessuna dipendenza)

- **Banco di filtri:** gammatone del 4° ordine realizzato come traslazione in banda base più quattro poli complessi in cascata (Holdsworth), **24 bande** con spaziatura ERB fra **100 Hz e 8 kHz**, larghezza 1,019 · ERB(fc), guadagno unitario a fc.
- **Per banda e finestra:**
  - ILD = 10·log10(E_L / E_R);
  - correlazione incrociata normalizzata fra L e R per ritardi entro **± 1 ms** (± 48 campioni);
  - ITD = ritardo del massimo, IACC = valore del massimo.
- **Finestre**, dagli attacchi dell'event log:
  - **primo fronte**, i primi 5 ms dopo ogni attacco (prima del tap a 11 ms);
  - **nota**, i primi 100 ms dopo ogni attacco.

  Le energie e le correlazioni si sommano su tutte le note.
- **Sintesi a banda larga:** ILD medio pesato per l'energia di banda (la pesatura con la loudness specifica del pacchetto 4 verrà dopo); ITD mediano delle bande sotto 1,5 kHz; IACC medio pesato per l'energia.

## 2. Validazione (segnali sintetici, rumore bianco deterministico, 2 s)

| ID | Prova | Esito atteso |
|---|---|---|
| **C1** | Pan d'ampiezza −6, −3, 0, +3, +6 dB | ILD a banda larga uguale al pan ± 0,2 dB, monotono |
| **C2** | Ritardo interaurale ± 0,3 e ± 0,6 ms (± 14 e ± 29 campioni) | ITD sotto 1,5 kHz uguale al ritardo ± 1 campione, con il segno giusto |
| **C3** | Rumore centrato ma decorrelato (L e R indipendenti) | ILD a banda larga entro ± 0,5 dB; IACC ≤ 0,3 |
| **C4** | Precedenza: burst di 3 ms con R 6 dB sopra L, poi a +8 ms una copia con L 10 dB sopra R | ILD del primo fronte negativo (destra); ILD della nota positivo (sinistra) |
| **C5** | Oracolo: gammatone di scipy (`scipy.signal.gammatone`, IIR) con ILD e correlazione in numpy, sugli stessi segnali di C1–C3 esportati in WAV, nell'ambiente fissato `tools/pyenv` | ILD a banda larga entro ± 0,3 dB e ITD uguale ± 1 campione rispetto a CYMATICA |
| **C6** | Invarianza | Due esecuzioni identiche |

C5 è un oracolo debole: su pan e ritardi uniformi in frequenza qualunque banco di filtri dà gli stessi ILD e ITD. Lo si dichiara.

## 3. Predizioni sullo stem della Trama (fixture, golden PCM `18256941985354732249`)

| ID | Predizione |
|---|---|
| **T1** | Primo fronte: ILD a banda larga entro ± 0,2 dB, ITD 0, IACC ≥ 0,99. Pan centrato, e nessun tap prima di 11 ms |
| **T2** | Nota: ILD a banda larga −1,5 ± 1,0 dB (F2: −1,47 dB sull'intero stem); IACC < 0,95, per la decorrelazione data dal delay asimmetrico |
| **T3** | Nota: l'ILD varia fra le bande, perché il filtraggio a pettine di 11 e 17 ms dipende dalla frequenza. La deviazione standard dell'ILD sulle bande sopra il 10 % dell'energia è ≥ 1 dB |

## 4. Limiti

- L'ILD e l'ITD per banda descrivono i segnali alle due orecchie in cuffia. Non sono un modello di localizzazione completo come quelli di AMT, che restano un oracolo facoltativo.
- La pesatura per energia non è la loudness.
- Esito `MEASURED`, mai `PASS`.
