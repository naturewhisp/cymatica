# M2.5 CP-3 — I3, basso: indagine. Pre-registrazione

- **Stato:** scritto il 2026-11-23, prima delle misure. L'hash SHA-256 (LF) è registrato in `progress.md`. Dopo l'hash il file non si modifica.
- **Decisioni del titolare (2026-11-23):** propende per un ruolo dedicato; d'accordo con un prototipo offline a tre versioni, ma **prima l'indagine pianificata**. L'ADR può seguire le evidenze.
- **Scopo:** dare al prototipo registro, rapporto col kick e separazione misurati. **Non** decide fra quinto ruolo e voce di Corpo: quello è il confronto del prototipo.
- **Già noto** (analisi della produzione dell'esperto, quindi non previsto qui): scala, durata mediana di mezzo battito, nessuna finestra di 4 battute ripetuta, 90–99 % d'energia sotto 100 Hz, ducking misurato di 1,7 dB.

## A. Il basso dell'esperto rispetto al kick

MIDI di `808_Bass`, `SubBass` e `Kick & Snare` (stessa base dei tempi); stem dell'808 per l'altezza reale. Solo statistiche aggregate.

| ID | Misura | Previsione |
|---|---|---|
| A1 | Quota degli attacchi del basso entro un trentaduesimo da un kick, nelle battute in cui suonano entrambi | ≥ 50 % |
| A2 | La stessa quota con il kick spostato di k sedicesimi (k = 1…15, ciclico nella battuta): media dei 15 | A1 la supera di almeno 15 punti: basso e kick sono legati e non indipendenti |
| A3 | Fondamentale nell'audio: mediana pesata per energia del picco fra 25 e 150 Hz, finestre di 200 ms | fra 35 e 70 Hz |
| A4 | Movimenti fra note successive: quota di 0–2 semitoni e quota di salti ≥ 7 | la prima ≥ 40 % |
| A5 | Attacchi del basso sui quarti (sedicesimi 0, 4, 8, 12) | ≤ 60 %: linea in parte sincopata |

## B. Lo spazio libero in CYMATICA

Stem di riferimento (`kick_presence_ab_v1`), con il kick attuale e con `combo36click3`.

| ID | Misura | Previsione |
|---|---|---|
| B1 | Quota d'energia di 30–120 Hz dovuta al kick, per sezione | ≥ 80 % in tutte |
| B2 | Quota del tempo in cui il kick, in 30–160 Hz (finestre di 10 ms), è più di 20 dB sotto il suo massimo | ≥ 40 % in tutte: c'è tempo libero per il basso |

## C. Registro e separazione dal kick

Basso **sintetico di prova**, non una proposta musicale: fondamentale dell'accordo di ogni battuta, ottavi, armoniche 1–3 a 0, −6 e −12 dB, attacco 5 ms, note lunghe l'80 % dell'ottavo con uscita di 10 ms. Livello: loudness K della sezione pari a quella di Corpo meno 3 LU, fissata senza separazione e mantenuta.

- **Registri:** basso (MIDI 28–39) e alto (40–51).
- **Separazioni:** S0 nessuna; S1 alternanza (la nota che coincide col kick entro un trentaduesimo entra 60 ms dopo); S2 ducking −6 dB; S3 ducking −12 dB (inviluppo esponenziale dall'attacco del kick, rientro con τ = 120 ms); S4 passa-alto del basso a 90 Hz, 24 dB/ottava.
- **R** = mediana, sui kick, di energia del kick su energia del basso in 40–160 Hz nei 60 ms dopo l'attacco. **P** = perdita di loudness K del basso rispetto a S0. **L** = energia del basso in 30–120 Hz rispetto a S0.

| ID | Previsione |
|---|---|
| C1 | R è ≥ 6 dB in S0 per tutte le sezioni con almeno un registro: il kick è già molto forte nelle basse, e una separazione leggera può bastare |
| C2 | S3 alza R di 8–12 dB rispetto a S0 e costa più di 2 LU di P |
| C3 | S4 costa meno del ducking in P nel registro alto, ma toglie più di 3 dB a L nel registro basso |
| C4 | S1 alza R di più di 12 dB e costa al massimo 2 LU |

**Regola:** per ogni kick si propone la coppia registro-separazione con R ≥ 6 dB in tutte le sezioni, P ≤ 2 LU e L ≥ −3 dB; fra quelle ammesse, la meno invasiva nell'ordine S0, S2, S1, S3, S4, e a parità il registro basso. Se nessuna è ammessa lo si riporta e il prototipo le prova tutte.

## Limiti

- R è un rapporto d'energia, non un modello di mascheramento; la soglia di 6 dB è ragionata, non validata.
- Un solo brano dell'esperto; il basso di prova è sintetico e con ritmo fisso.
- Gli stem WAV sono a 16 bit e prima del master: nessuna misura del limiter.
