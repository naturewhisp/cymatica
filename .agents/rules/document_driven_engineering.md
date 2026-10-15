---
trigger: always_on
description: Direttiva trasversale su rigore tecnico-scientifico, sviluppo document-driven, tracciabilità bidirezionale e separazione epistemica.
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
Qualora emergano anomalie acustiche, fisiche, matematiche o di stabilità (es. distorsione non lineare, aliasing, intermodulazione, instabilità di filtri):
- L'agente deve condurre un approfondimento scientifico rigoroso, avviando subagenti di ricerca paralleli e consultando fonti autorevoli in rete o nella letteratura DSP.
- Tutte le nuove evidenze scientifiche devono essere formalmente documentate in `docs/research/`, adattando l'analisi e l'architettura tecnica prima dell'implementazione.

## 4. Gestione delle Discrepanze e Zero Assunzioni Tacite
- Qualora emerga una discrepanza tra l'intenzione compositiva/ludica e i limiti fisici/realtime della callback, l'agente non deve applicare scorciatoie o assunzioni tacite.
- La contraddizione deve essere isolata, documentata con evidenze e sottoposta a review normativa prima di implementare il codice dipendente.

## 5. Doppia Validazione Ingegneristica e Percettiva
- **Validazione Ingegneristica (Mandatory):** suite CTest al 100%, determinismo bit-exact con isolamento dei seed, zero allocazioni dinamiche e zero lock nel callback audio, rispetto della deadline realtime ($\le 2,0\text{ ms}$ / 512 campioni Release).
- **Validazione Percettiva (CACP & Multimodale):** Il CACP esiste specificamente per essere impiegato in sinergia con le capacità multimodali dell'agente (`view_file`). Per ogni incremento che introduce o modifica comportamenti sonori, visivi o di controllo, l'agente non può limitarsi al passaggio dei test booleani: deve eseguire lo scenario diagnostico CACP, esaminare direttamente in prima persona i file binari audio (`.wav`) o i frame grafici (`.png`) tramite `view_file`, e formulare un giudizio percettivo esplicito e dettagliato prima della consegna e del playtest umano.
- **Sviluppo Parallelo Continuo e Proporzionato del CACP:** Il CACP non è un'attività differita alla chiusura finale di milestone. Per ogni incremento che introduca o modifichi comportamenti audio, grafici o di controllo, il piano e l'implementazione devono prevedere una verifica CACP proporzionata alle funzionalità introdotte, definendo scenari diagnostici mirati (`--cacp-run <scenario>`) o riutilizzando quelli esistenti quando sufficienti. Gli artefatti prodotti (manifest JSON RFC 8259 ed export `.wav` o frame `.png`) devono essere esaminati in prima persona dall'agente tramite ispezione multimodale (`view_file`), dichiarando esplicitamente la disponibilità degli strumenti nelle evidenze.
