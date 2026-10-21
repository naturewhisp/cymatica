---
trigger: always_on
description: Direttiva trasversale su rigore tecnico-scientifico, sviluppo document-driven, tracciabilità bidirezionale, separazione epistemica e decisioni fondate su evidenze documentate.
---

# Rigore Tecnico-Scientifico e Sviluppo Document-Driven — CYMATICA

## 1. Documentazione come Riferimento Permanente e Gerarchia Normativa
- Ogni incremento tecnico (es. Inc-1..Inc-5) deve identificare preliminarmente i riferimenti normativi applicabili (Specifica, DESIGN, AGENTS.md, ADR, Piano Operativo).
- È fatto divieto assoluto di modificare contratti approvati tramite documenti di ricerca, fixture sperimentali o assunzioni tacite.
- La gerarchia normativa è tassativa:
  $$\text{Specifica Tecnica} \succ \text{DESIGN.md} \succ \text{AGENTS.md} \succ \text{ADR} \succ \text{Piano Operativo} \succ \text{Ricerca / R&D}$$

## 2. Tracciabilità Bidirezionale
- Ogni modifica al codice deve rispettare la catena di tracciabilità:
  $$\text{Requisito} \longrightarrow \text{Decisione/ADR} \longrightarrow \text{Progettazione} \longrightarrow \text{Implementazione} \longrightarrow \text{Test} \longrightarrow \text{Evidenza}$$
- Non implementare funzionalità o contratti non deliberati nella specifica o negli ADR.
- Non descrivere nella documentazione o nei manifest funzionalità non effettivamente realizzate e verificate nella baseline corrente.

## 3. Separazione Epistemica, Metodo Scientifico e Ricerca Approfondita
Nelle analisi, nei log di decisione e nei report, distinguere sempre categoricamente:
1. **Fatti dimostrati:** proprietà analitiche, matematiche o empiriche verificate.
2. **Invarianti normativi approvati:** contratti vigenti vincolanti (`accepted for implementation`).
3. **Ipotesi progettuali:** scelte ingegneristiche in fase di confronto o validazione.
4. **Risultati sperimentali:** misurazioni numeriche concrete da test, profiler o manifest CACP.
5. **Questioni aperte:** decisioni differite o parametri rimessi alla sperimentazione percettiva.

Nessuna complessità algoritmica o dipendenza esterna deve essere introdotta senza un beneficio scientificamente dimostrabile.
Qualora emergano anomalie acustiche, fisiche, matematiche, prestazionali o di stabilità (es. distorsione non lineare, aliasing, intermodulazione, instabilità di filtri, code di latenza), oppure quando si deve scegliere fra alternative tecniche (§6):
- L'agente deve condurre un approfondimento scientifico rigoroso, avviando subagenti di ricerca paralleli e consultando fonti autorevoli in rete o nella letteratura DSP.
- Tutte le nuove evidenze scientifiche devono essere formalmente documentate in `docs/research/`, adattando l'analisi e l'architettura tecnica prima dell'implementazione.

## 4. Gestione delle Discrepanze e Zero Assunzioni Tacite
- Qualora emerga una discrepanza tra l'intenzione compositiva/ludica e i limiti fisici/realtime della callback, l'agente non deve applicare scorciatoie o assunzioni tacite.
- La contraddizione deve essere isolata, documentata con evidenze e sottoposta a review normativa prima di implementare il codice dipendente.

## 5. Doppia Validazione Ingegneristica e Percettiva
- **Validazione Ingegneristica (Mandatory):** suite CTest al 100%, determinismo bit-exact con isolamento dei seed, zero allocazioni dinamiche e zero lock nel callback audio, rispetto della deadline realtime ($\le 2,0\text{ ms}$ / 512 campioni Release).
- **Validazione Percettiva (CACP & Multimodale):** Il CACP esiste specificamente per essere impiegato in sinergia con le capacità multimodali dell'agente (`view_file`). Per ogni incremento che introduce o modifica comportamenti sonori, visivi o di controllo, l'agente non può limitarsi al passaggio dei test booleani: deve eseguire lo scenario diagnostico CACP, esaminare direttamente in prima persona i file binari audio (`.wav`) o i frame grafici (`.png`) tramite `view_file`, e formulare un giudizio percettivo esplicito e dettagliato prima della consegna e del playtest umano.
- **Sviluppo Parallelo Continuo e Proporzionato del CACP:** Il CACP non è un'attività differita alla chiusura finale di milestone. Per ogni incremento che introduca o modifichi comportamenti audio, grafici o di controllo, il piano e l'implementazione devono prevedere una verifica CACP proporzionata alle funzionalità introdotte, definendo scenari diagnostici mirati (`--cacp-run <scenario>`) o riutilizzando quelli esistenti quando sufficienti. Gli artefatti prodotti (manifest JSON RFC 8259 ed export `.wav` o frame `.png`) devono essere esaminati in prima persona dall'agente tramite ispezione multimodale (`view_file`), dichiarando esplicitamente la disponibilità degli strumenti nelle evidenze.

## 6. Decisioni Fondate su Evidenze (Evidence-Based Decision Record)
Si applica a ogni decisione consequenziale: scelta fra opzioni di un ADR, emendamento o precisazione di un criterio di accettazione, risoluzione di un rilievo `DIF-*` che ammette più soluzioni, tuning con effetti su contratti o gate. Le scelte di routine reversibili restano autonome (AGENTS.md §1.1), ma vanno motivate quando non sono ovvie.

1. **Formulare la domanda.** Esprimere il problema come quesiti verificabili (es. «il carico può raggiungere la soglia?», «l'effetto supera il rumore di misura?») prima di proporre soluzioni.
2. **Censire tutte le alternative realistiche**, incluse quelle «non fare nulla» ed «emendare il requisito». Nessuna alternativa va scartata senza un motivo documentato (infattibilità misurata, conflitto normativo, costo sproporzionato).
3. **Evidenze proprie misurate.** Ogni alternativa va supportata da dati prodotti sulla baseline corrente, con:
   - procedura e codice dell'esperimento;
   - ambiente (commit, configurazione, CPU, OS, compilatore, condizioni non controllate come il carico in background);
   - numero di ripetizioni e loro ordine (interlacciate, quando possibile);
   - dispersione (mediana con min–max o intervalli), non solo il valore puntuale;
   - confronto esplicito fra l'effetto misurato e il rumore di misura.
   L'analisi del codice (percorsi, rami, limiti teorici) integra le misure, non le sostituisce.
4. **Fonti esterne verificate.** Le affermazioni non dimostrate dalle misure vanno sostenute da fonti autorevoli: letteratura peer-reviewed, standard, documentazione primaria. Per ogni fonte riportare:
   - riferimento completo con DOI o link permanente;
   - data di consultazione;
   - livello di verifica effettivo (testo completo, solo abstract e metadati, segnalazione secondaria).
   È vietato citare fonti non verificate o attribuire loro affermazioni che non contengono. Quando le fonti disponibili non bastano, condurre una ricerca approfondita, anche con subagenti di ricerca paralleli, prima di raccomandare.
5. **Verifica di conformità completa.** Controllare ogni alternativa contro Specifica, DESIGN, AGENTS.md, ADR **e** le regole in `.agents/rules/`. Ogni conflitto o tensione emerso va dichiarato nel dossier, non risolto in silenzio.
6. **Matrice delle alternative e raccomandazione.** Confrontare le opzioni su criteri espliciti: conformità, fattibilità, copertura, effetto sui gate, rappresentatività, rischi e manutenzione, impatto su ABI e golden. Raccomandare un'opzione motivandola con i dati, indicando le alternative difendibili e i limiti dell'analisi.
7. **Decisione del titolare quando richiesta.** Se la scelta cambia scope, design, architettura, un criterio di accettazione o un ADR, la decisione spetta al titolare (AGENTS.md §1.1). L'agente presenta il dossier e non applica la scelta prima della decisione.
8. **Documentazione nel repository**, nella stessa modifica dell'implementazione:
   - dossier in `docs/research/<milestone>_<ID>_<tema>.md`, con stato (*proposed* / *decisa* / *applicata*), quesiti, analisi, misure, fonti, matrice, raccomandazione, limiti ed esito post-applicazione;
   - dati grezzi, sorgente dell'esperimento e manifest in `docs/research/experiments/<id>/`. Solo testo e file piccoli: niente binari pesanti né artefatti di build. Il sorgente archiviato deve indicare come riprodurlo;
   - decisione registrata nel record normativo (ADR, con sezione «Riferimenti») e in `docs/progress.md`, con chi ha deciso, la data e il collegamento al dossier;
   - indice in `docs/research/README.md`.
9. **Verifica post-applicazione.** Rieseguire i check pertinenti e registrare i risultati nel dossier e in `docs/progress.md`. Se i dati smentiscono le previsioni, riaprire la decisione invece di adattare i criteri.
10. **Nessuna evidenza dichiarata senza produrla.** Non affermare in documenti, manifest o messaggi di commit proprietà non misurate (es. «limiter in compressione», «ascolto percettivo eseguito»). Una condizione non verificata si dichiara `not_run`, `unsupported` o questione aperta.

Riferimento esemplare: [M2_4_DIF-INC4-R2-01_Limiter_GR_Analysis.md](../../docs/research/M2_4_DIF-INC4-R2-01_Limiter_GR_Analysis.md) (workload W di D-M2-10, decisione B1 del 2026-10-21).
