# M2.5 CP-3 — I4, forma: indagine. Pre-registrazione

- **Stato:** scritto il 2026-11-23, prima del codice e delle misure. L'hash SHA-256 (LF) è registrato in `progress.md`. Dopo l'hash il file non si modifica.
- **Decisione:** D-M2-51, [piano di CP-3](../M2_5_CP3_Expert_Plan.md), I4; il titolare: «Commit e I4».
- **Giudizi da avvalorare:** ripetitività 6/7, calo «al terzo o quarto loop» e «una figura che si ripete» (esperto); «ogni 8 battute la canzone dovrebbe cambiare» (ascoltatore A).
- **Già noto:** 50 % delle finestre di 8 battute ha un'entrata o uscita (riferimento e seed 34), contro l'81 % del brano dell'esperto. Nel corpus di Eigenfeldt le parti cambiano ai confini di frase, e dentro la frase variano solo riempimenti e colpi isolati. Fonti e contro-evidenza (Szpunar: sazietà solo nell'ascolto attento) nel [dossier](M2_5_CP3_Kick_Bass_Form_Sources.md).
- **Domanda:** la ripetizione sta **dentro** la frase, **fra** le frasi o negli **strati**? Da questo dipendono le leve.

## 1. Dati

- Piani della fixture di riferimento (seed 1337), del seed 34 e dei 64 seed procedurali (1–64), esportati in token da uno scenario (`form_investigation_v1`). Sedicesimi, 4/4, 96 battute.
- MIDI di `Kick & Snare` e `808_Bass` dell'esperto: solo statistiche aggregate.
- **Ritmo** di una battuta: insieme dei sedicesimi d'attacco. **Contenuto:** ritmo più altezze.

## 2. Misure e previsioni

| ID | Misura | Previsione |
|---|---|---|
| G1 | Per ruolo, quota di battute 2–4 di una frase con lo **stesso ritmo** della precedente (battute in cui il ruolo suona in entrambe), riferimento | Pulso, Trama e Vettore ≥ 95 % |
| G2 | Frasi in cui la battuta 4 ha un ritmo diverso da tutte e tre le precedenti (riempimento), per ruolo, riferimento | ≤ 5 % per tutti |
| G3 | Frasi consecutive della stessa sezione in cui il ritmo del ruolo cambia, riferimento | Pulso ≥ 50 % |
| G4 | Esperto, cassa e rullante: battute con ritmo uguale alla precedente | ≤ 50 % |
| G5 | Esperto: battute 4, 8, … (ultima di ogni 4) diverse dalla precedente, meno la stessa quota sulle altre | ≥ +10 punti: i cambi si addensano a fine frase |
| G6 | Finestre di 8 battute con un'entrata o uscita di ruolo (simbolica: il ruolo ha o non ha note in una battuta), mediana sui 64 seed | 40–60 % |
| G7 | Esperto, istanti di calo nel seed 34 (0:15, 1:32, 2:00, 2:20, 2:38, 2:46): battute trascorse dall'ultima entrata o uscita di ruolo, mediana | ≥ 4 (descrittivo: sei istanti, un ascoltatore) |
| G8 | Quante entrate o uscite in più servono, al minimo, perché 9 finestre su 12 abbiano un cambiamento nel riferimento | 3 (aritmetica: 6 finestre già coperte) |

## 3. Lettura stabilita prima

- **G1 e G2 vere:** il «loop» è la battuta dentro la frase, e le prime leve sono riempimenti e variazioni di battuta 4.
- **G3 falsa:** la ripetizione sta fra le frasi, e la leva è la varietà dei pattern.
- **G4 e G5 vere:** il riempimento a fine frase è un principio anche del brano dell'esperto. Se false, il paragone non sostiene quella leva.
- **G6 sotto 40 %:** la leva sono entrate e uscite più frequenti.
- Le leve che ne risultano restano **proposte**: ognuna avrà una pre-registrazione, il divieto di varietà fittizia e un confronto.

## 4. Limiti

- Simbolico: dinamica, timbro, dissolvenze e loudness non contano, e un cambio minimo conta quanto uno grande.
- Un brano dell'esperto, sei istanti di calo.
- «Stesso ritmo» non è «si sente uguale».
