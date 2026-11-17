# Pacchetto di misura 5 — Descrittori timbrici e separabilità fra ruoli: pre-registrazione

- **Stato:** scritto il 2026-11-17, prima del codice e di ogni misura. L'hash SHA-256 (LF) è registrato in `progress.md`. Dopo l'hash il file non si modifica.
- **Autorizzazione:** pacchetto 5 (T1, §4) del [dossier delle lacune di misura](M2_5_Measurement_Gaps_Research.md), approvato il 2026-11-17, con la regola di validazione comune.
- **Domanda aperta:** con il rapporto FM 1:1 lo spettro della Trama è armonico come quello di Corpo. La Trama resta distinguibile? D-M2-33 l'ha rinviata all'ascolto cumulativo; questo pacchetto dà una misura che la prepari, non che la sostituisca.

## 1. Strumento (offline, `engine/cacp/`, nessuna dipendenza nel codice C++)

- **Note:** note isolate dal synth reale, attraverso il motore offline, di Corpo, Trama e Vettore alle stesse sei altezze (220, 262, 294, 330, 392, 440 Hz). Durata 300 ms, velocity 0,8, rilascio di produzione. Ogni nota si analizza in mono, dall'attacco fino a 50 ms dopo il NoteOff.
- **Descrittori** (definizioni di Peeters et al. 2011, implementati da noi sullo STFT di `descriptors_v1`):
  1. **centroide armonico**: centroide spettrale medio pesato per l'energia, diviso per f0;
  2. **dispersione spettrale**, divisa per f0;
  3. **rapporto dispari/pari**: energia delle armoniche dispari su quella delle pari, dalle prime 10 armoniche, in dB;
  4. **tempo d'attacco**: dal 10 % al 90 % del massimo dell'inviluppo RMS a 1 ms;
  5. **pendenza del centroide**: regressione del centroide armonico sui primi 200 ms, per secondo;
  6. **flusso spettrale medio**.
- **Distanza fra ruoli:** distanza euclidea fra le medie dei ruoli, nello spazio dei descrittori standardizzati (z-score su tutte le note).
- **Separabilità:** accuratezza di un classificatore nearest-centroid a esclusione di uno (leave-one-out) fra le 12 note di due ruoli.

## 2. Validazione

| ID | Prova | Esito atteso |
|---|---|---|
| **D1, negativo** | Note di Corpo divise in due gruppi per indice d'altezza (alterne) | Accuratezza ≤ 0,67 (il caso è 0,5) |
| **D2, positivo** | Sinusoide contro dente di sega sintetici, stesse altezze e stesso inviluppo | Accuratezza 1,0 |
| **D3, positivo** | FM 1,88 (indice 3) contro dente di sega sintetici | Accuratezza 1,0 |
| **D4, oracolo** | Centroide e dispersione spettrale sui WAV delle note, con un'implementazione numpy indipendente (STFT e definizioni scritte di nuovo in Python, nell'ambiente fissato) | Entro ± 2 % per nota |
| **D5, invarianza** | Due esecuzioni | Identiche |

D4 usa un'implementazione numpy indipendente invece di librosa. librosa trascina numba e llvmlite, una dipendenza pesante per due descrittori. Se il titolare preferisce librosa come oracolo, lo si aggiunge con una nuova pre-registrazione.

## 3. Predizioni

| ID | Predizione |
|---|---|
| **P1** | Trama contro Corpo: accuratezza ≥ 0,9. Inviluppo e decadimento dell'indice FM danno una pendenza del centroide diversa dal filtro di Corpo, anche a spettro armonico |
| **P2** | Trama contro Vettore: accuratezza ≥ 0,9 |
| **P3** | Rapporto dispari/pari della Trama entro ± 3 dB da quello di Corpo, perché con 1:1 entrambi hanno tutte le armoniche: è il punto in cui la scelta di D-M2-33 li avvicina |

## 4. Limiti

- La separabilità nello spazio dei descrittori non prova la distinguibilità percepita: è un'indicazione per l'ascolto cumulativo, che resta il giudice (D-M2-30).
- 6 altezze per ruolo: le accuratezze hanno una granularità di 1/12.
- Pulso è escluso: è un kick, e il confronto riguarda i tre ruoli tonali.
