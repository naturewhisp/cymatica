# M2.5 — Analisi della produzione inviata dall'esperto. Pre-registrazione

- **Stato:** scritto il 2026-11-20, prima di aprire i dati musicali. Ho letto solo l'intestazione di un MIDI, per sapere se il formato è leggibile. L'hash SHA-256 (LF) è registrato in `progress.md`. Dopo l'hash il file non si modifica.
- **Decisione del titolare:** «Analisi del regalo» (2026-11-20).
- **Materiale:**
  - un brano dell'esperto: 162 BPM, Fa frigio, circa 5 minuti;
  - 29 stem MP3, compreso il mix, e 5 MIDI (808 Bass, Arp, Kick & Snare, Strings, SubBass);
  - inviato «a solo scopo di analisi e raccolta dati», insieme a una nota di produzione.
  - Il materiale resta fuori dal repository, nella cartella di download del titolare. Nel repository vanno solo gli script e le statistiche aggregate: nessuna nota, nessun campione e nessuna sequenza completa.
- **Perché:** l'ascolto cumulativo del 2026-11-19 chiede forma, basso, kick e transizioni (ADR-0007 §8.1). Il brano dell'esperto è un esempio concreto di come lui stesso risolve questi punti. Non è un modello da copiare: lui stesso lo definisce amatoriale, e il suo genere (jungle e trap aggressive) non è quello di CYMATICA. Se ne prendono misure e principi.

## 1. Domande e previsioni

### MIDI, con un parser minimale senza dipendenze

| ID | Domanda | Previsione |
|---|---|---|
| D1 | Backbeat del rullante | Nelle battute in cui suona il rullante, almeno il 75 % ha un colpo sui tempi 2 e 4 |
| D2 | Kick sul tempo 1 | Almeno il 75 % delle battute con kick ha un colpo sul tempo 1 |
| D3 | Ghost notes | Almeno il 10 % dei colpi di rullante ha una velocity ≤ 60 % della massima del rullante |
| D4 | Basso nella scala | Almeno il 90 % delle note del basso 808 sta in Fa frigio (Fa, Sol♭, La♭, Si♭, Do, Re♭, Mi♭); la classe d'altezza più frequente è Fa |
| D5 | Arpeggio | Almeno il 90 % degli intervalli fra attacchi dell'arpeggio vale un sedicesimo |
| D6 | Periodo del basso | Il pattern del basso si ripete con un periodo di 1, 2 o 4 battute in più della metà delle finestre; descrittivo, senza soglia d'esito |

### Stem audio, solo se il titolare approva la decodifica MP3

| ID | Domanda | Previsione |
|---|---|---|
| S1 | Frequenza dei cambiamenti | Un «cambiamento» è l'entrata o l'uscita di uno stem non di gruppo, con l'energia per battuta sopra o sotto −40 dB rispetto al suo massimo. L'intervallo mediano fra cambiamenti successivi del brano è ≤ 8 battute |
| S2 | Sidechain | Nello stem del basso 808, il livello nei primi 30 ms dopo ogni attacco del kick è più basso di almeno 6 dB rispetto a quello fra 120 e 150 ms, in mediana |
| S3 | Loudness | La loudness integrata del mix (BS.1770, implementata con scipy) è fra −8,5 e −5,5 LUFS. L'esperto dichiara circa −7 |
| S4 | Tagli delle basse | Negli stem non di basso (archi, pad, arpeggio, piatti, shaker), la quota d'energia sotto 100 Hz è ≤ 5 % in mediana |

Prima della misura stabilisco quali stem sono «di gruppo»: `Drums`, `Percs`, `Strings Bus`, il mix completo e i ritorni `A-Reverb`. Restano esclusi da S1, perché sommano altri stem.

## 2. Confronto con CYMATICA

Descrittivo, sul brano di riferimento (seed 1337) e sul brano del seed 34:
- intervallo mediano fra cambiamenti, con la stessa definizione di S1 applicata agli stem dei quattro ruoli;
- quota di battute con backbeat, periodo del basso;
- loudness integrata.

Il confronto serve a dare numeri a «ogni 8 battute la canzone dovrebbe cambiare» e a «manca un basso». Non fissa obiettivi: eventuali obiettivi per CYMATICA richiederanno una loro pre-registrazione.

## 3. Limiti

- È un solo brano, di un genere diverso, mixato per un ascolto da club (circa −7 LUFS) e non per un gioco.
- Gli MP3 hanno perdite: le misure sotto 100 Hz e sopra 16 kHz sono approssimate.
- Gli stem di gruppo e i ritorni d'effetto si sovrappongono agli stem singoli: le somme non ricostruiscono il mix.
