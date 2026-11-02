# M2.4 Inc-5 — statistica delle affermazioni raggruppate: unità d'analisi, intervalli, baseline banale, negativi scarsi e duplicati

- **Data:** 2026-11-02
- **Stato:** documento di ricerca, *proposed*, non normativo. Risponde al rilievo OSS-5d-01 della review di 5d («un duplicato non è un'osservazione indipendente»; per 5e «resta da distinguere il numero di affermazioni da quello delle finestre musicali indipendenti», [progress.md](../progress.md)). Sostiene una precisazione di [ADR-0006](../adr/0006-m2-4-inc5-entry-decisions.md) D-M2-15 (5d-10, 5d-11) da decidere nell'ingresso di 5e. Non cambia codice, ADR né criteri.
- **Origine:** regola document-driven §6.4 ([document_driven_engineering.md](../../.agents/rules/document_driven_engineering.md)): ricerca parallela allo sviluppo, perché la statistica del checker `evaluation_check_v0` dipende da un tema non ancora documentato nel repository.
- **Metodo e livelli di verifica:** come nel [dossier d'ingresso di Inc-5](M2_4_Inc5_Entry_Research.md). Ogni fonte riporta riferimento completo, DOI o link permanente; consultazione 2026-11-02. Livelli:
  - **FT:** testo completo letto (PDF estratto localmente o HTML completo);
  - **AM:** solo abstract e metadati sulla pagina primaria o su Crossref/PubMed. Se manca anche l'abstract, la fonte è indicata come «AM (solo metadati)» e serve solo ad attribuire un nome o una formula, mai un risultato;
  - **SEC:** solo segnalazione secondaria; non sostiene alcuna affermazione.

  Le citazioni testuali sono brevi. Le inferenze sono marcate come tali.
- **Dati:** calcoli nuovi, analitici ed esatti per enumerazione (nessuna simulazione Monte Carlo), §2.4, §3.3, §4.2, §5.2. Script in Python puro (nessuna dipendenza), eseguiti con `python -I` e archiviati con l'output in [experiments/inc5_claim_stats/](experiments/inc5_claim_stats/README.md); la riesecuzione riproduce l'output al byte. Tutte le formule usate sono scritte nel testo, quindi i numeri si possono ricalcolare.

## 0. Base già presente nel repository

| Documento | Che cosa fornisce già | Uso qui |
|---|---|---|
| [Dossier d'ingresso di Inc-5](M2_4_Inc5_Entry_Research.md) §3.3 | controlli negativi bilanciati, duplicati nascosti, catch trial, item a verità alterata, esito «non determinabile», confronto con la baseline banale «con un intervallo di confidenza» | questo dossier precisa *quale* intervallo e *su quale unità* |
| [ADR-0006](../adr/0006-m2-4-inc5-entry-decisions.md) D-M2-15, precisazioni 5d-08..5d-17 | 5d-09 composizione del pacchetto `blind` (fino a 2 frasi di prova per gruppo (sezione, maschera), duplicato, item alterato, catch trial); 5d-10 regole dei controlli; 5d-11 Wilson 95 % e regola «somma degli estremi inferiori > 1»; 5d-16 duplicato e catch trial fuori dal campione | punto di partenza; §6 propone una precisazione per 5e |
| [evaluation_pack.cpp](../../engine/cacp/evaluation_pack.cpp), `wilsonInterval` e `checkEvaluation` | conteggi TP/FN/TN/FP **per affermazione** di `role_signal_present` (solo `listening`/`spectrogram`, escluse le finestre `catch` e `duplicate`); Wilson con z = 1,96; `INSUFFICIENT_DATA` solo con zero positivi o zero negativi; duplicati: coppie concordi su tutte le affermazioni booleane | §2.1 verifica la struttura del campione |
| [experiments/inc5_evaluation_pack](experiments/inc5_evaluation_pack/README.md) | manifest (11 finestre `blind`: 8 di prova, 1 duplicato, 1 alterato, 1 catch); calibrazione della presenza per frase; riepiloghi del checker per la baseline «sempre presente» (33/33, 0/3) e per Antigravity (36/36, 4/4; 13/13 coppie concordi) | numeri degli esempi (§5.3) |

## 1. Domande

1. **Q1 — intervalli con dati raggruppati:** quali metodi danno intervalli per sensibilità, specificità e indice di Youden quando le affermazioni sono raggruppate in finestre? Quali restano usabili con circa 9 finestre e stime al bordo (k/k)?
2. **Q2 — baseline banale:** la regola «somma dei due estremi inferiori di Wilson > 1» è un test valido? È conservativa? Quale regola adottare?
3. **Q3 — negativi scarsi:** che cosa si può affermare onestamente con 3–4 negativi? Quanti negativi e quante finestre servono perché una specificità sia informativa? Che cosa chiedono le linee guida di reporting?
4. **Q4 — duplicati nascosti:** quale statistica di concordanza test–retest usare con pochissime coppie?
5. **Q5 — procedura per CYMATICA:** unità d'analisi, intervalli, regola di baseline, soglie minime, trattamento dei controlli, dimensionamento del pacchetto, con esempi numerici.

## 2. Q1 — Intervalli per proporzioni con dati raggruppati

### 2.1 Verifica sul codice e struttura del campione

- **Unità contata:** `checkEvaluation` incrementa TP/FN/TN/FP per affermazione. Ogni finestra di prova contribuisce fino a 4 affermazioni `role_signal_present`, una per ruolo, quindi le affermazioni sono **annidate nelle finestre**.
- **Positivi raggruppati, negativi quasi isolati.** Nella fixture l'unico ruolo non pianificato è la Trama nelle frasi 0–3 ([presence_calibration](experiments/inc5_evaluation_pack/presence_calibration_2026-10-29.json)). A questo si aggiunge il ruolo rimosso dell'item alterato.
  - Ogni finestra contiene quindi al più un negativo naturale. Inferenza: anche la finestra alterata ne contiene uno solo, se la frase sorgente non è fra le 0–3.
  - I negativi sono già, di fatto, uno per finestra. Il raggruppamento pesa quasi solo sulla **sensibilità**: 3–4 positivi per finestra.
- **Livello superiore di raggruppamento.** Le finestre vengono da frasi di **una sola fixture** con un solo seed musicale. 5d-09 ne estrae fino a 2 per gruppo (sezione, maschera). Due finestre dello stesso gruppo condividono sezione, maschera e materiale tematico. Inferenza: anche le finestre non sono campioni indipendenti di «musica CYMATICA». La popolazione di riferimento di ogni intervallo è «le finestre di questa fixture», non la musica generata in generale.
- **Discrepanza fra i numeri disponibili.** Il riepilogo di Antigravity (36/36, 4/4, quindi 40 affermazioni) è stato prodotto dal checker **precedente** a OSS-5d-01, che contava anche il duplicato: 10 finestre. Con il checker attuale il campione è di 9 finestre e 36 affermazioni. Ad esempio, il pacchetto con seed 2718281828 dà 33 positivi e 3 negativi: la baseline ottiene 33/33 e 0/3. Gli esempi del §5.3 usano entrambe le configurazioni.

### 2.2 Fonti

| ID | Fonte | Verifica | Contenuto pertinente |
|---|---|---|---|
| C1 | E. B. Wilson, «Probable Inference, the Law of Succession, and Statistical Inference», *JASA* 22(158), 209–212, 1927. [DOI 10.1080/01621459.1927.10502953](https://doi.org/10.1080/01621459.1927.10502953) | AM (solo metadati) | Attribuzione dell'intervallo «score» di Wilson. La formula usata è quella del codice, verificata analiticamente (§4.2). |
| C2 | A. Agresti, B. A. Coull, «Approximate is Better than "Exact" for Interval Estimation of Binomial Proportions», *The American Statistician* 52(2), 119–126, 1998. [DOI 10.1080/00031305.1998.10480550](https://doi.org/10.1080/00031305.1998.10480550) | AM (solo metadati) | Attribuzione dell'intervallo di Agresti–Coull. |
| C3 | L. D. Brown, T. T. Cai, A. DasGupta, «Interval Estimation for a Binomial Proportion», *Statistical Science* 16(2), 101–133, 2001. [DOI 10.1214/ss/1009213286](https://doi.org/10.1214/ss/1009213286) | AM (abstract su Crossref; PDF non scaricabile) | Copertura «erratica» dell'intervallo di Wald. Raccomandano Wilson o Jeffreys a code uguali per n piccolo e Agresti–Coull per n più grande. |
| C4 | L. Kish, *Survey Sampling*, Wiley, 1965, ISBN 047148900X (metadati Open Library) | AM (solo metadati) | Attribuzione del *design effect*. Il libro non è stato letto: la formula si cita da C5 e C6. |
| C5 | S. Gabler, S. Haeder, P. Lahiri, «A model based justification of Kish's formula for design effects for weighting and clustering», *Survey Methodology* 25(1), 105–106, 1999. [Statistics Canada 12-001-X](https://www150.statcan.gc.ca/n1/pub/12-001-x/1999001/article/4718-eng.pdf) | FT (PDF) | Fattore di raggruppamento `1 + (b̄ − 1)ρ`, con b̄ dimensione media del cluster e ρ correlazione intraclasse. Giustificato sotto un modello a correlazione scambiabile (varianza σ², covarianza ρσ² entro cluster, 0 fra cluster); il valore di Kish è interpretabile come conservativo. |
| C6 | A. Donner, N. Klar, «Pitfalls of and Controversies in Cluster Randomization Trials», *AJPH* 94(3), 416–422, 2004. [DOI 10.2105/AJPH.94.3.416](https://doi.org/10.2105/AJPH.94.3.416); [PMC1448267](https://pmc.ncbi.nlm.nih.gov/articles/PMC1448267/) | FT (HTML PMC) | Design effect «1 + (m̄ − 1) ρ». Il concetto va chiamato «unit of inference» e non unità d'analisi. Instabilità delle stime di ρ: è pericoloso fidarsene con «less than about 40 clusters». Con ρ = 0,05 e 10 cluster, l'estremo superiore atteso al 95 % è 0,15. Sconsigliato testare ρ = 0 e ignorare il raggruppamento quando il test non è significativo. |
| C7 | S. M. Killip, Z. Mahfoud, K. Pearce, «What Is an Intracluster Correlation Coefficient? Crucial Concepts for Primary Care Researchers», *Ann Fam Med* 2(3), 204–208, 2004. [DOI 10.1370/afm.141](https://doi.org/10.1370/afm.141) | FT (HTML PMC; formule solo come immagini) | Definisce l'*effective sample size* dei campioni raggruppati. Aumentare il numero di cluster aumenta la potenza più efficacemente che aumentare i soggetti per cluster. |
| C8 | J. N. K. Rao, A. J. Scott, «A Simple Method for the Analysis of Clustered Binary Data», *Biometrics* 48(2), 577–585, 1992. [DOI 10.2307/2532311](https://doi.org/10.2307/2532311) | AM (PubMed) | Metodo basato su design effect ed effective sample size, senza un modello specifico per la correlazione intracluster. Applicabile con software per dati indipendenti dopo una pre-elaborazione. |
| C9 | T. S. S. Genders, S. Spronk, T. Stijnen, E. W. Steyerberg, E. Lesaffre, M. G. M. Hunink, «Methods for Calculating Sensitivity and Specificity of Clustered Data: A Tutorial», *Radiology* 265(3), 910–916, 2012. [DOI 10.1148/radiol.12120509](https://doi.org/10.1148/radiol.12120509) | AM (testo a pagamento, 403) | Confronta quattro approcci: analisi a livelli diversi ignorando la correlazione, aggiustamento della varianza, modelli logistici a effetti casuali, GEE. La scelta del livello d'analisi e di reporting deve corrispondere al problema decisionale. |
| C10 | C. Franco, R. J. A. Little, T. A. Louis, E. V. Slud, «Comparative Study of Confidence Intervals for Proportions in Complex Sample Surveys», *J. Survey Statistics and Methodology* 7(3), 334–364, 2019. [DOI 10.1093/jssam/smy019](https://doi.org/10.1093/jssam/smy019); [PMC6690503](https://pmc.ncbi.nlm.nih.gov/articles/PMC6690503/); [preprint](https://math.umd.edu/~slud/s770/SurveyConfidenceIntervals/JSSAM-2017-065-FINAL.pdf) | FT (preprint PDF e HTML PMC) | Intervalli Wilson, Jeffreys, Clopper–Pearson, Agresti–Coull ecc. con n sostituito dall'*effective sample size* n/Deff; valori non interi ammessi. Aggiustamenti per i gradi di libertà del disegno di Korn–Graubard (1998) e di Dean–Pagano (2015). «All intervals undercover when there is clustering» se il design effect è stimato in modo design-based. Raccomandano Wilson o la forma bayesiana con prior uniforme. I campioni con varianza stimata nulla sono esclusi dalle simulazioni. |
| C11 | N. Dean, M. Pagano, «Evaluating Confidence Interval Methods for Binomial Proportions in Clustered Surveys», *JSSAM* 3(4), 484–503, 2015. [DOI 10.1093/jssam/smv024](https://doi.org/10.1093/jssam/smv024) | AM (solo metadati) | Il contenuto si usa solo come descritto in C10. |
| C12 | K. Saha, D. Miller, S. Wang, «A Comparison of Some Approximate Confidence Intervals for a Single Proportion for Clustered Binary Outcome Data», *Int. J. Biostatistics* 12(2), 2016 (online 2015). [DOI 10.1515/ijb-2015-0024](https://doi.org/10.1515/ijb-2015-0024) | AM (abstract su Crossref) | Propone intervalli da profilo di verosimiglianza e Wilson per proporzioni con dati raggruppati. Li confronta con GEE, rapporto di Rao–Scott e metodi da indagine. I risultati numerici non sono letti. |
| C13 | A. C. Cameron, J. B. Gelbach, D. L. Miller, «Bootstrap-Based Improvements for Inference with Clustered Errors», *Review of Economics and Statistics* 90(3), 414–427, 2008. [DOI 10.1162/rest.90.3.414](https://doi.org/10.1162/rest.90.3.414); letto il [NBER Technical Working Paper 0344](https://www.nber.org/papers/t0344) (2007) | FT (working paper; la versione pubblicata può differire) | Con «few (5-30) clusters» i test asintotici cluster-robust sovra-rifiutano. Con pochi cluster il bootstrap produce un numero finito di pseudo-campioni e una distribuzione non liscia. Il *pairs cluster bootstrap* con un regressore binario costante entro cluster dà ricampionamenti «inestimable»; il problema è evidente per G ≤ 10. Il wild cluster bootstrap-t si comporta meglio. |
| C14 | C. A. Field, A. H. Welsh, «Bootstrapping Clustered Data», *JRSS B* 69(3), 369–390, 2007. [DOI 10.1111/j.1467-9868.2007.00593.x](https://doi.org/10.1111/j.1467-9868.2007.00593.x) | AM (abstract) | La consistenza della varianza bootstrap dipende dal modello. Il cluster bootstrap è consistente sia sotto il modello a trasformazione sia sotto quello a effetti casuali. I risultati teorici sono «limited and more mixed». |
| C15 | L. A. Mancl, T. A. DeRouen, «A Covariance Estimator for GEE with Improved Small-Sample Properties», *Biometrics* 57(1), 126–134, 2001. [DOI 10.1111/j.0006-341X.2001.00126.x](https://doi.org/10.1111/j.0006-341X.2001.00126.x) | AM (PubMed) | Con pochi cluster lo stimatore robusto delle GEE dà test di ampiezza fino a 2–3 volte quella nominale (10–40 soggetti). Jackknife e bootstrap possono non funzionare con risposte binarie per celle nulle. Propongono una correzione della distorsione. |
| C16 | N. A. Obuchowski, «On the comparison of correlated proportions for clustered data», *Statistics in Medicine* 17(13), 1495–1507, 1998. [DOI 10.1002/(SICI)1097-0258(19980715)17:13<1495::AID-SIM863>3.0.CO;2-I](https://doi.org/10.1002/(SICI)1097-0258(19980715)17:13%3C1495::AID-SIM863%3E3.0.CO;2-I) | AM (PubMed) | In presenza di correlazione intracluster l'ampiezza del test di McNemar può superare di molto quella nominale. Propone un metodo senza ipotesi sulla struttura di correlazione. |

### 2.3 Analisi dei metodi

| Metodo | Che cosa richiede | Comportamento con ~9 finestre e stima k/k | Giudizio per CYMATICA |
|---|---|---|---|
| **Ignorare il raggruppamento** (situazione attuale, per affermazione) | indipendenza delle affermazioni | intervallo troppo stretto se ρ > 0 (C6, C10, C16) | non adatto come intervallo primario (OSS-5d-01) |
| **Design effect ed effective n** (Kish; C5, C6, C8, C10) | ρ noto o stimato | ρ non è stimabile da dati tutti corretti (varianza entro e fra finestre nulla). Con meno di ~40 cluster la stima di ρ è instabile (C6). C10 esclude i campioni a varianza nulla, quindi non dice nulla proprio sul nostro caso | **utilizzabile con ρ fissato per ipotesi conservativa** (vedi sotto) |
| **Stimatore a rapporto con varianza cluster-robust** (Rao–Scott, linearizzazione; C8, C9) | varianza dei residui per cluster | con p̂ = 1 tutti i residui `y_c − p̂·m_c` sono nulli: varianza 0, intervallo di Wald degenerato [1, 1] (inferenza dalla formula). Con pochi cluster sovra-rifiuta (C13) | non adatto |
| **Cluster bootstrap** (C13, C14) | ricampionamento delle finestre | con 9 finestre ci sono C(17,9) = 24 310 ricampionamenti distinti; con p̂ = 1 ogni replica vale 1, quindi il percentile è degenerato. Se i negativi stanno in 3 finestre su 9, la probabilità che una replica non ne contenga nessuno è (6/9)⁹ = 0,026, e la specificità è indefinita; con 1 finestra sale a 0,346 (calcolo §2.4). È l'analogo del regressore binario costante entro cluster di C13 | non adatto |
| **GEE / modelli misti** (C9, C15) | stimatore robusto o verosimiglianza | con ~10 cluster lo stimatore robusto ha ampiezza 2–3 volte la nominale (C15). Con risposte tutte 1 la verosimiglianza logistica cresce senza limite nell'intercetta: la stima di massima verosimiglianza non esiste (inferenza elementare; fonte formale non letta, §7) | non adatto |
| **Aggregazione per finestra** (la finestra come unità; C6 «unit of inference») | indipendenza delle finestre | binomiale su C finestre: nessuna stima di ρ, validità esatta se le finestre sono indipendenti. Per i positivi va scelto un riassunto per finestra | **adatto**; coincide con l'effective n a ρ = 1 quando tutto è corretto |
| **Wilson / Agresti–Coull con effective n** (C10, C12) | n_eff e conteggio effettivo p̂·n_eff, anche non interi | C10 raccomanda Wilson in questa forma. La sotto-copertura nasce dall'errore di stima di n_eff (C10), che con ρ fissato a 1 non c'è | **adatto, con ρ = 1** |

**Scelta proposta (inferenza dalle fonti).** Con dati tutti o quasi tutti corretti, ρ non si può stimare. Secondo C6 anche con dati non degeneri la stima con meno di 40 cluster non è affidabile. L'unica ipotesi difendibile senza dati è quindi **ρ = 1** («ogni finestra conta come un'osservazione»): è il caso peggiore del modello scambiabile di C5. Ne segue:

- **Effective n:** sotto il modello di C5 con ρ = 1, la media pesata delle affermazioni ha varianza `σ²·Σ m_c² / (Σ m_c)²`. Quindi `n_eff = (Σ m_c)² / Σ m_c²`, dove m_c è il numero di affermazioni positive (o negative) della finestra c. Vale n_eff ≤ C, con uguaglianza a cluster uguali. Esempi: 9 finestre da 4 → 9,000; quattro da 3 e cinque da 4 → 8,828 (derivazione propria).
- **Intervallo:** Wilson con `x_eff = p̂·n_eff` e `n_eff`, come in C10 (valori non interi ammessi).
- **Proprietà:** l'intervallo così ottenuto non si restringe aggiungendo ruoli alla stessa finestra. È il comportamento richiesto da OSS-5d-01.

**Alternativa più conservativa:** finestra «corretta» solo se tutte le sue affermazioni positive sono corrette, e Wilson su (finestre corrette, C). Con dati perfetti coincide con la scelta proposta; con errori sparsi è più severa, perché un errore conta come una finestra intera. Va riportata come dato secondario, non come primario: tratta allo stesso modo 1 errore e 4 errori nella stessa finestra.

### 2.4 Calcoli

Sensibilità tutta corretta, 36 affermazioni in 9 finestre da 4. Il limite inferiore di Wilson al 95 % per n/n vale `n/(n + z²)` (§4.2):

| ρ ipotizzato | Deff = 1 + (m − 1)ρ | n_eff | Wilson inferiore |
|---|---|---|---|
| 0 (affermazioni indipendenti, checker attuale) | 1,0 | 36,0 | 0,904 |
| 0,1 | 1,3 | 27,7 | 0,878 |
| 0,3 | 1,9 | 18,9 | 0,831 |
| 0,5 | 2,5 | 14,4 | 0,789 |
| 1 (finestra come unità) | 4,0 | 9,0 | 0,701 |

Con 33 positivi in 9 finestre (m̄ = 3,67) a ρ = 1 si ottiene lo stesso n_eff = 9 (a cluster quasi uguali, 8,8–9,0) e lo stesso limite inferiore, 0,70.

## 3. Q2 — Intervallo per la distanza dal caso e regola di baseline

### 3.1 Fonti

| ID | Fonte | Verifica | Contenuto pertinente |
|---|---|---|---|
| C17 | W. J. Youden, «Index for rating diagnostic tests», *Cancer* 3(1), 32–35, 1950. [DOI 10.1002/1097-0142(1950)3:1<32::AID-CNCR2820030106>3.0.CO;2-3](https://doi.org/10.1002/1097-0142(1950)3:1%3C32::AID-CNCR2820030106%3E3.0.CO;2-3) | AM (solo metadati) | Attribuzione dell'indice J = Se + Sp − 1. Accuratezza bilanciata = (J + 1)/2. |
| C18 | R. G. Newcombe, «Two-sided confidence intervals for the single proportion: comparison of seven methods», *Statistics in Medicine* 17(8), 857–872, 1998. [DOI 10.1002/(SICI)1097-0258(19980430)17:8<857::AID-SIM777>3.0.CO;2-E](https://doi.org/10.1002/(SICI)1097-0258(19980430)17:8%3C857::AID-SIM777%3E3.0.CO;2-E) | AM (PubMed) | Valutati su 96 000 punti dello spazio dei parametri; raccomandati gli intervalli basati sulle code e i metodi score più semplici. |
| C19 | R. G. Newcombe, «Interval estimation for the difference between independent proportions: comparison of eleven methods», *Statistics in Medicine* 17(8), 873–890, 1998 (errata 1999). [DOI 10.1002/(SICI)1097-0258(19980430)17:8<873::AID-SIM779>3.0.CO;2-I](https://doi.org/10.1002/(SICI)1097-0258(19980430)17:8%3C873::AID-SIM779%3E3.0.CO;2-I) | AM (PubMed) | Un metodo che combina gli intervalli score di Wilson delle due proporzioni «also performs well» e si implementa a qualunque dimensione campionaria. |
| C20 | G. Y. Zou, A. Donner, «Construction of confidence limits about effect measures: A general approach», *Statistics in Medicine* 27(10), 1693–1702, 2008. [DOI 10.1002/sim.3095](https://doi.org/10.1002/sim.3095) | AM (PubMed) | Approccio generale in forma chiusa (MOVER) per differenze fra misure d'effetto, migliore del bootstrap in campioni piccoli o moderati. |
| C21 | G. Shan, «Improved Confidence Intervals for the Youden Index», *PLOS ONE* 10(7), e0127272, 2015. [DOI 10.1371/journal.pone.0127272](https://doi.org/10.1371/journal.pone.0127272) | FT (HTML PMC) | Per J: intervalli di Wilson delle due proporzioni combinati con «square-and-add» (Newcombe 1998): `J_L = Ĵ − z·√(l₁(1−l₁)/m + u₂(1−u₂)/n)` (nella parametrizzazione dell'articolo). Per J scritto come somma Se + Sp − 1, la forma MOVER usata nel §3.3 è `Ĵ − √((Ŝe − l_se)² + (Ŝp − l_sp)²)` (derivazione propria da C19–C20, non citazione). Raccomandato («The NP interval is recommended»). Simulazioni solo con m, n da 20 a 120 e J da 0,4 a 0,9. Nessun trattamento di dati raggruppati. |
| C22 | A. Agresti, B. Caffo, «Simple and Effective Confidence Intervals for Proportions and Differences of Proportions Result from Adding Two Successes and Two Failures», *The American Statistician* 54(4), 280–288, 2000. [DOI 10.1080/00031305.2000.10474560](https://doi.org/10.1080/00031305.2000.10474560) | AM (solo metadati) | Attribuzione dell'alternativa «aggiungere due successi e due insuccessi»; non valutata. |

### 3.2 Validità della regola attuale (analisi)

Sia `L_se`, `L_sp` l'estremo inferiore di Wilson al 95 % bilaterale, cioè un limite unilaterale nominale al 97,5 %. Se entrambi coprono, `Se + Sp ≥ L_se + L_sp`. Per Bonferroni, `P(entrambi coprono) ≥ 1 − 0,025 − 0,025 = 0,95`. Ne segue:

- `J_L = L_se + L_sp − 1` è un **limite inferiore unilaterale per J con livello nominale ≥ 95 %**;
- la regola «`L_se + L_sp > 1`» è un **test unilaterale di H₀: J ≤ 0 al livello nominale ≤ 5 %**. Un classificatore banale ha J = 0, e un classificatore che inverte le risposte ha J < 0.

Il livello è solo nominale: Wilson ha copertura approssimata. L'enumerazione esatta del §3.3 serve a verificarlo.

**Inferenza:** la regola è valida e conservativa. Prende l'angolo del rettangolo di confidenza invece di combinare le incertezze in quadratura, come fa MOVER (C19–C21). Il difetto non sta nella regola ma negli `n` che riceve: con n per affermazione il limite di sensibilità è troppo alto (0,904 invece di 0,701 a ρ = 1). Inoltre la regola restituisce solo un booleano: un passaggio con J_L = 0,04 e uno con J_L = 0,41 risultano identici.

### 3.3 Verifica esatta per enumerazione

Ampiezza effettiva del test = massimo, sulla frontiera H₀ (Se + Sp = 1, griglia di passo 0,005), della probabilità di dichiarare «sopra la baseline». La probabilità è calcolata in modo esatto sommando su tutti i risultati binomiali possibili.

| Positivi, negativi (affermazioni indipendenti) | Ampiezza della regola somma-Wilson | Ampiezza di MOVER/Newcombe (`J_L > 0`, livello nominale unilaterale 2,5 %) |
|---|---|---|
| 36, 4 | 0,0088 | 0,0477 |
| 9, 4 | 0,0047 | 0,0335 |
| 33, 3 | 0,0104 | **0,0754** |
| 9, 3 | 0,0053 | 0,0345 |
| 36, 1 | 0,0379 | **0,1075** |
| 9, 2 | 0,0054 | 0,0420 |

Positivi raggruppati: 9 finestre da 4, correttezza beta-binomiale con ICC ρ entro finestra; 4 negativi indipendenti.

| ρ | Regola su 36 affermazioni come indipendenti | Regola con n_eff = 9 (§2.3) |
|---|---|---|
| 0 | 0,0088 | 0,0007 |
| 0,2 | 0,0123 | 0,0009 |
| 0,5 | 0,0189 | 0,0015 |
| 1 | 0,0335 | 0,0047 |

**Risultati (misura propria):**

1. **La regola attuale non supera il 5 % in nessuna delle configurazioni calcolate**, anche contando per affermazione e anche con ρ = 1. Il raggruppamento ignorato ne quadruplica l'ampiezza (da 0,009 a 0,034) senza superare il livello nominale. Quindi OSS-5d-01 non rende *falso* l'esito booleano attuale. Lo rende **eccessivamente preciso negli intervalli riportati**: 0,904 dichiarato contro 0,701 difendibile.
2. **MOVER/Newcombe, raccomandato da C21, è anticonservativo con 1–3 negativi.** L'ampiezza effettiva è 0,075 e 0,107 contro il 2,5 % nominale. Queste numerosità sono fuori dall'intervallo simulato da C21 (m, n ≥ 20). Non va usato come gate con così pochi negativi.
3. Con dati perfetti, la regola della somma calcolata con limiti esatti di Clopper–Pearson (97,5 % unilaterale, `α^(1/n)` per n/n) è più severa. Con 9/9 e 3/3 dà J_L = −0,044, cioè non supera; Wilson dà +0,139. La differenza nasce interamente dalla scelta fra copertura minima garantita (Clopper–Pearson, C10) e copertura media vicina alla nominale (Wilson, C3, C10).

### 3.4 Regola proposta

- **Mantenere** `J_L = L_se + L_sp − 1 > 0` come regola di baseline. È valida, conservativa, verificata per enumerazione e in forma chiusa.
- **Calcolarla con gli n effettivi a livello di finestra** (§2.3), non per affermazione.
- **Riportare il valore numerico di `J_L`** (e di `Ĵ`) oltre al booleano.
- **Accompagnarla con una soglia minima di negativi** (§4.3). Senza, un solo negativo basta a «superare la baseline» (§4.2), un esito valido come test ma non informativo.

## 4. Q3 — Campioni piccoli e negativi scarsi

### 4.1 Fonti

| ID | Fonte | Verifica | Contenuto pertinente |
|---|---|---|---|
| C23 | J. A. Hanley, A. Lippman-Hand, «If Nothing Goes Wrong, Is Everything All Right? Interpreting Zero Numerators», *JAMA* 249(13), 1743–1745, 1983. [DOI 10.1001/jama.1983.03330370053031](https://doi.org/10.1001/jama.1983.03330370053031) | AM (solo metadati; nessun abstract in PubMed) | Attribuzione della «regola del tre». Il contenuto non è letto: la regola si usa tramite C24 e la derivazione esatta del §4.2. |
| C24 | E. Eypasch, R. Lefering, C. K. Kum, H. Troidl, «Probability of adverse events that have not yet occurred: a statistical reminder», *BMJ* 311(7005), 619–620, 1995. [DOI 10.1136/bmj.311.7005.619](https://doi.org/10.1136/bmj.311.7005.619) | AM (abstract PMC; testo solo come immagine) | Con zero eventi su n, rischio massimo ≈ «3/n (for n > 30)», dalla formula di Hanley. |
| C25 | P. M. Bossuyt et al., «STARD 2015: an updated list of essential items for reporting diagnostic accuracy studies», *BMJ* 351:h5527, 2015. [DOI 10.1136/bmj.h5527](https://doi.org/10.1136/bmj.h5527); [PMC4623764](https://pmc.ncbi.nlm.nih.gov/articles/PMC4623764/) | FT (HTML PMC) | Item 15: gestione dei risultati indeterminati. Item 18: dimensione campionaria prevista e come è stata determinata. Item 19: flusso dei partecipanti. Item 23: tabulazione incrociata indice × riferimento. Item 24: «Estimates of diagnostic accuracy and their precision (such as 95% confidence intervals)». **Non tratta osservazioni multiple per partecipante.** |
| C26 | A. Flahault, M. Cadilhac, G. Thomas, «Sample size calculation should be performed for design accuracy in diagnostic test studies», *J Clin Epidemiol* 58(8), 859–862, 2005. [DOI 10.1016/j.jclinepi.2004.12.009](https://doi.org/10.1016/j.jclinepi.2004.12.009) | AM (PubMed) | Il numero minimo di casi si fissa in base al limite inferiore di confidenza accettabile della sensibilità (tabelle binomiali); i controlli si ricavano dalla prevalenza. |

### 4.2 Calcoli

**Forma chiusa.** Per x = n, il limite inferiore di Wilson è `n/(n + z²)`, con z² = 3,8415. Infatti p̂ = 1 annulla il termine p̂(1 − p̂). Il valore coincide con l'uscita del codice per 36/36 (0,9036) e per 4/4 (0,5101). Il limite esatto unilaterale di Clopper–Pearson per n/n è `α^(1/n)`. Il massimo tasso d'errore compatibile al 95 % unilaterale con 0 errori su n è `1 − 0,05^(1/n)`: la regola del tre `3/n` (C24) lo sovrastima sempre, poiché −ln 0,05 = 2,996 < 3.

| n (tutti corretti) | Wilson 95 % inferiore | Clopper–Pearson 97,5 % unilaterale | Errore massimo compatibile (95 % unilaterale, esatto) | Regola del tre 3/n | Wilson inferiore con 1 errore |
|---|---|---|---|---|---|
| 1 | 0,207 | 0,025 | 0,950 | — | — |
| 2 | 0,342 | 0,158 | 0,776 | — | — |
| 3 | 0,439 | 0,292 | 0,632 | 1,00 | — |
| 4 | 0,510 | 0,398 | 0,527 | 0,75 | 0,301 |
| 5 | 0,566 | 0,478 | 0,451 | 0,60 | 0,376 |
| 8 | 0,676 | 0,631 | 0,312 | 0,38 | 0,529 |
| 9 | 0,701 | 0,664 | 0,283 | 0,33 | — |
| 10 | 0,723 | 0,692 | 0,259 | 0,30 | 0,596 |
| 16 | 0,806 | 0,794 | 0,171 | 0,19 | 0,717 |
| 20 | 0,839 | 0,832 | 0,139 | 0,15 | 0,764 |
| 30 | 0,887 | 0,884 | 0,095 | 0,10 | 0,833 |
| 35 | 0,901 | 0,900 | 0,082 | 0,086 | 0,855 |

Numero minimo di negativi indipendenti, tutti corretti, perché il limite inferiore superi la soglia L:

| L | Wilson 95 % | Clopper–Pearson 97,5 % |
|---|---|---|
| 0,5 | 4 | 6 |
| 0,7 | 9 | 11 |
| 0,8 | 16 | 17 |
| 0,9 | 35 | 36 |
| 0,95 | 73 | 72 |

### 4.3 Che cosa si può affermare con 3–4 negativi (analisi)

- **3/3 corretti:** compatibili con un tasso di falsi positivi fino al 63 % (95 % unilaterale esatto). Il limite di Wilson, 0,44, non esclude una specificità da lancio di moneta. Si può dire solo «nessun falso positivo osservato su 3 finestre negative».
- **4/4 corretti:** 0,51 è appena sopra 0,5, con un tasso di falsi positivi compatibile fino al 53 %. È la soglia minima perché un risultato perfetto escluda la specificità del caso con Wilson: `n > z²`, cioè n ≥ 4. Con Clopper–Pearson ne servono 6.
- **Effetto sulla regola di baseline:** per affermazione con 36/36 basta **1 solo negativo** corretto per «superare la baseline»: 0,904 + 0,207 > 1, ampiezza effettiva 0,038. A livello di finestra (9/9) ne bastano 2: 0,701 + 0,342 = 1,043. Il test resta valido (§3.3), ma la conclusione su J dipende quasi solo dalla sensibilità.
- **Reporting** (C25, analogia dalla diagnostica medica): tabulazione incrociata completa (item 23); intervalli al 95 % (item 24); dimensione prevista e sua motivazione (item 18); gestione degli esiti indeterminati, qui `abstained` e `unverifiable` (item 15). STARD non copre il raggruppamento. Per questo C9 e C6 indicano di dichiarare il livello d'analisi.
- **Dimensionamento** (C26, C7): il numero minimo di negativi si fissa *prima* in base al limite inferiore che si vuole poter dimostrare. A parità di affermazioni, conviene aumentare il numero di finestre indipendenti, non i ruoli per finestra (C7).

## 5. Q4 — Concordanza sui duplicati nascosti

### 5.1 Fonti

| ID | Fonte | Verifica | Contenuto pertinente |
|---|---|---|---|
| C27 | A. R. Feinstein, D. V. Cicchetti, «High agreement but low Kappa: I. The problems of two paradoxes», *J Clin Epidemiol* 43(6), 543–549, 1990. [DOI 10.1016/0895-4356(90)90158-L](https://doi.org/10.1016/0895-4356(90)90158-L) | AM (PubMed) | Un'alta concordanza osservata p₀ può essere «drastically lowered» da kappa quando i marginali sono sbilanciati. La sostituzione con kappa max non risolve. |
| C28 | T. Byrt, J. Bishop, J. B. Carlin, «Bias, prevalence and kappa», *J Clin Epidemiol* 46(5), 423–429, 1993. [DOI 10.1016/0895-4356(93)90018-V](https://doi.org/10.1016/0895-4356(93)90018-V) | AM (PubMed) | Kappa dipende da bias e prevalenza; indici separati di bias e prevalenza. Raccomandano di non riportare kappa da solo. La formula PABAK = 2p₀ − 1 non è nell'abstract letto: qui è derivata (kappa con p_e = 0,5). |
| C29 | K. L. Gwet, «Computing inter-rater reliability and its variance in the presence of high agreement», *Br J Math Stat Psychol* 61(1), 29–48, 2008. [DOI 10.1348/000711006X126600](https://doi.org/10.1348/000711006X126600) | AM (abstract su Crossref) | Coefficiente AC1, più stabile rispetto ai paradossi di kappa, con stimatori di varianza. |
| C30 | N. Wongpakaran, T. Wongpakaran, D. Wedding, K. L. Gwet, «A comparison of Cohen's Kappa and Gwet's AC1…», *BMC Med Res Methodol* 13:61, 2013. [DOI 10.1186/1471-2288-13-61](https://doi.org/10.1186/1471-2288-13-61) | FT (HTML PMC) | Formule a due valutatori: κ con `e(K) = (A1/N)(B1/N) + (A2/N)(B2/N)`; AC1 con `e(γ) = 2q(1 − q)`. Casi reali con 88 % di concordanza e kappa 0. |

### 5.2 Analisi e calcoli

- **Struttura reale:** «13/13 coppie concordi» (Antigravity) sono 13 affermazioni di predicati diversi sulla **stessa** finestra duplicata. Le osservazioni indipendenti di coerenza sono quindi **D = 1**. Un tasso di concordanza con intervallo non ha senso: Wilson per 1/1 dà 0,21.
- **Kappa** (calcolo proprio sulle formule di C30):
  - 13 coppie tutte «vero»: p_e = 1, kappa indefinito (0/0);
  - 12 «vero–vero» e 1 discordanza: p₀ = 0,923, kappa = 0, AC1 = 0,917, PABAK = 0,846.

  È il paradosso di C27: kappa non è utilizzabile con marginali così sbilanciati. AC1 (C29) e PABAK (C28) restano vicini a p₀, ma con D = 1 nessun coefficiente stima una ripetibilità.
- **Raccomandazione:**
  - riportare la concordanza grezza, con i conteggi **per finestra duplicata** e il numero D;
  - con D piccolo, trattare il duplicato come **controllo a esito** (concorde o discorde, con il predicato in disaccordo), non come stima;
  - una stima di ripetibilità richiede D finestre duplicate e indipendenti. Con tutte concordi, Wilson supera 0,5 da D = 4 e 0,8 da D = 16, come per la specificità.

  Inferenza: kappa e AC1 si riportano solo come descrittivi accanto a p₀ e ai marginali (C28), e solo con D ≥ 4.

## 6. Q5 — Procedura raccomandata per CYMATICA

### 6.1 Proposta `presence_stats_v1` (da decidere nell'ingresso di 5e)

1. **Unità d'inferenza:** la finestra `blind` indipendente, cioè le finestre `test` e `altered`. Restano fuori `duplicate` (controllo di coerenza) e `catch` (controllo d'esclusione), come oggi (5d-16). Le affermazioni sono osservazioni annidate.
2. **Campione:** come oggi, solo `role_signal_present` da `listening` o `spectrogram`, deduplicato (5d-15).
3. **Sensibilità e specificità:** Wilson 95 % con `n_eff = (Σ m_c)² / Σ m_c²` e `x_eff = p̂·n_eff`, separatamente per positivi e negativi (ρ = 1, §2.3). Si riportano anche:
   - conteggi per affermazione (TP, FN, TN, FP) e numero di finestre che contribuiscono;
   - il conteggio secondario delle finestre «tutte corrette»;
   - nessun intervallo calcolato per affermazione: indurrebbe a leggerlo come precisione reale.
4. **Baseline:** `J_L = L_se + L_sp − 1` sui valori del punto 3; `above_trivial_baseline = J_L > 0`; nel report anche `Ĵ`, `J_L` e l'accuratezza bilanciata `(Ĵ + 1)/2`. MOVER non si usa come gate (§3.3).
5. **Soglie minime (fail-closed):**
   - `n_eff` dei negativi < 4 **oppure** `n_eff` dei positivi < 4 → stato `INSUFFICIENT_DATA`, tassi e intervalli riportati come descrittivi e `above_trivial_baseline` nullo. Motivazione: sotto n = 4 anche un risultato perfetto non esclude 0,5 con Wilson (`n > z²`);
   - catch trial fallito → `EXCLUDED_BY_CATCH_RULE`, come oggi;
   - la soglia si fissa prima della misura e si versiona.
6. **Esiti non booleani** (C25 item 15): `abstained` e `unverifiable` restano fuori dai tassi, contati a parte per finestra. Si aggiunge un'analisi di sensibilità nel caso peggiore (`abstained` = errore), riportata accanto, mai al posto del primario.
7. **Tipi di negativo:** riportare separatamente i negativi naturali (ruolo non pianificato) e quelli per rimozione (item alterato). Hanno meccanismi diversi e il secondo serve anche al controllo di plan-following (5d-10).
8. **Duplicati:** `duplicate_windows` (D) e concordanza per finestra; esito di controllo. Nessun kappa con D < 4 (§5.2).
9. **Portata dichiarata nel report:** «intervalli condizionati alle finestre di questa fixture e di questo seed musicale; nessuna generalizzazione ad altri seed». Si dichiarano anche il numero di frasi e di gruppi (sezione, maschera) distinti.
10. **Versionamento:** il report dichiara `stats_profile: presence_stats_v1` con z, ρ ipotizzato, regola e soglie. Un cambiamento crea una nuova versione, senza riscrivere i risultati passati.

### 6.2 Dimensionamento del pacchetto (proposta)

- **Specificità:** almeno **4** negativi indipendenti perché la specificità sia riportabile. Per poter dimostrare Sp ≥ 0,7 / 0,8 / 0,9 con un risultato perfetto ne servono 9 / 16 / 35 (Wilson; con Clopper–Pearson 11 / 17 / 36).
- **Negativi bilanciati** (dossier d'ingresso §3.3): numero di finestre negative comparabile a quello delle positive. Inferenza: la fixture ha negativi naturali in 4 frasi (Trama, frasi 0–3) e finestre della stessa frase non sono indipendenti. Servono quindi negativi **per rimozione dall'audio** distribuiti su frasi e ruoli diversi, oppure una fixture con più maschere. Entrambe le strade cambiano 5d-09 e sono decisioni del titolare.
- **Sensibilità:** con 9 finestre il limite inferiore massimo è 0,70. Per arrivare a 0,8 servono ≥ 16 finestre positive; per 0,9, ≥ 35. Aggiungere ruoli per finestra non aiuta (§2.4, C7).
- **Duplicati:** D ≥ 4 solo se si vuole una stima di coerenza; altrimenti D = 1 resta un controllo.

### 6.3 Esempi numerici

| Caso | Se: conteggio → limite | Sp: conteggio → limite | J_L (regola somma) | Esito `presence_stats_v1` |
|---|---|---|---|---|
| A1. Antigravity, checker attuale v0 (per affermazione) | 36/36 → 0,904 | 4/4 → 0,510 | 0,414 | — (riferimento) |
| A2. Stesso dato, finestra come unità (9 finestre, n_eff = 9) | 9/9 → 0,701 | 4/4 → 0,510 | **0,211** | `MEASURED`, sopra la baseline. Sp riportabile ma debole: errore massimo compatibile 53 % |
| B1. Configurazione del checker attuale, per affermazione | 33/33 → 0,896 | 3/3 → 0,439 | 0,334 | — (riferimento) |
| B2. Stesso dato, finestra come unità | 9/9 → 0,701 | 3/3 → 0,439 | 0,139 | **`INSUFFICIENT_DATA`** (n_eff dei negativi = 3 < 4); J_L riportato come descrittivo |
| C. Baseline «sempre presente», finestra come unità | 9/9 → 0,701 | 0/3 → 0 | < 0 | catch trial fallito → `EXCLUDED_BY_CATCH_RULE`; anche senza, sotto la baseline |

Note sugli esempi:

- In A1 e A2 i 36 positivi in 9 finestre sono i numeri richiesti nel mandato. Come osservato nel §2.1, i 36/36 e 4/4 reali di Antigravity venivano da 10 finestre, duplicato incluso. Il calcolo serve da illustrazione.
- Per confronto, MOVER dà J_L = 0,501 (A1), 0,426 (A2), 0,429 (B1), 0,364 (B2). È più alto e, con questi negativi, non controllato (§3.3).
- Con Clopper–Pearson, A2 dà J_L = 0,061 e B2 −0,044.

### 6.4 Matrice delle alternative

| Opzione | Conformità con OSS-5d-01 | Validità con k/k e ~9 cluster | Complessità nel checker C++ | Rischio |
|---|---|---|---|---|
| 0. Lasciare tutto com'è | no | test valido (§3.3), intervalli troppo stretti | nessuna | si dichiara una precisione inesistente |
| 1. **Wilson con n_eff a ρ = 1 + regola della somma + soglia di 4 negativi (proposta)** | sì | sì; ampiezza ≤ 0,005 nei casi calcolati | bassa (formula chiusa) | conservativo; con la fixture attuale B2 diventa `INSUFFICIENT_DATA` |
| 2. Come 1, ma con Clopper–Pearson | sì | copertura minima garantita (C10) | bassa (bisezione o `α^(1/n)`) | più severo: A2 passa a stento, B2 no; servono 6 negativi |
| 3. Design effect con ρ stimato | parziale | ρ non stimabile da dati perfetti; instabile sotto 40 cluster (C6) | media | fallisce proprio nel caso osservato |
| 4. Cluster bootstrap | sì in teoria | degenerato (§2.3; C13) | media | intervalli [1, 1] e repliche senza negativi |
| 5. GEE o modelli misti | sì in teoria | stima inesistente con separazione completa; ampiezza 2–3 volte la nominale (C15) | alta, dipendenza nuova | vietato da AGENTS §2 senza approvazione e inutile qui |
| 6. MOVER/Newcombe per J | indifferente | anticonservativo con ≤ 3 negativi (§3.3) | bassa | falsi «sopra la baseline» |

**Raccomandazione:** opzione 1, con l'opzione 2 riportata come analisi di sensibilità. Sono difendibili la soglia a 6 negativi (Clopper–Pearson) e l'esclusione dell'item alterato dalla specificità. Entrambe spettano al titolare.

## 7. Non verificate e non usate

- **Donner e Klar, *Design and Analysis of Cluster Randomization Trials in Health Research*, Arnold, 2000** (ISBN 0340691530, metadati Open Library): non letto; si usa C6 dello stesso autore.
- **Donner, Birkett, Buck, «Randomization by cluster», *Am J Epidemiol* 114(6), 906–914, 1981** ([DOI](https://doi.org/10.1093/oxfordjournals.aje.a113261)): solo metadati.
- **Korn e Graubard 1998** (*Survey Methodology*, intervalli di Clopper–Pearson con effective n e gradi di libertà del disegno): visto solo descritto in C10 (SEC).
- **Kish 1995, «Methods for design effects», *J. Official Statistics*:** non reperito.
- **Albert e Anderson, «On the existence of maximum likelihood estimates in logistic regression models», *Biometrika* 71(1), 1–10, 1984** ([DOI](https://doi.org/10.1093/biomet/71.1.1)): solo metadati. La non esistenza della stima con risposte tutte uguali è qui un'inferenza elementare, non una citazione.
- **Jovanovic e Levy, «A Look at the Rule of Three», *Am Stat* 51(2), 137–139, 1997** ([DOI](https://doi.org/10.1080/00031305.1997.10473947)): solo metadati.
- **Clopper e Pearson 1934:** non letto; il limite esatto si calcola direttamente come inversione della coda binomiale (§4.2), e la sua copertura garantita è riportata da C10.
- **Testo completo di C3, C9, C19, C21:** C3 e C9 non scaricabili (403 o HTML al posto del PDF). Le affermazioni attribuite si limitano all'abstract, salvo C21, letto in FT.
- **Pagine web secondarie** (Wikipedia, blog su Newcombe–Wilson, calcolatori online): viste nei risultati di ricerca, non usate.

## 8. Ricadute proposte su ADR-0006 (decisione del titolare)

| Precisazione | Proposta | Fonti |
|---|---|---|
| 5d-11 (emendamento, per 5e) | Unità = finestra; Wilson con n_eff a ρ = 1; `J_L` numerico nel report; regola della somma invariata | §2.3, §3.2–3.4; C5, C6, C10, C13, C15 |
| 5d-11 (nuova soglia) | `INSUFFICIENT_DATA` se n_eff dei negativi o dei positivi < 4. Alternativa: 6 con Clopper–Pearson | §4.2–4.3 |
| 5d-09 (composizione) | Negativi indipendenti ≥ 4, da frasi diverse; valutare negativi per rimozione bilanciati per ruolo; dichiarare il numero di frasi e gruppi distinti | §6.2; dossier d'ingresso §3.3 |
| 5d-10 (duplicati) | Riportare D e la concordanza per finestra; nessuna statistica di ripetibilità con D < 4 | §5.2; C27–C30 |
| Report v0 | `stats_profile`, conteggi per affermazione e per finestra, analisi di sensibilità per le astensioni, portata «questa fixture» | C25 |

Il checker attuale **non viene modificato** da questo dossier. Le precisazioni richiedono decisione, implementazione, test e review secondo AGENTS §1.1 e §5.1.1. Il dossier è indicizzato in `docs/research/README.md`.

## 9. Limiti

- **Analogie:** tutte le fonti trattano pazienti, indagini o trial a cluster, non agenti che valutano audio. Il trasferimento è un'analogia metodologica, come nel dossier d'ingresso.
- **Indipendenza delle finestre:** anche ρ = 1 entro finestra presuppone finestre indipendenti fra loro. Finestre della stessa sezione e maschera possono essere correlate. Un livello «frase» o «gruppo» non è modellato: con 8–9 finestre non resterebbe un campione utile. Va dichiarato come limite di portata.
- **Ipotesi del modello:** il modello beta-binomiale del §3.3 è uno solo fra i possibili modelli di dipendenza. L'enumerazione verifica la regola su una griglia di Se e su poche configurazioni, non in generale.
- **Copertura di Wilson:** l'enumerazione propria mostra che il limite inferiore unilaterale (nominale 97,5 %) scende a circa 0,83 per p vicino a 0 (n = 3–36). Per p ≥ 0,8 resta ≥ 0,98. La regola di baseline è verificata nel suo insieme (§3.3), non per componente.
- **Fonti AM:** C1–C4, C8, C9, C11, C12, C14–C20, C22–C24, C26–C29 sono verificate solo su abstract o metadati. Nessuna affermazione numerica è loro attribuita oltre l'abstract.
- **Calcoli:** archiviati in [experiments/inc5_claim_stats/](experiments/inc5_claim_stats/README.md) e riprodotti al byte.
