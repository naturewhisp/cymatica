# M2.5.1c — Strumento offline d'insieme e di sviluppo: pre-registrazione

- **Stato:** pre-registrazione, scritta il 2026-11-12 **prima** di implementare ed eseguire lo strumento. Non normativa: non modifica specifica, ADR, contratti o golden.
- **Origine:** [dossier R-M25-7](M2_5_F5_Music_Director_Research.md), §«Sviluppo o varietà fittizia» e tabella delle fasi (M2.5.1: «fixture avverse, nullo per permutazione, variogramma; statistiche verticali su griglia intera; stima del costo logico dei progetti B, D e G, senza integrarli»).
- **Regola del dossier:** una misura che non ordina correttamente le fixture avverse è un «cavallo» ([Sturm 2016](https://arxiv.org/abs/1606.03044)) e non si usa. Ipotesi fissate prima; **nessuna soglia di qualità** è proposta. Le misure non entrano nello scoring del planner.
- **Integrità:** l'hash SHA-256 di questo file è registrato in `docs/progress.md` prima della prima esecuzione. I risultati vanno in un documento separato; questo file non si modifica dopo.

## 1. Misure

**Token.** Per ogni frase e ruolo, le note del piano su una griglia di sedicesimi della frase: onset relativo all'inizio della frase, nota MIDI arrotondata, classe di durata 1..16 sedicesimi (come `planTokens` di `diversity`, ma per frase).

**Distanza fra frasi.** d_r(i, j) = distanza di Levenshtein normalizzata fra le sequenze del ruolo r, con elementi (Δonset, intervallo, classe di durata): è la M4 già esistente (`m4EditDistance`). Proprietà nota prima di misurare: è **invariante per trasposizione** (usa intervalli) e ignora la posizione assoluta della prima nota nella frase.

**Variogramma.** V_r(k) = media di d_r(i, i + k) su tutte le coppie a distanza k, per k = 1..P−1, con il numero di coppie. Le analisi usano k ≤ P/2; il resto è descrittivo.

**Nullo per permutazione.** M = 1000 permutazioni dell'ordine delle frasi (Fisher-Yates, stream deterministico da un seed fisso dichiarato nel codice), stessa matrice delle distanze. Per ogni k: media del nullo, quantili 2,5 % e 97,5 %, e p unilaterale «più vicine del caso» = (1 + #{nullo ≤ osservato}) / (M + 1).

**Tendenza.** ρ di Spearman fra k e V(k) per k = 1..P/2. Si riporta il segno e il valore; nessuna soglia.

**Classificazione per fixture** (solo descrittiva):
- `DEGENERATE`: tutte le distanze sono 0, il nullo non ha ampiezza;
- `ORDERED`: V(1) sotto il quantile 2,5 % del nullo (frasi vicine più simili del caso);
- `ORDER_FREE`: V(1) dentro la banda 2,5–97,5 %;
- `ANTI_ORDERED`: V(1) sopra il 97,5 %.

## 2. Fixture avverse

Tutte sintetiche, a livello di token, 24 frasi da 4 battute × 16 sedicesimi, un solo ruolo melodico (Vettore), Re dorico, generatore deterministico con seed dichiarato. Le altre corsie sono vuote.

| ID | Fixture | Costruzione |
|---|---|---|
| F1 | Loop letterale | Lo stesso motivo di 8 note in ogni frase |
| F2 | Rumore nella scala | Ogni frase: 8 note con onset, altezza nella scala e durata casuali |
| F3 | Sviluppo reale | Frase 0 = motivo; frase i = frase i−1 con **una** operazione elementare casuale: un grado ±1, un onset ±1 sedicesimo senza scavalcare, una durata ±1, inserire o togliere una nota |
| F4 | Sviluppo rimescolato | Le frasi di F3 in un ordine permutato con seed fisso |
| F5 | Trasposizione casuale | Il motivo trasposto di −5..+5 gradi, a caso per frase |
| F6 | Ornamenti casuali | Il motivo con 2 note di passaggio aggiunte in sedicesimi liberi, a caso per frase |
| F7 | Controllo positivo umano | Letto da `docs/research/experiments/m25_ensemble_tools/positive_control.json` se esiste; altrimenti `NOT_RUN`. **Non viene scritto dall'agente**: il dossier chiede un controllo scritto da una persona |

## 3. Predizioni

| Fixture | Classificazione attesa | Tendenza attesa | Motivo |
|---|---|---|---|
| F1 | `DEGENERATE` | nessuna (V = 0) | Frasi identiche |
| F2 | `ORDER_FREE` | nessun segno atteso | L'ordine non porta informazione |
| F3 | `ORDERED` | ρ > 0 | Lo stadio dipende dal precedente: la distanza cresce col lag |
| F4 | `ORDER_FREE` | nessun segno atteso | Stesse frasi di F3, ordine distrutto: **verifica che il nullo funzioni** |
| F5 | `DEGENERATE` | nessuna (V = 0) | La M4 è invariante per trasposizione. **È un limite atteso della misura**, non un pregio: se la trasposizione è sviluppo musicale, M4 non lo vede |
| F6 | `ORDER_FREE` | nessun segno atteso | Ornamenti indipendenti per frase |

Ordinamento atteso di V(1): F1 = F5 = 0 < F3 < F6 < F2.

**Criterio di cavallo.** La misura «variogramma M4 + nullo» è utilizzabile per la diagnosi solo se F3 è `ORDERED` con ρ > 0, F4 e F2 sono `ORDER_FREE`, F1 è `DEGENERATE`. Se F5 è `DEGENERATE`, la cecità alla trasposizione va dichiarata in ogni uso. Ogni predizione mancata si riporta come tale, senza ritoccare le fixture.

## 4. Fixture reale

`fixture-m2-4-v1`, seed 1337, per ruolo. **Nessuna predizione numerica**: la misura è descrittiva. Fatti già noti, che ci si aspetta di ritrovare: il ritmo di Pulso e Trama è fisso entro una sezione (dossier R-M25-7), quindi distanze basse a lag brevi; Trama è assente nelle frasi 0–3. Il ritorno A → A' è a lag 16 e si riporta V(16) contro il nullo, solo come descrizione.

## 5. Statistiche verticali su griglia intera

Sulla fixture reale, griglia di sedicesimi, per sezione:
- coincidenze di onset fra coppie di ruoli (conteggi);
- quota di onset di Vettore e di Trama che sono note dell'accordo della battuta (accordi del piano), separando il battere della battuta dagli altri sedicesimi.

Descrittive, nessuna predizione e nessuna soglia. Sostituiscono `vertical_check.py`, che leggeva artefatti non versionati.

## 6. Stima del costo logico dei progetti B, D e G

Formule della tabella del dossier, valutate con parametri dichiarati, in unità «valutazioni di passo» (non è una misura di tempo):

| Progetto | Formula del dossier | Parametri |
|---|---|---|
| A, attuale | C × B × S | C = 8 candidati, B = 4 battute, S = 16 sedicesimi |
| B, sequenziale con lettura | C × somma dei ruoli (B × S per ruolo) | R = 4 ruoli |
| D, fascio per ruolo | L × K × R, per passo | L = 3, K = 6: **valori ipotetici**, nessuno è deciso |
| G, revisione a K passaggi | K × R × C | K = 2 passaggi: **ipotetico** |

È un ordine di grandezza per CP-3, non un budget approvato.

## 7. Limiti dichiarati prima

- 24 frasi sono poche: tutto è diagnosi, non prova.
- Le fixture avverse sono scritte dall'agente che conosce la misura: possono essere troppo facili. F7, scritta da una persona, serve proprio a questo.
- Nessun ascolto: le misure non dicono nulla sulla percezione.
