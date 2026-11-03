# Prompt per la review qualitativa e tecnica di ADR-0006 (Antigravity, fine Inc-5)

- **Data:** 2026-11-03
- **Origine:** decisione del titolare del 2026-10-29 («a fine Inc-5 Antigravity esamina anche le decisioni di ADR-0006, con ricerche approfondite di supporto per i punti aperti»); ADR-0006 5e-10.
- **Regole di verificabilità:** derivano dalla [riconciliazione del modello matematico](M2_4_Inc5_Math_Model_Reconciliation.md) §2 e §8, che ha trovato in una consegna precedente SHA, file, funzioni e parametri inesistenti.

Il testo seguente si consegna ad Antigravity così com'è.

```text
Ruolo: revisore tecnico-scientifico indipendente del progetto CYMATICA (repository naturewhisp/cymatica, Windows, C++20/CMake/MSVC).

Compito: review qualitativa e tecnica delle decisioni di docs/adr/0006-m2-4-inc5-entry-decisions.md (ADR-0006, M2.4 Inc-5), con ricerche approfondite di supporto per i punti aperti. Non è una review del codice riga per riga, ma il codice è la verifica di ciò che l'ADR dichiara.

Commit: lavora sul commit che il titolare ti indica. Riporta all'inizio del dossier l'output grezzo di:
  git rev-parse HEAD
  git status --short
Usa solo quello SHA completo; non scrivere SHA a memoria.

Letture obbligatorie
- ADR-0006 per intero, comprese le precisazioni 5d, 5e, 5e.1–5e.4a, R-01 e le decisioni del titolare (sessione alla cieca rinviata a M2.5).
- docs/progress.md, sezioni di Inc-5 (da «M2.4 Inc-5 — piano a fasi» in poi).
- docs/m2_procedural_evidence.md (evidenze, mappatura M2.5, handoff).
- docs/research/M2_4_Inc5_Math_Model_Claude.md, M2_4_Inc5_Math_Model_Antigravity.md e M2_4_Inc5_Math_Model_Reconciliation.md.
- I dossier di ricerca collegati da ADR-0006 (metodi CACP, entry research, statistica delle affermazioni raggruppate, calibrazione AC-AO-02).

Domande
1. Coerenza: ogni decisione di ADR-0006 (D-M2-12..16, 5d-*, 5e-*, precisazioni) è attuata dal codice e dai test come dichiarato? Segnala le divergenze, con evidenza.
2. Solidità: le regole fissate prima delle misure (H1/H2/R2, P1–P4 e M-NORM, previsione dei gruppi di 5e.3c, self-test S1–S6) sono formulate in modo da poter fallire? Ci sono regole tautologiche o costruite a posteriori?
3. Statistica: presence_stats_v1 (gruppo musicale come unità, n_eff di Kish con ρ = 1, Wilson su n_eff, soglie 4 gruppi / n_eff ≥ 4, J_L). È adeguata allo scopo? Quali alternative difendibili esistono per M2.5 (dipendenza fra gruppi imparentati, CL-MM-10)? Sostieni le risposte con fonti.
4. Misura: il rivelatore di onset con normalizzazione globale (M-NORM) e la scelta per M2.5 fra normalizzazione globale, locale o robusta. Quali soluzioni usa la letteratura per l'onset detection per stem o per traccia? Con fonti.
5. Rilascio: dato release_ab_v1 (H1 NOT_SUPPORTED sulla fixture, H2 SUPPORTED) e il modello delle pendenze, quali criteri dovrebbe usare M2.5 per scegliere il rilascio per ruolo? Con fonti su de-click, inviluppi e udibilità dei transitori di rilascio.
6. Varietà: il teorema D (il seed agisce solo sui pareggi) implica che la varietà deve venire dallo stato. Quali approcci documentati (stato lungo la forma, variazione dei motivi, direttore musicale) sono compatibili con i vincoli di determinismo e fairness di CYMATICA? Con fonti.
7. Handoff: la mappatura M2.5 di docs/m2_procedural_evidence.md è corretta e completa? Manca qualche criterio o evidenza?

Per le ricerche: fonti primarie dove possibile (articoli, standard, libri), ciascuna con autore, titolo, anno, DOI o URL e livello di verifica (letta / consultata l'abstract / citata da altra fonte). Distingui i fatti dalle tue proposte.

Regole di verificabilità (obbligatorie)
- Prima di citare un file, una funzione, una costante o un finding, trovalo con una ricerca nel repository; riporta il comando e la riga trovata. Se non lo trovi, non citarlo.
- Per ogni rilievo, copia l'estratto di codice o di documento a cui si riferisce, con il percorso relativo al repository e il numero di riga. Non usare link file:///.
- Per ogni comando eseguito (build, test, scenari) riporta l'output grezzo rilevante, non un riassunto.
- Non dichiarare parametri (budget, finestre, domini) senza l'estratto che li definisce.
- Se una conclusione deriva da un ragionamento e non da una verifica, dichiaralo.

Vincoli
- Nessuna modifica a codice, test, build, documenti esistenti o golden. Nessun commit, nessun push.
- L'unico file che crei nel repository è docs/research/M2_4_Inc5_ADR0006_Review_Antigravity.md.
- La sessione alla cieca è rinviata a M2.5: non eseguirla e non generare pacchetti per valutatori.
- Nessuna nuova affermazione di validazione percettiva o statistica. Se ascolti dei WAV, dichiara i limiti del tuo canale audio (ADR-0005 D-M2-11).

Formato del dossier
1. Riproducibilità: output di git rev-parse HEAD e git status --short, ambiente, comandi eseguiti con output.
2. Sommario (massimo una pagina) con verdetto per area: coerente / con divergenze / da rivedere.
3. Un capitolo per domanda (1–7), con evidenze e fonti.
4. Tabella dei rilievi con ID AG-ADR-NN: decisione di ADR-0006, affermazione, evidenza (estratto con file:riga), stato (confermata / divergenza / da rivedere / proposta per M2.5), severità.
5. Proposte per M2.5, separate dai rilievi.
6. Bibliografia con livello di verifica.

Rispondi in italiano. Al termine riporta il file creato, i comandi eseguiti, i rilievi principali e i limiti della review.
```
