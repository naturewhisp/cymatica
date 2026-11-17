# Pacchetto di misura 6 — Misure dello sviluppo validate su verità dichiarate: pre-registrazione

- **Stato:** scritto il 2026-11-17, prima del codice e di ogni misura. L'hash SHA-256 (LF) è registrato in `progress.md`. Dopo l'hash il file non si modifica.
- **Decisione di riferimento:** ADR-0007 D-M2-38 (E1), approvata dal titolare il 2026-11-17: prima della variazione derivata servono misure dello sviluppo validate.
- **Lacuna:** la M4 (Levenshtein categorica sugli intervalli) ha superato le fixture sintetiche F1–F4, ma non ha visto lo sviluppo in F8 (Bach) e in F9 ([risultati M2.5.1c](M2_5_1c_Ensemble_Tools_Results.md) §6): è categorica, e due frasi reali differiscono quasi sempre in quasi tutti gli intervalli. Il [dossier delle lacune](M2_5_Measurement_Gaps_Research.md) §6 propone distanze graduate (D2) e autosimilarità con novità di Foote (D1).

## 1. Il problema della verità

Una misura dello sviluppo si valida solo contro una verità su quali frasi derivino da quali. F8 (Bach) non ha annotazioni. **F9 sì:** la [nota d'intento](experiments/m25_ensemble_tools/controls/F9_subagent_intent.md) del subagente dichiara le derivazioni e le sezioni. La uso come annotazione, letta in modo meccanico e fissata qui prima di misurare:

- **Derivazioni non invertite** (numerazione da 1): 3 ← 1 (trasposta una quarta sopra), 5 ← 1 (abbellita), 13 ← 1 (aumentazione), 15 ← 1 (un'ottava sopra), 21 ← 1 (identica), 22 ← 2 (parte come la 2).
- **Inversioni**, solo descrittive: 6 ← 1, 14 ← 6. Una distanza sugli intervalli con segno le vede lontane per costruzione.
- **Inizi di sezione:** frasi 7, 11, 15, 19, 21.

Limite: F9 è scritta da un modello, non da una persona, e l'annotazione è la sua intenzione dichiarata. F7 (umano) resta atteso e, quando arriva, si aggiunge. F8 resta **descrittivo**: senza annotazioni «superare F8», come chiedeva il dossier, non è definito, e lo si dichiara come deviazione.

## 2. Misure candidate

Sulle note di un ruolo per frase, in ordine d'attacco: successione degli intervalli in semitoni e degli intervalli d'attacco (IOI, in sedicesimi).

- **M5, graduata sugli intervalli:** Levenshtein con costo di sostituzione min(|Δintervallo|, 12) / 12 e costo 1 per inserimento e cancellazione, normalizzata per la lunghezza maggiore. Due frasi vuote danno 0, una vuota e una no danno 1.
- **M5r, graduata con il ritmo:** come M5, con costo di sostituzione 0,5 · min(|Δintervallo|, 12) / 12 + 0,5 · min(|log2(IOI₁ / IOI₂)|, 2) / 2.

Strumenti comuni a entrambe:
- variogramma e nullo per permutazione della M4 (1000 permutazioni, stesso seed, stesse classi);
- **novità di Foote** sulla matrice di autosimilarità S = 1 − d, con kernel a scacchiera di semiampiezza 2 frasi; i picchi sono i massimi locali.

## 3. Prove di validazione e regola d'ammissione

| ID | Prova | Superata se |
|---|---|---|
| **V1, cavallo** | F1–F6 sintetiche (fixture avverse di M2.5.1c) | F1 `DEGENERATE`; F3 `ORDERED` con ρ > 0; F2 e F4 `ORDER_FREE` |
| **V2, lignaggio (F9)** | Per ogni derivazione del §1, rango del genitore dichiarato fra tutte le frasi precedenti, ordinate per distanza crescente | Genitore al rango 1 in almeno **4 su 6** e rango reciproco medio ≥ **0,6** |
| **V3, sezioni (F9)** | I 5 picchi di novità più alti contro gli inizi di sezione dichiarati | Almeno **3 inizi su 5** con un picco entro ± 1 frase |
| **V4, sezioni (fixture del gioco)** | Corpo della fixture (seed 1337, 24 frasi, sezioni A, B, A′ che iniziano alle frasi 9 e 17) | I due picchi di novità più alti alle frasi 9 e 17, ± 1 |
| **V5, invarianza** | Ripetizione | Identica |

**Ammissione:** una misura entra nell'uso di M2.5.3–4 solo se supera V1–V5. Una misura non ammessa non si usa per diagnosticare lo sviluppo, né per scegliere varianti del planner. **Nessuna misura entra nel punteggio del planner** (Sturm 2016; Yang e Lerch 2020).

## 4. Predizioni

| ID | Predizione |
|---|---|
| **Q1** | M5 e M5r superano V1 |
| **Q2** | M5r supera V2: l'aumentazione (13 ← 1) conserva gli intervalli, e la trasposizione per gradi cambia gli intervalli di al più un semitono |
| **Q3** | M5 supera V2 |
| **Q4** | Almeno una delle due supera V3 |
| **Q5** | Entrambe superano V4: Corpo ripete lo stesso materiale dentro ogni sezione |
| **Q6** | La M4, rimisurata con la stessa procedura come riferimento, **non** supera V2 |

Senza predizione: le distanze delle inversioni 6 ← 1 e 14 ← 6; F8 (variogramma e novità, descrittivi).

## 5. Limiti

- Una sola verità annotata, F9, scritta da un modello: è una validazione debole, dichiarata come tale. Diventa più solida con F7.
- Le misure guardano gli intervalli e il ritmo di un ruolo per volta: non vedono armonia, timbro o rapporti fra ruoli.
- L'esperienza di «ricordi, sviluppi, sorprenda e ritorni» resta all'ascolto cumulativo (D-M2-30).
