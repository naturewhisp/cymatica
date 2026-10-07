# CYMATICA — Specifica tecnica per sviluppo agentico e generazione adattiva

**Documento:** specifica tecnica-operativa per prototipo e sviluppo incrementale  
**Versione:** 0.8.3 — integrazione controllo agentico e osservazione audiovisiva richiesta il 2026-10-07 (baseline 0.8.1/0.8.2 accettata)
**Data:** 2026-10-07
**Baseline revisionata:** 0.8 su commit `4aa2d5271aea11ad8aa088932c5afad8d6aaeec7`, integrata in `ac1ac59`; 0.8.1 su `ac1ac59`  
**Target primario:** Windows x64  
**Target secondario da preservare:** Android  
**Stack core raccomandato:** C++20, CMake, raylib, miniaudio, shader GLSL  
**Paradigma generativo:** AI ibrida, data-driven, vincolata, riproducibile e adattiva  
**Strategia repository:** monorepo modulare, target e distribuzioni separate  
**Documento di game/visual design:** `DESIGN.md`  
**Regole operative per agenti:** `AGENTS.md`

---

## 0. Changelog

### 0.8.3 — 2026-10-07

**Stato:** principi e roadmap incrementale accepted for implementation, su richiesta del titolare; profili numerici, IPC e codec restano proposti/differiti. Revisione solo documentale, nessuna nuova capacità attestata.

- Correzioni post-review DIF-AO-DOC-01/02: nucleo dispatcher tono M2 in D-AO-01; AC-AO-01A obbligatorio distinto da AC-AO-01B supplementare dipendente dal client, stessa distinzione per video M6/M7.
- §31.5 e [analisi dedicata](docs/agent_control_observability.md): controllo esterno separato dal CIE, osservazioni correlate, tap realtime bounded, input condiviso e prove OS distinte;
- §32: estensione della baseline M1 in M2 (spazio e tono), sequenze M3, clip/replay M4, osservazione live umana M5, spike video M6 e valutazione M7, riuso Lab M8;
- DESIGN §17.4.1: valutazione player separata dalla diagnosi debug; nessuna sostituzione dei gate umani;
- M1 storica resta conclusa secondo il record esistente; nuove estensioni non eseguite, nessun avanzamento di milestone.

### 0.8.2 — 2026-10-06

**Stato:** accettata; le aggiunte sono additive e si applicano da M6. Nessun impatto su M0–M5 (fino a M5 il retriever restituisce tutti i candidati).

- §8.2–§8.3, §11.4, nuova §11.7: System 1 / Fast Candidate Retrieval deterministico (descriptor interi, distanza pesata, tie-break stabile) separato dal System 2 (planner validato). System 1 non decide mai la sicurezza;
- §18.8, §19.2, §26.1: uso online dell'archive tramite retriever, retriever appreso solo post-M8 se batte la baseline su Recall@K, ricerca semantica BGE/MiniLM/multilingual ammessa solo nel tooling;
- §32 M6–M8, §34, §36: deliverable, criteri, decisioni e open question 18–20;
- §32 M3 e M7: gate di identità visiva introdotti da `DESIGN.md` 0.3 (tavola delle invarianti prima di M3, test di luminanza e riconoscibilità in M7).

### 0.8.1 — 2026-10-05

**Stato:** accettata per l'implementazione dal titolare il 2026-10-05. Requisiti, contratti e criteri di accettazione sono vincolanti per M0 e successive. Restano *proposti*, non validati: valori di tuning, budget numerici, esempi C++/JSON e le open question di §36 e DESIGN §24, che si risolvono alla milestone indicata.

Correzioni di coerenza documentale prima dell'avvio di M0; nessun cambio di stack, architettura o numerazione milestone. Nessuna build o test esiste ancora.

- §20.4–§20.5: il worker restituisce proposte; l'unico producer della SPSC audio è il coordinatore (allineato a §8.6 e §20.7);
- §20.3: gli eventi gameplay seguono il transport canonico di §7.5, non il conteggio della callback;
- §22.1: rimosso `paused` dal frame latest-value (pause/resume sono comandi con acknowledgement, §20.7); la telemetria espone cursori e qualità della stima di presentazione (§7.5);
- §15.5: `decision_id` usa il mixer stabile versionato, non `std::hash`;
- §21: aggiunti `docs/progress.md` e `docs/adr/` richiesti da `AGENTS.md`;
- §32 M0: resi espliciti deliverable e criteri già richiesti da `AGENTS.md` (record di avanzamento, almeno un test eseguito da CTest, unico owner audio verificato, toolchain registrata). È un allineamento, non un allentamento: nessun criterio precedente è stato rimosso.

### 0.8 — 2026-09-23

Revisione documentale proposta, non attestazione di implementazione. Il repository esaminato contiene documentazione, non un runtime compilabile. La milestone attiva resta M0.

- precisati transport, sample clock, latenza, fixed tick e gestione overload;
- corretti snapshot concorrenti, lifetime e commit transazionale dei piani;
- separati determinismo logico, deadline realtime e replay;
- rafforzata la validazione spazio-temporale e composizionale, con esito unknown;
- formalizzate eccezioni di sicurezza, fallback e telegraph;
- resi espliciti i limiti scientifici di proxy, cimatica e playtest automatico;
- introdotti gate anticipati per regole player, metriche e profilo hardware;
- mantenuti stack, milestone, archetipi e ambito del progetto.

Le integrazioni sono requisiti proposti da adottare con questa revisione. Numeri di tuning e alternative creative aperte restano da validare; non diventano risultati sperimentali per il fatto di essere documentati. Gli esempi C++/JSON sono contratti illustrativi, non sorgenti o schemi già implementati.

### 0.7 — 2026-09-09

Riscrittura sostanziale orientata al motore di intelligenza generativa e al modello non deterministico di CYMATICA:

- introdotto il **Cymatic Intelligence Engine**, o **CIE**, come sottosistema esplicito e separato dal renderer, dal DSP e dalla simulazione;
- definita un’architettura ibrida: regole, utility scoring, ricerca vincolata, player modeling e quality-diversity offline; i modelli ML restano opzionali e sostituibili;
- sostituito il concetto di “livello generato” con quello di **politica generativa eseguita su orizzonte mobile**;
- introdotti planning horizon, commit horizon, decision points musicali, fallback sicuri e runtime safety guard;
- definita la riproducibilità del non determinismo tramite seed gerarchici, stream indipendenti, decision trace e versionamento delle policy;
- dettagliato il player model multidimensionale e la dynamic difficulty con isteresi, limiti di variazione e confidence score;
- introdotto un catalogo di pattern data-driven con grammatica, trasformazioni, cost model, telegraph contract e hard constraints;
- definita una validazione conservativa di raggiungibilità per evitare configurazioni inevitabili o unfair;
- introdotto un laboratorio offline con MAP-Elites/quality-diversity e procedural personas per generare e validare una libreria di pattern diversificata;
- separata la specifica tecnica dal design creativo: la visione ludica, visuale, sonora e UX è ora formalizzata in `DESIGN.md`;
- riorganizzati moduli C++, contratti dati, test, metriche, roadmap e criteri di accettazione attorno al motore generativo.

### 0.6 — 2026-07-10

- separati i flussi `audio → game/render` e `game → audio`;
- introdotti fixed timestep, clock audio autoritativo e protocollo di scambio bounded;
- riallineate le milestone NatuStem/stem separation;
- corrette ownership, replay e determinismo temporale.

### 0.5 e precedenti

Le revisioni 0.2–0.5 hanno consolidato stack nativo, monorepo, dipendenze, particelle custom, NatuStem come riferimento, Strudel come research track, pacchetti `.cymlevel`, `AGENTS.md` e roadmap iniziale. Per il dettaglio storico fare riferimento alla cronologia Git.

---

## 1. Scopo del documento

Questo documento descrive **come costruire CYMATICA**. Definisce:

- architettura software;
- motore di generazione procedurale e adattiva;
- contratti tra audio, AI, gameplay e rendering;
- modello di non determinismo controllato;
- fairness e validazione;
- formato dei contenuti;
- dipendenze e ambiente di sviluppo;
- strategia di test;
- roadmap implementativa.

Non è la fonte primaria per tono artistico, palette, motion language, UX, archetipi o sensazione di gioco: tali aspetti sono normativi in `DESIGN.md`. Quando una decisione tecnica contraddice una regola di design, il conflitto deve essere segnalato e risolto esplicitamente; non va nascosto nell’implementazione.

---

## 2. Executive summary

CYMATICA è un musical bullet hell nel quale musica, geometria, pericolo e colore sono prodotti da uno stesso sistema generativo. Il runtime non deve limitarsi a sincronizzare ostacoli con un file audio: deve costruire in tempo reale una **forma musicale**, una **curva di tensione** e una **sequenza di situazioni giocabili**, adattandole alla performance del giocatore senza perdere identità, fairness o coerenza sonora.

La scelta architetturale principale è un motore AI ibrido, non un modello neurale monolitico.

```text
Musical Form + Player Model + Archetype Policy
                    |
                    v
          Cymatic Intelligence Engine
                    |
        +-----------+------------+
        |                        |
        v                        v
  Music Intent Plan       Gameplay Pattern Plan
        |                        |
        v                        v
  Audio Scheduler          Level Scheduler
        |                        |
        +-----------+------------+
                    v
          Shared Musical Clock
```

Il CIE deve:

1. osservare musica, stato del giocatore e storico della run;
2. definire il target di esperienza per i prossimi beat, battute e frasi;
3. generare più candidati di livello;
4. eliminare quelli che violano vincoli hard;
5. classificare i restanti con utility score e novelty score;
6. impegnare soltanto un breve tratto futuro;
7. mantenere modificabile il resto dell’orizzonte;
8. degradare verso pattern sicuri se il planner non produce una risposta in tempo;
9. registrare seed e decisioni per riprodurre la run.

L’esperienza è quindi **non deterministica per il giocatore**, ma deve essere **riproducibile per sviluppo, test e bug report**.

Per la vertical slice non è richiesto un modello ML. L’intelligenza iniziale deriva da:

- modello gerarchico del tempo musicale;
- utility AI;
- pattern grammar;
- generazione costruttiva stocastica;
- ricerca bounded su candidati;
- player model filtrato;
- validator di raggiungibilità;
- memoria di novità;
- policy di pacing.

Machine learning, reinforcement learning e ONNX possono essere aggiunti in seguito solo dietro interfacce stabili e con fallback deterministico.

---

## 3. Valutazione della revisione 0.6

### 3.1 Elementi confermati

La revisione precedente contiene decisioni solide che restano valide:

- procedurale infinito prima di musica custom e stem separation;
- C++20/CMake/raylib/miniaudio per il core;
- Windows come piattaforma di validazione;
- Android preservato ma differito;
- audio callback priva di allocazioni e blocchi;
- fixed timestep indipendente dal rendering;
- clock audio come autorità degli eventi musicali;
- separazione tra bullet, particelle e shader;
- niente motore fisico esterno nel core;
- tool CLI e runtime come target separati;
- NatuStem come riferimento, non dipendenza;
- Strudel/Tidal come riferimento grammaticale, non runtime iniziale;
- `.cymlevel` ispezionabile e versionato.

### 3.2 Lacuna principale

La versione 0.6 descrive correttamente audio procedurale, mapping cimatica, archetipi e threading, ma non formalizza abbastanza il componente che prende decisioni. Mancano in particolare:

- uno stato interno del director;
- una rappresentazione esplicita dell’intento musicale;
- un modello del giocatore con confidence;
- una grammatica dei pattern;
- un meccanismo di selezione tra candidati;
- hard constraints verificabili;
- un planning horizon distinto dal commit horizon;
- una strategia di fallback temporizzata;
- una politica di novità e memoria;
- un sistema riproducibile di random stream;
- metriche per stabilire se il generatore sta funzionando.

Senza questi elementi, il rischio è produrre un insieme di trigger audio-visivi reattivi ma non un vero livello dinamico: musica e arena “si muovono”, tuttavia non costruiscono intenzionalmente tensione, variazione, recupero e climax.

### 3.3 Correzione concettuale

In CYMATICA un livello non è primariamente una lista di spawn. È una politica che, dato uno stato, produce il tratto successivo dell’esperienza.

```text
next_plan = policy(
    musical_context,
    player_model,
    recent_history,
    archetype_state,
    safety_constraints,
    seed_key
)
```

La timeline rimane utile come output, replay, contenuto finito o pacchetto custom, ma nella modalità infinita è una **timeline prodotta incrementalmente**.

---

## 4. Principi architetturali non negoziabili

### 4.1 Fairness prima della sorpresa

Il sistema può sorprendere il giocatore, non può generare pericoli inevitabili, invisibili o incoerenti con gli strumenti disponibili. Ogni candidato deve superare vincoli hard prima di essere valutato esteticamente.

### 4.2 Un’unica autorità temporale musicale

Il transport musicale canonico, espresso in sample frame logici, determina beat, battute, frasi e scadenze. Il conteggio dei frame prodotti dalla callback non coincide con il tempo già udito. La mappatura fra transport, device e presentazione è definita in §7.5; gameplay e rendering non inferiscono il tempo musicale dal frame rate.

### 4.3 Decisioni ai confini musicali

La maggior parte delle decisioni AI avviene su boundary musicali, non ogni frame:

- micro: sottodivisione/beat;
- meso: battuta;
- macro: frase/sezione.

Correzioni di sicurezza possono avvenire immediatamente secondo §12.5. La sicurezza ha precedenza sulla quantizzazione; quando possibile il feedback musicale viene risolto sul boundary successivo.

### 4.4 Non determinismo isolato

Ogni sottosistema possiede uno stream casuale indipendente. Cambiare particelle o camera shake non deve cambiare i pattern di gameplay.

### 4.5 AI fuori dalla callback audio

Nessuna ricerca, inferenza, allocazione, parsing, logging o validazione complessa deve avvenire nella callback miniaudio. L’audio callback esegue soltanto lavoro bounded e preallocato.

### 4.6 Degradazione sicura

Un planner lento, una policy corrotta o un archivio mancante non devono bloccare la callback né creare caos; un guasto grave può richiedere una sospensione tecnica controllata. Il runtime deve possedere pattern fallback semplici, musicali e sicuri.

### 4.7 Dati prima di scripting arbitrario

Pattern, archetipi e policy sono descritti tramite dati versionati e validati. Nella prima fase non viene eseguito scripting utente nel runtime.

### 4.8 Osservabilità nativa

Ogni decisione rilevante deve essere spiegabile in debug:

- candidati considerati;
- vincoli falliti;
- score;
- seed/key;
- motivo della scelta;
- stato del player model;
- deadline del planner.

---

## 5. Glossario operativo

| Termine | Definizione |
|---|---|
| CIE | Cymatic Intelligence Engine, motore decisionale generativo |
| Director | componente che decide pacing e target di esperienza |
| Music Intent | rappresentazione simbolica futura della musica, prima del rendering audio |
| Pattern | unità parametrica di minaccia/struttura/telegraph |
| Candidate | istanza di pattern proposta ma non ancora impegnata |
| Plan Chunk | segmento futuro validato e schedulabile |
| Planning horizon | finestra futura considerata dal planner |
| Commit horizon | parte del piano non più modificabile senza violare coerenza |
| Decision point | boundary musicale in cui il director può scegliere |
| Hard constraint | regola che rende un candidato valido o invalido |
| Utility score | misura relativa della bontà di un candidato valido |
| Novelty budget | quantità di variazione concessa in una finestra temporale |
| Player model | stima filtrata delle capacità e dello stato del giocatore |
| Experience target | vettore della pressione desiderata per il prossimo tratto |
| Safety guard | controllo runtime conservativo sull’immediato futuro |
| Decision trace | registro delle decisioni necessario per replay e diagnosi |
| Policy version | identificatore immutabile della logica/configurazione generativa |
| Semantic replay | rigenerazione tramite seed e policy compatibile |
| Exact replay | riproduzione tramite input e decision trace già materializzato |

---

## 6. Modalità di gioco e substrato comune

### 6.1 Infinite Resonance

È la modalità prioritaria. Musica e livello sono pianificati a finestre mobili. Il CIE può adattare le parti non ancora impegnate sulla base della run.

Caratteristiche:

- durata indefinita;
- macroforma ciclica ma non ripetitiva;
- adattamento progressivo;
- archetipi selezionabili o mescolabili;
- nessuna dipendenza dal tool offline;
- replay tramite seed, policy version e trace.

### 6.2 Generated Track

Il motore usa la stessa pipeline, ma produce una forma chiusa di durata scelta. Tutta la timeline può essere generata e validata prima del play.

Vantaggi:

- lookahead completo;
- playtest automatico più profondo;
- possibilità di ranking e rigenerazione;
- esportazione in `.cymlevel`;
- condivisione di seed o pacchetto.

### 6.3 Custom Track

Il tool offline produce un `MusicIntentTimeline` a partire da mix e, opzionalmente, stem. Il CIE non deve dipendere da stem perfetti: usa confidence e fallback.

### 6.4 Substrato comune

Le tre modalità convergono sugli stessi contratti, ma l’ownership cambia in base alla sorgente.

**Procedurale live/generated:**

```text
Form Policy + Player Model + Archetype Policy
                     |
                     v
                    CIE
              +------+------+
              v             v
       MusicIntent      PlanChunk
              |             |
              +------+------+ 
                     v
             Runtime Schedulers
```

**Custom track:**

```text
Precomputed MusicIntentTimeline
              |
              v
        CIE + Pattern Catalog
              |
              v
          PlanChunk stream
              |
              v
        Runtime Scheduler
```

Nella modalità procedurale il CIE orchestra la pianificazione congiunta e produce un Music Intent canonico prima di audio e gameplay. Nella modalità custom riceve invece un intento estratto dalla traccia. In nessun caso il sistema deve creare un ciclo nel quale il director reagisce continuamente all’audio già renderizzato per ricostruire decisioni che conosceva in origine.

---

## 7. Modello musicale gerarchico

### 7.1 Quattro canali funzionali

Il sistema conserva i canali già definiti:

| Canale | Funzione musicale | Funzione di gameplay |
|---|---|---|
| Pulso (`Pulse`) | ritmo, transienti, accenti | trigger, onde, emissioni, telegraph |
| Corpo (`Body`) | basso, gravità, fondazione | geometria nodale, muri, spazio |
| Trama (`Texture`) | armonia, densità, riempimento | micro-pattern, particelle, pressione diffusa |
| Vettore (`Vector`) | lead, frase espressiva, direzione | inseguitori, boss-like actor, attacchi mirati |

Il canale Vettore non coincide necessariamente con la voce. Può essere un lead strumentale o una componente saliente stimata.

### 7.2 Livelli temporali

```text
sample -> audio block -> subdivision -> beat -> bar -> phrase -> section -> run form
```

Ogni livello ha responsabilità diverse:

- sample/audio block: DSP;
- subdivision/beat: eventi ritmici e telegraph;
- bar: selezione pattern e trasformazioni;
- phrase: pacing, difficoltà, memoria di novità;
- section: archetype blend, tonalità, orchestrazione;
- run form: progressione, climax, recupero e meta-obiettivi.

### 7.3 Music Intent, non analisi postuma

Nella musica procedurale, audio e gameplay devono derivare dallo stesso intento simbolico. Il gioco non deve sintetizzare una cassa e poi eseguire FFT per scoprire che esiste una cassa.

```text
MusicIntentEvent
  -> AudioEvent
  -> GameplayIntentEvent
  -> VisualIntentEvent
```

L’analisi del segnale resta utile per feedback, custom tracks, verifica o modulazioni timbriche, ma non è la fonte primaria della struttura quando il sistema conosce già la composizione.

### 7.4 Rappresentazione minima

```cpp
struct MusicPosition {
    std::uint64_t sampleFrame;
    std::uint64_t subdivisionIndex;
    std::uint64_t beatIndex;
    std::uint64_t barIndex;
    std::uint64_t phraseIndex;
    float beatPhase; // [0, 1)
};

enum class MusicalRole : std::uint8_t {
    Pulse,
    Body,
    Texture,
    Vector
};

struct MusicIntentEvent {
    MusicPosition at;
    MusicalRole role;
    std::uint32_t motifId;
    float energy;       // [0, 1]
    float tension;      // [0, 1]
    float density;      // [0, 1]
    float salience;     // [0, 1]
    float pitchHz;      // 0 se non applicabile
    std::uint32_t flags;
};
```


---

### 7.5 Transport, unità e sincronizzazione

Contratto richiesto entro M1:

- `sampleFrame` è un frame multicanale (non un singolo campione per canale), unsigned 64 bit, sulla timeline logica della run. Per il primo profilo proporre 48.000 frame/s interni; il device può avere un rate diverso tramite conversione esplicita. La scelta va registrata, non ereditata implicitamente dal device.
- Distinguere `renderCursor` (primo frame ancora da sintetizzare), `presentationCursor` (stima del frame in uscita udibile) e `simulationTick`. Pubblicare qualità/invalidità della stima di presentazione: non fingere precisione sample-accurate sul tempo udito quando il backend non la offre.
- Mantenere una sola tempo map versionata. BPM indica quarter-note/minute; numeratore, denominatore, suddivisioni e confini di frase sono espliciti. Nel primo prototipo è ammesso limitarsi a 4/4 e tempo costante, rifiutando altri casi anziché interpretarli male.
- Convertire posizioni musicali in frame assoluti con aritmetica razionale o accumulo del resto e arrotondamento documentato; non sommare ripetutamente durate float arrotondate. `beatPhase` è una vista derivata, mai autorità.
- Per eventi alla stessa data usare ordine stabile `(sampleFrame, eventPriority, eventId)`. Specificare la priorità di note-off, note-on e controlli e congelarla nella versione del scheduler.
- Il frame limite del tick k è `floor(k * internalSampleRate / simulationHz)` con aritmetica controllata contro overflow. Un evento gameplay entra nel primo tick il cui limite è maggiore o uguale alla sua data. La callback audio lo applica invece all'offset esatto nel blocco, anche se il blocco attraversa più eventi.
- Input acquisiti con timestamp monotono vengono assegnati a un tick tramite la mappatura della run e registrati; replay usa i tick registrati. La valutazione ritmica può usare il timestamp sub-tick, mentre la collisione resta fixed-step.
- Latenza audio, input e display richiedono misure separate. La calibrazione modifica la mappatura percettiva/valutazione ritmica, non sposta silenziosamente hitbox o seed. Registrare offset e profilo di timing nel RunRecord.
- Pause, cambio device e discontinuità incrementano `transportEpoch`; i frame logici della run non vengono azzerati a ogni riapertura. Un piano di epoch precedente è rifiutato. In headless un clock virtuale esegue la stessa timeline.

Test M1: 44,1/48 kHz lato device, blocchi di dimensione variabile, eventi sul bordo del blocco, BPM non divisore del rate, pausa/ripresa e nessuna deriva cumulativa dopo un'ora virtuale. Le configurazioni non supportate devono fallire esplicitamente.

## 8. Cymatic Intelligence Engine

### 8.1 Responsabilità

Il CIE è il motore che trasforma contesto musicale, stato del giocatore e memoria della run in un piano futuro validato.

Non è responsabile di:

- produrre direttamente campioni audio;
- disegnare shader;
- risolvere collisioni frame per frame;
- leggere file o modelli nella callback audio;
- modificare retroattivamente eventi già impegnati;
- garantire da solo il frame rate.

È responsabile di:

- pacing;
- esperienza target;
- generazione di candidati;
- selezione di pattern;
- controllo di varietà;
- adattamento della difficoltà;
- validazione preventiva;
- pianificazione quantizzata;
- spiegabilità e decision trace.

### 8.2 Architettura logica

```text
+-----------------------+       +-----------------------+
| Music/Form State      |       | Player Model          |
| beat/bar/phrase       |       | skill/stress/style    |
+-----------+-----------+       +-----------+-----------+
            |                               |
            +---------------+---------------+
                            v
                 +------------------------+
                 | Experience Target      |
                 | pressure/novelty/load  |
                 +-----------+------------+
                             v
                 +------------------------+
                 | Pacing Director        |
                 | macro/meso decisions   |
                 +-----------+------------+
                             v
                 +------------------------+
                 | Candidate Retriever    |
                 | System 1, top-K (M6+)  |
                 +-----------+------------+
                             v
                 +------------------------+
                 | Candidate Generator    |
                 | grammar + RNG streams  |
                 +-----------+------------+
                             v
                 +------------------------+
                 | Hard Constraint Gate   |
                 | fairness/reachability  |
                 +-----------+------------+
                             v
                 +------------------------+
                 | Scorer + Selector      |
                 | fit/novelty/cost       |
                 +-----------+------------+
                             v
                 +------------------------+
                 | Receding-Horizon Plan  |
                 | commit + mutable tail  |
                 +-----------+------------+
                             v
                 +------------------------+
                 | Runtime Safety Guard   |
                 +-----------+------------+
                             v
                       Schedulers
```

### 8.3 Layer del motore

Il CIE è diviso in cinque layer, ciascuno sostituibile e testabile.

#### Layer A — Perception and state

Costruisce snapshot immutabili di:

- MusicState;
- PlayerModel;
- RunHistory;
- WorldState;
- ResourceBudget;
- active policy/version.

#### Layer B — Experience management

Decide che tipo di esperienza produrre nel prossimo orizzonte:

- pressione;
- precisione richiesta;
- mobilità;
- densità cognitiva;
- rischio/rendimento;
- novità;
- recupero;
- intensità estetica.

#### Layer C — Content generation

Istanzia pattern e trasformazioni compatibili con:

- archetipo;
- sezione musicale;
- target di esperienza;
- stato corrente;
- capacità del giocatore;
- budget CPU/GPU/entity.

Comprende il **Candidate Retriever (System 1, §11.7)**: preselezione deterministica e veloce dei candidati da esaminare. System 1 non decide mai la sicurezza di un pattern.

#### Layer D — Validation and selection

Applica hard constraints e valuta i candidati validi. È il **System 2** (§11.7): ricerca limitata, validator autoritativo, scoring e commit.

#### Layer E — Scheduling and observability

Impegna una parte del piano, pubblica eventi, registra le decisioni e gestisce i fallback.

### 8.4 Snapshot di input

Il planner non legge oggetti mutabili sparsi nel runtime. Riceve un unico snapshot coerente:

```cpp
struct DirectorInput {
    MusicState music;
    PlayerModelSnapshot player;
    WorldSnapshot world;
    RunHistorySummary history;
    ExperiencePolicy policy;
    ResourceBudget budget;
    SeedContext seed;
    DirectorDeadline deadline;
};
```

Requisiti:

- trivially copyable o composto da handle immutabili;
- nessun puntatore a oggetti con lifetime ambiguo;
- dimensione bounded;
- schema versionato per trace e test;
- generato sul game thread e consegnato al planning worker.

### 8.5 Output del planner

```cpp
struct PlanChunk {
    PlanChunkId id;
    std::uint64_t transportEpoch;
    std::uint64_t basePlanRevision;
    std::uint64_t snapshotId;
    std::uint64_t decisionId;
    PolicyVersion policyVersion;
    MusicPosition begin;
    MusicPosition commitUntil;
    MusicPosition plannedUntil;
    ExperienceTarget target;
    std::vector<PlannedEvent> events; // allocata fuori dai path realtime
    ValidationSummary validation;
    SelectionExplanation explanation;
    std::uint64_t contentHash;
};
```

Nel runtime la rappresentazione può essere convertita in buffer preallocati prima della pubblicazione. `std::vector` è ammesso nel worker/planner, non nella callback audio.

### 8.6 Ciclo decisionale di riferimento

Il worker costruisce una proposta da snapshot immutabile e stato accettato. Non avanza lo stato canonico quando termina un calcolo: una proposta può essere scaduta o sostituita.

```text
worker:
  derive nextState and target from acceptedState + snapshot
  generate candidates in stable order within logical work budget
  validate composed future, score accepted candidates, select
  return PlanProposal(chunk, nextState, snapshotId, basePlanRevision,
                      transportEpoch, decisionId, validityWindow)

game/coordinator at decision boundary:
  check epoch, revision, deadline, state compatibility and queue capacity
  if valid: commit proposal and nextState together
  else: commit contextual fallback and its explicit state transition
  publish accepted event buffers once; record actual accepted decision
```

Il commit ha un solo proprietario sul game/coordinator thread. I risultati tardivi vengono scartati, non inseriti nel futuro come se appartenessero a una nuova decisione. Una proposta rifiutata non consuma novelty debt, recovery debt o ID canonici. Riservare prima la capacità dei buffer audio/gameplay e pubblicare un token/revisione di commit comune; nessun consumer esegue metà transazione. La callback non attende il coordinatore.

Gli oggetti dinamici del worker sono convertiti in slot preallocati prima del commit. Il rilascio di vector, shared ownership e buffer pesanti avviene fuori dalla callback. Trace delle proposte e trace dei commit sono distinti: il secondo è autoritativo per replay.

### 8.7 Stato persistente del director

Lo stato è piccolo, versionato e serializzabile:

```cpp
struct DirectorState {
    PacingState pacing;
    ExperienceTarget previousTarget;
    float recoveryDebt;
    float noveltyDebt;
    std::uint64_t nextDecisionId;
    PatternHistoryWindow recentPatterns;
    ArchetypeBlend activeBlend;
    std::uint32_t consecutiveFallbacks;
};
```

Non conservare nel director puntatori alle entità runtime. Lo stato deve poter essere incluso in un replay checkpoint e confrontato nei test.

---

## 9. Pacing Director

### 9.1 Stato di pacing

Il director usa una macchina a stati gerarchica guidata da forma musicale e performance.

Stati macro raccomandati:

```text
INTRO
EXPOSITION
BUILD
PRESSURE
DROP
RECOVERY
VARIATION
CLIMAX
RESOLUTION
```

Nella modalità infinita, `RESOLUTION` può condurre a una nuova `EXPOSITION` con trasformazione di tonalità, archetipo o densità.

### 9.2 Regole di transizione

Le transizioni normali avvengono su frase o battuta. Un colpo subito dal giocatore non deve causare un cambio istantaneo e percepibile come rubber-banding.

Esempio:

```text
BUILD -> PRESSURE
quando:
- phrase boundary;
- energia musicale prevista > soglia;
- player confidence sufficiente;
- nessun recovery debt attivo.

PRESSURE -> RECOVERY
quando:
- pressione cumulativa supera target;
- dissonanza cresce rapidamente;
- safety margin si riduce;
- sezione musicale consente una distensione.
```

### 9.3 Recovery debt

Ogni tratto ad alta pressione accumula un debito di recupero. Il director deve ripagarlo con spazio, densità o velocità ridotte entro un limite di frasi.

```cpp
recoveryDebt += pressureAboveBaseline * duration;
recoveryDebt -= recoveryStrength * duration;
```

Il debito evita sequenze casuali di picchi consecutivi e rende la curva più musicale.

### 9.4 Tension curve

Il director mantiene due valori distinti:

- `musicalTension`: richiesta dalla composizione;
- `gameplayPressure`: pressione effettiva stimata.

Non devono coincidere sempre. Un crescendo può aumentare spettacolo e densità visuale senza rendere subito più strette le hitbox. La differenza consente anticipazione e respirazione.

### 9.5 Experience target

```cpp
struct ExperienceTarget {
    float pressure;          // esposizione a pericoli
    float precisionDemand;   // accuratezza movimento richiesta
    float mobilityDemand;    // distanza/ritmo degli spostamenti
    float rhythmDemand;      // importanza del timing musicale
    float cognitiveLoad;     // numero di flussi/regole simultanei
    float grazeOpportunity;  // opportunità di rischio volontario
    float novelty;           // distanza dal recente
    float spectacle;         // intensità VFX non letale
    float recovery;          // spazio e stabilità desiderati
};
```

Il target è un vettore. Ridurre la difficoltà non significa soltanto rallentare i proiettili: può significare mantenere velocità ma ridurre simultaneità, aumentare telegraph o ampliare corridoi.

---

## 10. Player model e adattamento

### 10.1 Perché non basta un singolo livello di abilità

Un giocatore può essere bravo nei micro-movimenti ma scarso nel dash, oppure aggressivo nel graze ma vulnerabile alla poliritmia. Il modello deve essere multidimensionale.

```cpp
struct PlayerSkillVector {
    float movementPrecision;
    float spatialPlanning;
    float dashExecution;
    float rhythmicTiming;
    float hazardTracking;
    float recoveryControl;
    float grazeControl;
};

struct PlayerStateEstimate {
    PlayerSkillVector skill;
    float stressProxy;  // euristica, non emozione misurata
    float flowProxy;    // sperimentale, non controllo safety
    float fatigueProxy;
    float confidence;
    float adaptationReadiness;
};
```

### 10.2 Osservazioni

Il sistema può derivare segnali locali, senza telemetria remota obbligatoria:

- collisioni per minuto;
- near miss e graze;
- distanza media dalle minacce;
- errori di dash;
- destinazioni corrette dal safe landing;
- tempo in zone a bassa mobilità;
- input reversal rate;
- frequenza di movimento;
- tempo di reazione dopo telegraph;
- uso eccessivo o mancato del Drop Shock;
- variazione della Dissonanza;
- sopravvivenza per pattern family;
- accuratezza rispetto a beat/off-beat;
- abbandono o restart volontario.

Questi segnali sono proxy e non vanno interpretati come emozioni certe.

### 10.3 Filtraggio e confidence

Non adattare su un singolo errore. Ogni metrica usa:

- EWMA o filtro equivalente;
- finestra minima di campioni;
- confidence crescente con osservazioni valide;
- decadimento nel tempo;
- separazione per pattern family/archetipo.

Esempio:

```cpp
estimate = lerp(estimate, observation, alpha);
confidence = evidenceQuality(validCount, recency, coverage); // euristica versionata
```

### 10.4 Challenge corridor

Ogni preset di difficoltà definisce un intervallo desiderato, non un valore assoluto.

```text
too low pressure -> boredom risk
inside corridor  -> flow target
too high pressure -> frustration risk
```

Il director adatta il livello per restare nel corridoio, ma con variazioni intenzionali: un climax può superarlo brevemente; un recovery può stare sotto.

### 10.5 Limiti dell’adattamento

Regole obbligatorie:

- variazione massima per frase;
- isteresi per evitare oscillazioni;
- cooldown dopo un cambio importante;
- nessun cambio retroattivo;
- nessuna modifica nascosta a hitbox o invulnerabilità;
- nessun annullamento adattivo di un attacco già telegrafato; le sole eccezioni tecniche di sicurezza seguono §12.5;
- ogni assist deve appartenere a una policy dichiarata;
- adattamento disattivabile in modalità challenge/seeded leaderboard.

### 10.6 Preset

| Preset | Corridoio | Adattamento | Note |
|---|---|---|---|
| Assistito | largo, pressione bassa | forte ma graduale | telegraph esteso, recovery frequente |
| Standard | medio | moderato | esperienza prevista principale |
| Intenso | alto | limitato | maggiore simultaneità |
| Pure Seed | fisso | disattivato | confrontabilità e replay |
| Training | mirato | orientato a una skill | pattern family selettive |

### 10.7 Privacy

Il player model è locale per default. Qualunque invio di telemetria richiede consenso, documentazione, minimizzazione e separazione da dati identificativi.

### 10.8 Stima della pressione osservata

Per la prima implementazione, calcolare una misura esplicita e ispezionabile:

```text
observedPressure =
    w_exposure    * timeToCollisionExposure
  + w_compression * spatialCompression
  + w_actions     * requiredActionRate
  + w_tracking    * simultaneousFlowCount
  + w_precision   * corridorPrecision
  + w_failure     * recentFailureSignal
```

Dove:

- `timeToCollisionExposure` cresce quando più hazard hanno TTC breve;
- `spatialCompression` misura la riduzione dell’area raggiungibile;
- `requiredActionRate` stima cambi direzione, dash e finestre per secondo;
- `simultaneousFlowCount` misura il carico di tracking;
- `corridorPrecision` misura margini rispetto a hitbox/velocità;
- `recentFailureSignal` usa collisioni e crescita Dissonanza filtrate.

Pesi e normalizzazioni sono policy data-driven. Registrare breakdown e non usare il solo numero finale per debug.

Ogni feature deve dichiarare unità grezza, finestra, dominio, funzione di normalizzazione, saturazione e comportamento con dati mancanti. Pesi non negativi normalizzati a somma 1 mantengono la combinazione in [0,1] se tutte le feature lo sono. Un dato mancante non equivale a pressione zero: ridurre confidence o usare baseline.

Separare `environmentPressure` (geometria/richiesta d'azione) e `performanceError` (collisioni e difficoltà osservate). Il primo serve a confrontare candidati, il secondo a correggere gradualmente il target; evitare doppio conteggio di feature correlate. Nel prototipo confidence significa sufficienza dell'evidenza, non probabilità calibrata. Validare i proxy con playtest e non inferire stress/flow reali senza dati specifici.

### 10.9 Controller adattivo iniziale

Separare `baseTarget` della forma musicale, `desiredPressure` del preset e `commandedTarget` inviato al generatore. Non aggiornare un target inseguendo continuamente sé stesso.

Baseline M6: controllo proporzionale limitato, `Ki = 0`, aggiornato ai confini di frase su una finestra completata. L'integrale è un esperimento successivo, non un requisito iniziale.

```text
error = desiredPressure - measuredPressure
delta = confidence * Kp * deadzone(error)
commandedTarget = clamp(previousCommand + clamp(delta, -maxDelta, +maxDelta),
                        presetMin, presetMax)
```

Questa è una policy euristica: misurare segno, ritardo e risposta fra target e pressione prima di attivarla. Recovery e climax applicano obiettivi espliciti e possono sospendere l'adattamento. Scarsa confidence congela la correzione e riporta gradualmente alla baseline dichiarata.

Un futuro PI deve includere `dt` della finestra, anti-windup ai limiti, reset/freeze in pausa, fallback, cambio preset e dati invalidi. Il recovery debt resta non negativo, bounded, con unità pressione×secondi e termine massimo di rimborso configurato. Saturazione, novelty e controller non possono aumentare difficoltà tramite percorsi indipendenti che aggirano il clamp finale.

Accettazione: fixture con pressione a gradino, rumore, dati mancanti, saturazione e ritardo; nessuna oscillazione persistente o deriva fuori preset. Soglie di convergenza e numero di frasi vanno fissati nella policy prima del test; il prototipo non promette stabilità sulla base della sola formula.

### 10.10 Confidence gating

```text
if playerModel.confidence < minimum:
    use preset baseline
elif adaptationReadiness is low:
    limit delta
else:
    apply bounded adjustment
```

Questo evita che i primi secondi della run determinino una classificazione permanente.

---

## 11. Generazione dei candidati

### 11.1 Pattern come unità semantica

Un pattern non è una routine di spawn opaca. È una risorsa con metadati, parametri, vincoli e descrittori.

Esempio concettuale:

```json
{
  "schema_version": 1,
  "id": "synthetic.crossfire.v1",
  "generator": "cartesian_crossfire",
  "roles": ["pressure", "rhythmic_precision"],
  "archetypes": ["synthetic"],
  "duration_beats": { "min": 2, "max": 8 },
  "telegraph_beats": 0.5,
  "parameters": {
    "lane_count": { "type": "int", "min": 3, "max": 9 },
    "speed": { "type": "float", "min": 0.15, "max": 0.75 },
    "gap_width": { "type": "float", "min": 0.08, "max": 0.35 }
  },
  "hard_limits": {
    "min_safe_area": 0.10,
    "max_simultaneous_hazards": 256,
    "requires_dash": false
  },
  "descriptors": {
    "symmetry": 0.95,
    "curvature": 0.0,
    "rhythm_complexity": 0.35
  }
}
```

### 11.2 Registro nativo dei generatori

Il campo `generator` risolve una funzione C++ registrata in modo esplicito:

```cpp
using PatternGeneratorFn = GenerateResult(*)(
    const PatternDefinition&,
    const PatternParameters&,
    const GenerationContext&,
    EventBuffer&
);
```

Nella prima fase non usare reflection complessa o plugin binari. Il registro deve fallire chiaramente se l’identificatore è sconosciuto.

### 11.3 Grammatica dei pattern

La grammatica compone:

```text
motif + emitter topology + temporal pattern + spatial transform + modulation
```

Esempi:

- motif: pulse burst, rotating wall, pursuit curve;
- topology: radial, cartesian, nodal, edge, focal point;
- temporal pattern: straight, euclidean, polymetric, call-response;
- transform: rotate, mirror, phase-shift, invert, dilate;
- modulation: energy, tension, player-relative offset, archetype blend.

La grammatica produce varietà senza generare combinazioni arbitrarie: ogni trasformazione dichiara compatibilità e impatto sui descrittori.

### 11.4 Candidate batch

A ogni decision point il generatore crea un batch bounded, per esempio 16–64 candidati, in funzione del budget.

Pseudo-flusso:

```cpp
// System 1 (§11.7): ordina riferimenti con descriptor stimati, senza generare eventi.
refs = retriever.retrieve(query(context), sources, k);
// System 2: generazione completa, validazione e scoring solo sui top-K.
for ref in refs:
    candidate = generate(ref.family, ref.params, context);
    if hardValidator.accepts(candidate):
        candidate.score = scorer.evaluate(candidate, context);
        valid.push(candidate);
return selector.choose(valid);
```

Il numero è configurabile. Il runtime deve restare corretto anche con un solo candidato e con zero candidati validi. Fino a M5 il retriever può restituire tutti i candidati compatibili in ordine stabile; la baseline a distanza pesata arriva in M6.

### 11.5 Trasformazioni player-relative

Un pattern può orientarsi rispetto al giocatore, ma deve evitare inseguimento perfetto e imprevedibile. Le trasformazioni player-relative usano:

- posizione campionata a un decision point;
- predizione limitata e smussata;
- offset massimo;
- telegraph aggiornato coerentemente;
- nessuna correzione dell’ultimo istante dopo il commit.

### 11.6 Memoria della run

Il generatore mantiene un sommario bounded:

- ultime family;
- ultimi descrittori;
- pattern falliti/subiti;
- trasformazioni recenti;
- palette/archetype blend;
- motivi musicali usati;
- novelty debt;
- recovery debt.

Questo impedisce ripetizioni locali anche se la distribuzione globale è ricca.

### 11.7 System 1 — Fast Candidate Retrieval

Il CIE separa due stadi. I nomi riprendono un'analogia (veloce/approssimativo contro lento/deliberato); non sono affermazioni cognitive.

- **System 1, `CandidateRetriever`:** risponde a "quali candidati vale la pena esaminare adesso?". È economico, deterministico e restituisce un top-K ordinato.
- **System 2, planner validato:** generazione completa degli eventi, trasformazioni, ricerca bounded (§14.3), validator autoritativo (§12), utility (§13) e commit (§8.6). Risponde a "quale candidato è valido, sicuro e migliore ora?".

```text
MusicIntent, PlayerModel, ExperienceTarget, ArchetypePolicy, NoveltyMemory
        |
        v
System 1  CandidateRetriever: filtri certi -> distanza pesata -> novelty/costo -> top-K stabile
        |  top-K (per esempio 8–32)
        v
System 2  generazione eventi -> validator -> utility -> selezione -> PlanProposal
```

#### 11.7.1 Regola di sicurezza

System 1 non decide mai se un candidato è sicuro né lo ammette. Può solo escluderlo dall'esame o ordinarlo. Ogni candidato eseguito passa per System 2 e, a runtime, per il safety guard (§12.5). Un retriever errato può peggiorare qualità o varietà, ma non la fairness. I fallback (§14.5) non dipendono dal retriever.

#### 11.7.2 Descriptor di retrieval

`DescriptorVector` (§11.1, §17.5) è già un embedding di dominio. System 1 ne usa una versione normalizzata e quantizzata in interi:

```cpp
// Illustrativo: dimensioni definitive in M6 (open question §36.18).
struct RetrievalDescriptor {
    std::int16_t density, symmetry, dashDemand, curvature,
                 rhythmComplexity, safeArea, pressure, readability;
};
```

Il descriptor deve essere **stimabile senza generare gli eventi completi**, a partire da definizione e parametri del pattern (o dall'archivio M8). Se richiedesse la generazione completa, System 1 non ridurrebbe il costo. La stima è approssimata e la verifica dei valori reali resta a System 2. Mapping e quantizzazione fanno parte della policy versionata (§15.8).

#### 11.7.3 Baseline M6

```text
D(c, t) = sum_i w_i * (c_i - t_i)^2      aritmetica intera, saturazione verificata
```

- `t`: target derivato da `ExperienceTarget`; `w`: pesi di policy da archetipo, player model e contesto musicale.
- Pipeline: filtri certi -> distanza -> penalità novelty e costo -> compatibilità con il profilo di controllo -> top-K.
- **Filtri certi:** escludono solo per motivi non di sicurezza e decidibili senza simulazione, come family non ammessa dall'archetipo, inviluppo di controllo richiesto non disponibile, budget entità dichiarato superato, cooldown di ripetizione. Superarli non significa "safe".
- **Diversità:** il top-K non deve collassare su varianti quasi identiche. Usare quote per family o una diversificazione deterministica, più slot riservati all'esplorazione entro il novelty budget (§13.3).
- **Ordinamento stabile:** `(distanza, PatternId, indice parametri)`. K, quote e pesi sono dati di policy, mai costanti nel codice.
- **Costo:** ricerca lineare. Per circa 10.000 pattern × 16 dimensioni sono circa 160.000 operazioni per query; non servono indici approssimati (HNSW) né database vettoriali.

#### 11.7.4 Determinismo e Pure Seed

In Pure Seed K è fisso, la distanza è intera e il tie-break è stabile. Il risultato non dipende da wall-clock, ordine di caricamento o thread. In Standard live K può ridursi sotto pressione (§27.2). Il top-K effettivo è registrato nella decision trace e il replay exact lo consuma. Il retriever ha un proprio identificatore di versione incluso nel RunRecord.

#### 11.7.5 Interfaccia

```cpp
// Illustrativo.
struct RetrievalQuery {
    RetrievalDescriptor target;
    RetrievalWeights weights;
    ContextFilter filter;       // archetipo, envelope, budget, storico
    std::uint32_t k;
};

class ICandidateRetriever {
public:
    virtual ~ICandidateRetriever() = default;
    // Scrive al massimo query.k riferimenti in out; ordinamento stabile.
    virtual RetrievalResult retrieve(const RetrievalQuery&, const CandidateSource&,
                                     FixedSpan<CandidateRef> out) const = 0;
};
```

Le sorgenti (generatore costruttivo, archivio M8) espongono descriptor stimati. Il retriever non conosce il validator.

#### 11.7.6 Metriche e sostituzione con modelli appresi

La metrica principale è **Recall@K**: la frazione di decision point in cui il candidato scelto da un planner di riferimento (System 2 con budget esteso o esaustivo, offline/headless) compare nel top-K del retriever. Si accompagna a chiamate al validator per decisione, latenza p50/p95/p99, fallback rate, accepted ratio, utility finale, diversità/novelty e riproducibilità Pure Seed. K, soglie e corpus si fissano prima di misurare.

Un retriever appreso (§19.2) o un encoder sostituisce la baseline solo se, sullo stesso corpus: Recall@K aumenta oppure le chiamate al validator e la latenza p95 diminuiscono a Recall uguale; il fallback rate non peggiora; la qualità finale non peggiora; il replay resta invariato. Altrimenti si rimuove (§31.4). Gli encoder testuali generici (BGE, MiniLM e simili) non sono ammessi nel runtime CIE: i descriptor strutturati contengono già l'informazione rilevante. Il loro uso possibile è la ricerca semantica nel tooling (§19.2).


---

## 12. Validazione hard e fairness

### 12.1 Principio

La decisione di ammissione è binaria; il validator distingue accepted/rejected/unknown. Solo accepted passa. La qualità è graduata: un candidato non ammesso non può vincere grazie a uno score estetico elevato.

### 12.2 Classi di hard constraint

#### Temporali

- telegraph minimo;
- nessun evento prima del commit boundary;
- durata compatibile con frase/battuta;
- rate di spawn sotto limite;
- nessun cambio di regola senza preavviso.

#### Spaziali

- nessuno spawn letale dentro hitbox + margine;
- area sicura minima;
- corridoio raggiungibile;
- velocità di chiusura sotto limite;
- destinazioni dash valide quando il pattern richiede dash;
- margine contro discretizzazione e floating point.

#### Di capacità

- numero massimo di entità;
- budget particellare separato;
- costo shader/pattern entro quality tier;
- memoria bounded;
- eventi per tick entro limite.

#### Di leggibilità

- telegraph visibile rispetto alla palette;
- minacce distinguibili dagli effetti;
- nessun lampeggio oltre profilo accessibilità;
- pattern simultanei limitati per cognitive load.

#### Musicali

- eventi quantizzati secondo policy;
- accenti compatibili con Music Intent;
- trasformazioni che non distruggono la frase;
- recovery e climax coerenti con pacing state.

### 12.3 Reachability validator

Il validator usa la geometria autoritativa, con approssimazioni conservative esplicite. Una griglia 64×36 a 10–30 campioni/s è soltanto uno spike di costo: controllare celle libere agli estremi non dimostra che il tragitto intermedio sia libero.

1. Comporre hazard attivi, eventi già impegnati e candidato, incluse code che oltrepassano il chunk.
2. Propagare stati `(cell/position, time, dashCooldown, phase, resources)` con limiti conservativi. Non unire stati incompatibili in una cella attribuendo loro il cooldown migliore.
3. Validare ogni transizione sull'intero intervallo tramite swept collision o un bound dimostrato sul moto relativo, incluso movimento di muri e confini.
4. Dilatare gli ostacoli per raggio del player, errore spaziale e incertezza temporale; restringere l'insieme di azioni del player al profilo ammesso. L'inflazione va derivata da velocità/accelerazione massime, non scelta come margine arbitrario.
5. Gli archi dash richiedono origine raggiungibile, risorsa disponibile, percorso compatibile con le regole di fase e landing sicuro all'arrivo; aggiornano le risorse.
6. Conservare predecessori e ricostruire almeno una traiettoria testimone. Rivalidarla nella simulazione autoritativa con lo stesso timestep, margine di reazione e incertezza dichiarata.
7. Validare la transizione terminale verso una continuazione sicura; una strada che finisce contro un muro appena fuori orizzonte non basta.

```text
R_next = { successor(s, action) |
           s in R_current,
           action feasible for s,
           swept transition safe,
           resulting resources valid }
```

Esito: `accepted`, `rejected` oppure `unknown` (timeout, forecast incompleto, bounds assenti). `unknown` non passa il gate. Il certificato di accettazione include contesto, intervallo, hash geometria/policy, modello di controllo e testimone. La sicurezza vale entro tali assunzioni; una via esistente non garantisce che un essere umano la legga né che ogni sua scelta sia salvabile.

Se lo snapshot di accettazione è cambiato, rivalidare rispetto allo stato effettivo o a un insieme iniziale certificato che lo contiene. Non confondere la sola esistenza di una via da una vecchia posizione con sicurezza dalla posizione attuale. Il runtime guard impedisce nuove violazioni della policy; non cancella sistematicamente le conseguenze di errori volontari del giocatore.

Regressioni minime: proiettile che attraversa una cella tra campioni; diagonale che taglia un angolo; due dash richiesti prima del recharge; pattern singolarmente validi ma invalidi insieme; chunk valido isolato ma senza uscita alla fine; snapshot scaduto. Misurare falsi positivi contro un oracle più preciso su fixture piccole.

### 12.4 Modelli di controllo

Per evitare che un solo bot determini la fairness, validare almeno contro envelope distinti:

- novice movement envelope;
- standard envelope;
- expert envelope;
- no-dash envelope quando il pattern non dichiara dash obbligatorio;
- cooldown-aware dash envelope.

Un pattern può dichiarare il profilo minimo richiesto, ma deve essere selezionato solo se compatibile con modalità e player model.

### 12.5 Runtime safety guard

Prima di introdurre nuove minacce, controllare in lavoro bounded: epoch e revisione, spawn non sovrapposto, capacità, precondizioni del certificato, telegraph realmente presentato e validità del futuro composto. Un callback di disegno eseguito non prova che il giocatore abbia visto il telegraph: registrare la prima presentazione stimata e usare margini conservativi per stall/jitter.

Politiche:

- Prima del telegraph: rifiutare/sostituire l'evento e scegliere una continuazione valida nel contesto.
- Dopo il telegraph: nessuna retargetizzazione, accelerazione, riduzione del preavviso o nuova minaccia. Per un guasto tecnico/certificato invalido è ammessa solo riduzione del pericolo, resa visibile (dissolvenza/neutralizzazione), con `SafetyIntervention` nel trace.
- Il guard non neutralizza una minaccia corretta soltanto perché il giocatore ha scelto una traiettoria perdente. Distinguere violazione della policy e errore di gioco.
- Se manca una continuazione certificata, sospendere nuovi spawn; se anche lo stato già impegnato è tecnicamente incoerente, usare una pausa controllata o neutralizzazione esplicita. Mai aggiungere un fallback letale non validato.

Le eccezioni tecniche interrompono l'idoneità al confronto Pure Seed della run; il giocatore può continuare come sessione non confrontabile. Il replay registra evento, motivo, tick e modifica. Safety ha precedenza sul beat e nessun controllo complesso viene trasferito alla callback.

### 12.6 Fairness metrica

Metriche minime per candidato:

```cpp
struct FairnessMetrics {
    float minReachableArea;
    float meanReachableArea;
    float minCorridorWidth;
    float minReactionSeconds;
    float dashDependency;
    float unavoidableRisk;
    std::uint32_t escapeRouteCountMin;
};
```

`unavoidableRisk` è un indicatore di violazione nel modello, non una probabilità scientificamente calibrata. Per contenuto standard sono ammessi solo esiti accepted senza violazioni; zero non dimostra assenza universale di rischio. Timeout e assunzioni mancanti sono unknown, non zero.

---

## 13. Scoring e selezione

### 13.1 Funzione di utility

Dopo il gate hard, ogni candidato riceve score normalizzati:

```text
utility =
    w_fit       * targetFit
  + w_music     * musicalCoherence
  + w_novelty   * novelty
  + w_style     * archetypeIdentity
  + w_flow      * transitionQuality
  + w_afford    * grazeAndDashAffordance
  - w_cost      * performanceCost
  - w_repeat    * repetitionPenalty
  - w_risk      * fairnessMarginPenalty
```

I pesi sono dati di policy, non costanti sparse nel codice.

### 13.2 Target fit

Confronta i descrittori del candidato con `ExperienceTarget`:

```text
targetFit = 1 - normalized_distance(candidate.descriptors, target)
```

Le dimensioni possono avere tolleranze diverse. La distanza non deve assumere che più pressione sia sempre meglio.

### 13.3 Novelty score

La novità è distanza dallo storico recente, non casualità assoluta.

Descrittori consigliati:

- densità;
- simmetria;
- curvatura;
- direzione dominante;
- velocità;
- ritmo;
- numero di flussi;
- dash demand;
- graze opportunity;
- safe-area profile;
- archetype blend.

La novelty è limitata da un budget. Troppa novità consecutiva riduce leggibilità e apprendimento.

### 13.4 Selezione stocastica controllata

Scegliere sempre il massimo rende il sistema prevedibile e può produrre convergenza locale. Il selector può campionare tra i migliori candidati:

```text
P(candidate_i) = softmax(utility_i / temperature)
```

Regole:

- temperature bassa in climax/challenge;
- temperature più alta in exposition/variation;
- top-k limitato;
- candidati quasi al limite di fairness penalizzati;
- seed/key registrati;
- modalità Pure Seed completamente riproducibile.

### 13.5 Explanation record

```cpp
struct SelectionExplanation {
    PatternId selected;
    float utility;
    float targetFit;
    float novelty;
    float musicalCoherence;
    float performanceCost;
    std::array<RejectedCandidateReason, kMaxLoggedRejections> rejected;
};
```

In release può essere ridotto; in debug è fondamentale per capire perché il director produce una certa run.

---

## 14. Pianificazione a orizzonte mobile

### 14.1 Due orizzonti

- **Planning horizon:** futuro su cui il planner ragiona, tipicamente 2–8 battute.
- **Commit horizon:** tratto già garantito al player/audio, tipicamente 0.5–1 battuta o un valore coerente con il telegraph massimo.

```text
now | committed events | mutable planned tail | unknown future
```

Il planning horizon può variare con BPM; in Pure Seed non varia con il carico hardware (§15.9). Le durate in battute sono suggerimenti; le condizioni reali sono espresse in sample frame.

Distinguere coda audio già pubblicata (immutabile) e piano mutabile nel worker. Un evento diventa impegnato al primo tra pubblicazione audio e presentazione del suo telegraph. Non inserire tutta la coda mutabile nella SPSC audio.

Per ogni hazard: `activation - firstTelegraphPresentation >= minReactionFrames`. Il lead di pubblicazione deve inoltre coprire lookahead del renderer audio, margine di pubblicazione/jitter e preavviso; il planner parte prima di tale deadline. A BPM estremi aumentare i beat di anticipo o rifiutare la policy: `commit_beats = 1` non garantisce da solo la fairness. Misurare separatamente lead audio, durata telegraph e margine operativo.

### 14.2 Receding-horizon planning

A ogni decision point:

1. conserva il prefisso impegnato;
2. aggiorna player model e pacing;
3. genera candidati per la coda mutabile;
4. valida e seleziona;
5. pubblica il nuovo chunk;
6. avanza il commit boundary.

### 14.3 Beam search bounded

Per transizioni di più pattern, usare un beam search piccolo:

- beam width: 4–8 iniziale;
- profondità: 2–4 pattern;
- budget temporale hard;
- pruning immediato sui vincoli;
- cache di risultati geometrici;
- fallback se deadline superata.

La configurazione va profilata. In Pure Seed il limite di espansioni è logico e fisso; il superamento della deadline invalida il confronto, non cambia il candidato vincente. Non inserire numeri elevati per “più intelligenza” senza misurazione.

### 14.4 Deadline

Il planner riceve una deadline legata al commit horizon. Non è ammesso attendere indefinitamente.

```cpp
// Standard live only; Pure Seed uses the logical budget in section 15.9.
if (clock.now() >= deadline.soft) {
    stopExpandingCandidates();
}
if (clock.now() >= deadline.hard) {
    returnBestValidOrFallbackProposal(); // coordinator validates and commits
}
```

### 14.5 Fallback library

Ogni archetipo deve possedere almeno:

- neutral sustain;
- safe pulse;
- recovery corridor;
- low-density transition;
- resolution pattern.

I fallback:

- sono prevalidati entro precondizioni esplicite e ricontrollati contro hazard attivi e prefisso impegnato;
- non richiedono ricerca estesa; se le precondizioni non valgono, usare il fallback senza nuovi hazard di §12.5;
- rispettano il Music Intent;
- possono essere parametrizzati soltanto entro range sicuri;
- non devono risultare come freeze o errore evidente.

### 14.6 Late planner policy

Se il planner è in ritardo:

1. riusa un fallback coerente;
2. riduce novelty e cognitive load;
3. non salta il clock musicale;
4. registra `PlannerDeadlineMiss`;
5. non blocca mai audio o game thread.

---

## 15. Non determinismo controllato e riproducibilità

### 15.1 Obiettivo

CYMATICA deve sembrare vivo e non memorizzabile completamente. Allo stesso tempo, un bug deve poter essere riprodotto.

### 15.2 Seed hierarchy

```text
run_seed
├── music_form_stream
├── harmony_stream
├── rhythm_stream
├── director_stream
├── pattern_stream
├── adaptation_stream
├── audio_humanization_stream
├── vfx_stream
└── cosmetic_stream
```

Gli stream di gameplay non dipendono da VFX o audio humanization.

### 15.3 Random key stateless

Per ridurre l’accoppiamento all’ordine delle chiamate, usare una funzione pseudo-casuale indicizzata:

```cpp
struct RandomKey {
    std::uint64_t runSeed;
    std::uint32_t streamId;
    std::uint64_t decisionId;
    std::uint32_t sampleIndex;
};

std::uint64_t randomU64(RandomKey key);
```

L’implementazione può usare un mixer stabile documentato o un counter-based generator. L’algoritmo scelto deve avere test vector e non può cambiare senza incrementare `rng_version`.

Non usare `std::hash` per output persistenti: la stabilità tra implementazioni non è garantita.

### 15.4 Seed derivation

```text
stream_seed = stable_mix(run_seed, stream_id, policy_version)
value       = stable_mix(stream_seed, decision_id, sample_index)
```

Il sistema non ha bisogno di conservare uno stato globale mutabile per ogni chiamata, e il risultato resta stabile anche se un altro stream consuma più numeri.

### 15.5 Decision identity

Ogni decision point ha ID deterministico derivato dalla posizione musicale e dalla run:

```text
decision_id = stable_mix(section, phrase, bar, decision_slot, policy_version)
```

`stable_mix` è il mixer documentato di §15.3, coperto da test vector e `rng_version`; mai `std::hash`.

### 15.6 Replay levels

#### Exact replay

Contiene:

- build/content hash;
- policy version;
- run seed;
- decision trace materializzato;
- input trace o snapshot necessari;
- correzioni del safety guard.

È il formato per bug e test regression. La prima garanzia è limitata a stessa build, piattaforma, asset, profilo numerico e input tick-stamped. Portabilità bitwise cross-platform non è promessa; audio DSP e immagine finale non sono inclusi negli hash gameplay salvo contratto specifico.

#### Semantic replay

Contiene seed e parametri principali. Per una policy non adattiva può rigenerare il piano entro il profilo di determinismo dichiarato; una run adattiva richiede anche gli input/osservazioni necessari al director. Seed e parametri da soli non ricostruiscono la risposta a un giocatore diverso.

#### Share seed

È un’esperienza equivalente destinata ai giocatori; non promette identità tra versioni del gioco.

### 15.7 Run record

```json
{
  "schema_version": 1,
  "build_id": "git:7496098+working",
  "policy_version": "cie-policy-0.1.0",
  "rng_version": 1,
  "run_seed": "0x6e51a0b7d44c1f23",
  "mode": "infinite",
  "difficulty_policy": "standard-adaptive",
  "decisions": [],
  "runtime_interventions": [],
  "final_metrics": {}
}
```

### 15.8 Versioning compatibility

Una modifica a uno dei seguenti elementi può invalidare il semantic replay:

- RNG algorithm;
- pattern generator;
- hard constraints;
- score weights;
- pattern catalog;
- music grammar;
- fixed timestep;
- collision geometry.

Il progetto non deve fingere compatibilità quando non esiste.

---

### 15.9 Determinismo, deadline e modalità Pure Seed

RNG stabile non basta se il numero dei candidati dipende dai millisecondi disponibili. Distinguere:

- **Standard live:** deadline wall-clock e degradazione sono ammesse; registrare candidati effettivi, piano accettato, fallback e interventi. Il replay exact riproduce queste decisioni, non riesegue la competizione con il timer.
- **Pure Seed:** budget logico fisso (candidati/nodi/step), ordine stabile, tie-break tramite ID, configurazione e contenuti congelati. Il timer misura la performance ma non sceglie il vincitore. Se l'hardware non completa in tempo, la run perde l'idoneità al confronto prima di usare fallback; non dichiara lo stesso seed equivalente.
- **Headless:** clock virtuale, stesso algoritmo e stesso budget logico del profilo sotto test; accelerare il wall-clock, non il delta di simulazione.

Disabilitare l'adattamento non disabilita automaticamente targeting player-relative o Drop Shock: Pure Seed significa stessa policy e stesse condizioni iniziali, non stessi proiettili per input differenti. Stessi input devono riprodurre lo stesso stato. Un eventuale confronto su timeline identica richiede una modalità separata che materializzi il piano ed escluda targeting reattivo.

RunRecord deve includere: internal sample rate, simulation Hz, transport epoch/discontinuità, input tick e ordine, hash catalogo/asset/policy, versione validator/generator/RNG, profilo numerico, configurazione assist e timing, budget logico, revisioni accettate, checkpoint e stato del controller. Serializzare campi canonici, mai memoria grezza con padding/puntatori. Verificare saturazioni e overflow; RNG-to-float, ordinamento, softmax stabile e tie-break fanno parte del contratto. Se la traccia si interrompe per overflow o limite disco, marcare replay incompleto e confronto non valido.

### 15.10 Memoria e durata della run

Le cronologie del director sono bounded; trace e checkpoint hanno limite disco e retention configurabili. Non accumulare `decisions` all'infinito in RAM: il JSON §15.7 è uno schema illustrativo, il writer deve essere incrementale fuori realtime. Definire granularità e frequenza di checkpoint prima del replay M6. Nessuna esecuzione infinita può garantire conservazione illimitata su risorse finite.

## 16. Motore di musica procedurale

### 16.1 Obiettivo

Il motore musicale deve creare continuità, memoria e forma. Randomizzare note valide in una scala non è sufficiente.

### 16.2 Pipeline gerarchica

```text
Run Form Planner
      v
Section/Phrase Planner
      v
Harmony + Motif Planner
      v
Rhythm Planner
      v
Four-Role Orchestrator
      v
Event Scheduler
      v
DSP/Synthesis Graph
```

### 16.3 Form planner

Genera una curva a medio termine:

- durata indicativa delle sezioni;
- energia;
- tensione;
- tonalità/modalità;
- archetipo dominante;
- densità;
- punti di recovery e climax.

Nella modalità infinita usa una grammatica con memoria per evitare cicli identici.

### 16.4 Motif memory

Ogni sezione introduce pochi motivi identificabili. Il sistema può:

- ripetere;
- trasporre;
- invertire intervalli;
- variare ritmo;
- frammentare;
- call-and-response;
- cambiare orchestrazione.

La probabilità di trasformazione dipende da novelty budget e pacing.

### 16.5 Rhythm planner

Supporta:

- griglie regolari;
- Euclidean rhythms;
- polymeter;
- sincopi;
- density curves;
- fill limitati a boundary;
- humanization bounded.

Ogni evento ritmico produce anche un’intenzione di gameplay, non necessariamente una minaccia.

### 16.6 Harmony planner

Per il prototipo:

- scale e modi espliciti;
- progressioni finite per archetipo;
- voice-leading semplice;
- tension/release controllata;
- bass note e chord tones coerenti;
- dissonanze musicali volontarie distinte dalla Dissonanza di salute.

La Dissonanza gameplay non deve automaticamente introdurre note casualmente stonate: deve degradare il suono con una policy musicalmente controllata.

### 16.7 Four-role orchestrator

Assegna eventi a Pulso, Corpo, Trama e Vettore. Ogni ruolo produce:

- eventi audio;
- intensità;
- descrittori per il CIE;
- eventuale affordance di gameplay.

### 16.8 Sintesi

Approccio iniziale ibrido:

- oscillatori band-limited dove possibile;
- sample proprietari o con licenza verificata per transienti;
- physical modeling semplice per membrane/stringhe;
- granular texture con pool preallocato;
- ADSR;
- filtri;
- saturazione;
- delay/reverb bounded;
- sidechain;
- limiter finale.

### 16.9 Pianificazione audio ahead-of-time

Il Music Planner opera fuori dalla callback e produce eventi con anticipo. La callback consuma una coda SPSC preallocata di eventi timestamped.

```cpp
struct ScheduledAudioEvent {
    std::uint64_t sampleFrame;
    AudioEventType type;
    VoiceId voice;
    float valueA;
    float valueB;
    std::uint32_t flags;
};
```

La callback non decide la forma musicale; esegue eventi già pianificati e applica controlli latest-value.

### 16.10 Adattamento musicale

La performance può influenzare il futuro musicale, ma solo in regioni mutabili:

- densità della prossima frase;
- apertura filtro;
- orchestrazione;
- tensione;
- complessità ritmica;
- probabilità di variazione;
- durata del recovery.

Non modificare retroattivamente note già schedulate né introdurre click, salto di fase o desincronizzazione.

### 16.11 Strudel/Tidal come riferimento

Strudel e Tidal sono utili per studiare pattern ciclici, mini-notation, trasformazioni e randomizzazione. CYMATICA deve derivare una grammatica nativa più piccola, tipizzata e orientata a Music Intent. Non dipendere da Strudel nel runtime senza decisione esplicita su licenza e stack.


---

## 17. Livelli dinamici

### 17.1 Livello come flusso di intenti

Il runtime non riceve direttamente “sparare proiettile X”. Riceve eventi semantici che il realizer trasforma in entità concrete.

```cpp
enum class PlannedEventType : std::uint8_t {
    Telegraph,
    HazardPattern,
    CymaticField,
    VectorActor,
    SafeWindow,
    GrazeOpportunity,
    DropShockAffordance,
    PaletteTransition,
    CameraAccent,
    VfxOnly
};
```

### 17.2 Pipeline di realizzazione

```text
Pattern candidate
  -> validated PatternPlan
  -> PlannedEvent stream
  -> Runtime Scheduler
  -> Authoritative gameplay primitives
  -> Rendering/VFX representation
```

Le primitive gameplay sono autoritative; shader e particelle le rappresentano ma non ne sostituiscono la collisione.

### 17.3 Cymatic field model

La geometria visuale usa una famiglia analitica ispirata alle figure di Chladni. È una scelta procedurale, non una simulazione fisica validata di una piastra: mancano qui materiale, spessore, condizioni al contorno, forzante e smorzamento.

```text
F(x, y) = cos(n*pi*x)*cos(m*pi*y)
        - cos(m*pi*x)*cos(n*pi*y)
```

Parametri dinamici:

- `m`, `n` interi bounded e distinti: con `m == n` la funzione è identicamente nulla; escludere anche combinazioni/blend degeneri;
- rotazione;
- phase offset;
- thickness;
- threshold;
- polarity;
- deformation;
- blend con altra modalità;
- archetype transform.

Il gameplay non usa direttamente ogni pixel della funzione. Usare coordinate normalizzate documentate, per esempio `(x,y) ∈ [0,1]²`, e conversione in world units indipendente dall’aspect ratio del viewport. Un `CymaticFieldSampler` genera:

- curve o segmenti principali;
- regioni nodali;
- regioni antinodali;
- spawn anchors;
- collision proxies;
- occupancy grid per validator.

### 17.4 Pattern families iniziali

Catalogo minimo:

1. `RadialPulse`
2. `CartesianCrossfire`
3. `NodalCorridor`
4. `RotatingArc`
5. `ExpandingRing`
6. `VectorPursuit`
7. `FractureBurst`
8. `DriftField`
9. `PolymetricOverlay`
10. `RecoveryWindow`

Ogni family deve avere almeno:

- una configurazione safe baseline;
- range parametri;
- test di determinismo;
- test del validator;
- cost estimate;
- descrittori;
- telegraph renderer;
- una rappresentazione di debug.

### 17.5 Archetype policy

Gli archetipi non sono classi C++ rigide. Sono policy e vincoli che influenzano:

- pattern family ammesse;
- distribuzione dei parametri;
- trasformazioni;
- tension curve;
- palette/motion via `DESIGN.md`;
- preferenza di score;
- fallback library;
- modalità di dash compatibili.

```cpp
struct ArchetypePolicy {
    ArchetypeId id;
    PatternFamilyMask allowedFamilies;
    DescriptorVector preferredCenter;
    DescriptorVector preferredTolerance;
    ScoreWeights scoreWeights;
    SafetyProfile safety;
    PacingProfile pacing;
};
```

### 17.6 Archetype blending

Una sezione può mescolare due archetipi con peso continuo, ma le regole gameplay non devono cambiare in modo ambiguo.

Regole:

- blend visuale continuo;
- pattern family composabili;
- una sola policy primaria per dash e collisioni in ogni intervallo impegnato;
- transizione telegrafata;
- nessun cambio di input semantics durante un’azione già iniziata.

### 17.7 Saturazione della piastra

La saturazione è un modificatore di lungo periodo, distinto dalla difficoltà adattiva.

```cpp
struct SaturationState {
    float energyAccumulation;
    float instability;
    float fractureProbability;
    float recoveryResistance;
};
```

Il director usa la saturazione per rendere più estreme le versioni degli archetipi, ma resta vincolato dalla fairness e dal challenge corridor.

### 17.8 Non determinismo strutturale

La varietà deve emergere da più livelli:

- forma musicale;
- sequenza di pattern family;
- parametri;
- trasformazioni;
- orientamento;
- archetipo/blend;
- risposta al player model;
- selezione stocastica top-k;
- micro-humanization non gameplay.

Non affidarsi a una singola chiamata casuale per “rendere diverso” un livello.

---

## 18. AI Lab offline e quality-diversity

### 18.1 Motivazione

Il runtime non deve esplorare ciecamente l’intero spazio. Una parte consistente della creatività può essere preparata offline generando pattern, simulandoli e organizzandoli in un archivio di soluzioni valide e diverse.

### 18.2 MAP-Elites / quality-diversity

Il laboratorio può usare MAP-Elites o un algoritmo equivalente per riempire celle definite da descrittori.

Esempio di spazio:

```text
axis 1: density
axis 2: symmetry
axis 3: dash demand
axis 4: curvature
axis 5: rhythm complexity
axis 6: safe-area profile
```

Ogni cella conserva uno o più elite che massimizzano qualità sotto quella combinazione. Partire con 2–3 assi misurati: sei assi con 10 bin ciascuno producono un milione di celle, spesso scarsamente coperte. Dichiarare denominatore della coverage, budget di valutazioni, memoria e confronto con baseline casuale/costruttiva.

Vantaggi per CYMATICA:

- non ottenere soltanto “il pattern migliore”;
- costruire varietà controllata;
- scegliere online un candidato vicino al target;
- mantenere stili differenti ma validi;
- ispezionare buchi nel design space.

### 18.3 Genoma di pattern

Un individuo può essere rappresentato da:

- family ID;
- parametri numerici;
- temporal grammar;
- spatial transform chain;
- field modes;
- telegraph profile;
- optional vector actor behavior;
- seed locale.

### 18.4 Fitness

La validità hard è un gate separato, mai un addendo compensabile. Dopo il gate la fitness combina:
- margine di fairness;
- coerenza musicale;
- aderenza all’archetipo;
- leggibilità;
- costo runtime;
- affordance di graze/dash;
- qualità della transizione;
- robustezza a più player persona.

La qualità estetica automatica è imperfetta: prevedere rating umano e blacklist/curation.

### 18.5 Procedural personas

Bot/playtester minimi:

| Persona | Comportamento |
|---|---|
| Novice | reazione lenta, poca previsione, dash conservativo |
| Survivor | massimizza distanza e sopravvivenza |
| Grazer | cerca rischio e score |
| Dasher | usa frequentemente il cambio di fase |
| Rhythm Expert | ottimo timing, mobilità media |
| Spatial Expert | pianificazione alta, timing medio |
| Stress Bot | introduce ritardo e errori crescenti |

Non servono subito agenti RL. Una combinazione di ricerca best-first, steering e policy parametriche è sufficiente per la prima validazione.

### 18.6 Fast simulation

Il lab usa una simulazione headless semplificata:

- stessa logica autoritativa di collisione;
- rendering disabilitato;
- esecuzione accelerata senza cambiare il delta del fixed timestep;
- output metriche;
- seed fissati;
- batch paralleli;
- timeout per candidato.

### 18.7 Archive format

```json
{
  "schema_version": 1,
  "generator_version": "pattern-gen-0.1.0",
  "validator_version": "fairness-0.1.0",
  "descriptor_axes": [
    { "name": "density", "bins": 10 },
    { "name": "symmetry", "bins": 8 },
    { "name": "dash_demand", "bins": 6 }
  ],
  "entries": []
}
```

L’archivio distribuibile deve contenere definizioni/parametri, non modelli o asset non necessari.

### 18.8 Uso online

Il runtime, tramite il `CandidateRetriever` (System 1, §11.7), può:

1. cercare celle vicine all’ExperienceTarget;
2. filtrare per archetipo e budget;
3. applicare mutazioni bounded;
4. rivalidare il risultato (System 2);
5. selezionare con novelty e utility (System 2).

L’archivio accelera e migliora la varietà, ma non è un single point of failure: il constructive generator e i fallback devono funzionare senza di esso. L'archivio è una sorgente aggiuntiva per il System 1, non un percorso alternativo al validator.

### 18.9 Surrogate model futuro

Quando esistono abbastanza dati, un modello leggero può stimare fitness o probabilità di successo e ridurre simulazioni costose. Deve:

- essere opzionale;
- avere confidence;
- non bypassare hard constraints;
- essere versionato;
- avere fallback al calcolo esplicito;
- dimostrare un beneficio misurabile prima dell’integrazione runtime.

---

## 19. Moduli ML opzionali

### 19.1 Regola generale

Il termine “AI” non obbliga a usare una rete neurale. Il core deve funzionare con sistemi simbolici e search-based. I modelli appresi possono migliorare singoli componenti.

### 19.2 Candidati futuri

#### Player model estimator

Input: metriche recenti.  
Output: skill vector, stress/flow proxy, confidence.

#### Pattern preference model

Input: player history + descrittori.  
Output: preferenza o retention proxy.

#### Contextual bandit

Seleziona pattern family tra opzioni valide, apprendendo reward locali senza controllare direttamente la safety.

#### Archetype/section classifier

Per musica custom, stima sezioni e blend archetipi da feature MIR/stem.

#### Fitness surrogate

Stima costo/qualità di candidati offline o online.

#### Music continuation model

Solo dopo una base simbolica stabile; deve produrre Music Intent o MIDI-like events, non audio opaco privo di timing semantico.

#### Learned candidate retriever (System 1)

Input: `ContextDescriptor` + `RetrievalDescriptor` del pattern. Output: punteggio di utilità attesa per l'ordinamento top-K. Modelli piccoli (regressione logistica, piccolo MLP, gradient-boosted trees). Sostituisce la baseline di §11.7 solo alle condizioni di §11.7.6 e non decide mai l'ammissione.

#### Ricerca semantica nel catalogo (solo tooling)

Query in linguaggio naturale ("spirale organica densa, poco dash") verso metadati dei pattern, tramite un encoder di frasi. Solo in AI Lab/editor, mai nel runtime né nel replay. BGE-small-en e all-MiniLM-L6 sono modelli inglesi: per query italiane valutare un encoder multilingua. Richiede review di licenza, peso del modello (circa 90–130 MB non quantizzati per i due citati) e un runtime di inferenza tool-only (§19.4).

### 19.3 Inference boundary

Ogni modello espone:

```cpp
struct ModelResult {
    bool valid;
    float confidence;
    ModelVersion version;
    FixedSizeOutput output;
};
```

Se `valid == false`, confidence è bassa o deadline superata, usare la policy non-ML.

### 19.4 ONNX Runtime

ONNX Runtime resta tool/future runtime dependency opzionale. Non introdurlo nella vertical slice. Nessuna inferenza nella callback audio.

### 19.5 Training data

Non assumere di possedere dataset adeguati. Prima di training:

- definire target e metrica;
- stabilire provenienza e licenza;
- anonimizzare playtrace;
- separare train/validation/test per seed e pattern family;
- evitare leakage tra varianti quasi identiche;
- registrare model card e dataset card.


---

## 20. Architettura runtime e threading

### 20.1 Thread/phase model

Configurazione minima:

```text
Audio callback thread      hard realtime-like, bounded
Main/game thread           fixed simulation + input
Render phase               variable rate, main thread
Planning worker            bounded AI generation/search
I/O/tool workers           fuori gameplay realtime
```

### 20.2 Audio callback

Consentito:

- consumare eventi audio già schedulati;
- generare/mixare campioni;
- aggiornare DSP preallocato;
- leggere `AudioControlFrame` latest-value;
- pubblicare telemetria compatta;
- incrementare clock sample-accurate.

Vietato:

- heap allocation/free;
- mutex bloccanti;
- file I/O;
- log;
- parsing;
- ricerca AI;
- inferenza;
- accesso a renderer o UI;
- lavoro non bounded.

### 20.3 Game simulation

Default proposto per vertical slice:

- fixed timestep 120 Hz;
- accumulator con limite massimo di catch-up;
- rendering interpolato;
- collisioni autoritative a tick fisso;
- eventi musicali attivati in base al transport canonico e alla mappatura tick di §7.5;
- nessun avanzamento gameplay dipendente da FPS.

Il valore 120 Hz è una configurazione iniziale, non dogma. Deve essere profilato; un profilo 60 Hz può essere necessario su Android e costituisce un diverso profilo di simulazione/replay.

Non eliminare tick autoritativi per recuperare FPS mentre la musica continua. Entro il limite di catch-up eseguire tutti i tick; oltre il limite entrare in sospensione tecnica controllata, fermare nuove minacce, riconciliare transport/audio e riprendere con preavviso. Registrare la discontinuità; la run non resta idonea a Pure Seed. Valori del limite e della tolleranza vengono fissati nel profilo M1. Interpolazione di rendering e collisione devono rispettare la stessa timeline percettiva; VFX non spostano l'hitbox.

### 20.4 Planning worker

Il planning worker:

- riceve `DirectorInput` immutabile;
- genera e valuta candidati;
- rispetta deadline;
- restituisce una `PlanProposal` validata; il commit avviene sul coordinatore (§8.6);
- può allocare entro limiti noti;
- non modifica lo stato autoritativo;
- non chiama API raylib/miniaudio non thread-safe;
- conserva cache owned dal worker.

### 20.5 Comunicazione

Canali distinti:

```text
Audio -> Game:                AudioTelemetryFrame latest-value
Game -> Audio:                AudioControlFrame latest-value + comandi identificati con ack
Music Planner -> Coordinator: proposte di eventi audio (non direttamente all'audio)
Coordinator -> Audio:         ScheduledAudioEvent SPSC, unico producer (§20.7)
Game -> AI Worker:            DirectorInput mailbox/SPSC
AI Worker -> Game:            PlanProposal mailbox/SPSC, commit sul coordinatore
Game -> Telemetry:            bounded event log buffer
```

### 20.6 Snapshot protocol

Baseline: SPSC bounded con ownership degli indici, oppure triple buffer con slot esclusivo al reader e pubblicazione acquire/release. Il producer non sovrascrive un payload finché il consumer lo possiede. Vietato presumere che `trivially_copyable` renda atomica una copia concorrente.

Un seqlock con contatore atomico e payload ordinario letto/scritto contemporaneamente può avere data race C++; il retry non la elimina. Non è una baseline ammessa. Un'eventuale implementazione alternativa richiede prova di correttezza del memory model, operazioni atomiche lock-free sul target e limite di tentativi. Mai spinning illimitato nella callback. Il razionale è documentato in [WG21 P1478R0](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2019/p1478r0.html), proposta tecnica e non API C++20 da usare automaticamente.

Snapshot e piani devono possedere i dati o usare slot immutabili con lifetime esplicito fino all'ack del consumer. `FixedSpan` in §22.2 è una vista, non ownership: vietato riferirla a un pool live che il game thread può riusare. Aggiungere generation counter agli handle e restituire gli slot solo dopo il consumo/scarto.

### 20.7 Backpressure

| Canale | Politica obbligatoria |
|---|---|
| Audio telemetry | Perdita ammessa con contatore; reader drena e conserva il più recente. Il producer SPSC non modifica l'indice del reader per scartare il vecchio. |
| Audio controls | Latest-value per parametri continui, smoothing DSP; azioni one-shot, pause/resume e stop richiedono comandi identificati e acknowledgement. |
| Scheduled audio | Capacity ed eventi massimi per blocco/timestamp verificati prima del commit. Mai overwrite silenzioso di note-off o comandi già accettati. |
| Plan proposal | Rifiuto di epoch/revisione scaduti, capacità bounded e restituzione slot fuori audio. |
| Trace | Buffer bounded e writer esterno; perdita segnalata, replay exact marcato incompleto. |

Il coordinatore ammette batch solo se c'è spazio per tutti gli eventi e un margine riservato ai controlli essenziali. Al superamento: rifiutare il batch prima del commit e usare fallback; se manca materiale audio, applicare una coda di sustain/fade preallocata e segnalare underrun senza bloccare. Non sintetizzare retroattivamente un burst di note arretrate.

Un solo thread è producer della SPSC audio: il coordinatore audio/game riceve le proposte del music planner e i comandi player e li ordina. Non consentire due producer, neppure come scorciatoia fra music planner e audio (§20.5). Definire il massimo di eventi per blocco, voci e costo DSP sul profilo target.

### 20.8 Pause e device loss

Definire una state machine esplicita:

```text
Running -> Pausing -> Paused -> Resuming
Running -> DeviceLost -> Recovering -> Running/Fatal
```

Lo stop/start/uninit del device avviene sul thread di controllo, mai nella callback (vedere [manuale miniaudio](https://miniaud.io/docs/manual/)). La callback può rendere silenzio e pubblicare acknowledgement tramite percorso bounded.

Durante recovery:

- non avanzare eventi gameplay musicali senza clock valido;
- congelare o riconciliare accumulator;
- invalidare piani oltre boundary se necessario;
- non produrre burst di eventi arretrati.

---

## 21. Struttura C++ proposta

```text
cymatica/
├── CMakeLists.txt
├── CMakePresets.json
├── AGENTS.md
├── DESIGN.md
├── CYMATICA_Specifica_Agentica_Sviluppo.md
├── apps/
│   ├── cymatica_game/
│   ├── cymatica_tool_cli/
│   └── cymatica_lab_cli/
├── engine/
│   ├── core/
│   │   ├── fixed_step_clock.*
│   │   ├── run_context.*
│   │   └── version_ids.*
│   ├── audio/
│   │   ├── audio_engine.*
│   │   ├── audio_scheduler.*
│   │   ├── synthesis_graph.*
│   │   ├── music_clock.*
│   │   └── realtime_exchange.*
│   ├── music/
│   │   ├── form_planner.*
│   │   ├── phrase_planner.*
│   │   ├── rhythm_planner.*
│   │   ├── harmony_planner.*
│   │   ├── motif_memory.*
│   │   └── music_intent.*
│   ├── intelligence/
│   │   ├── director.*
│   │   ├── pacing_state.*
│   │   ├── experience_target.*
│   │   ├── player_model.*
│   │   ├── candidate_generator.*
│   │   ├── candidate_scorer.*
│   │   ├── receding_horizon_planner.*
│   │   ├── fallback_library.*
│   │   └── explanation.*
│   ├── generation/
│   │   ├── pattern_catalog.*
│   │   ├── pattern_definition.*
│   │   ├── pattern_registry.*
│   │   ├── pattern_transform.*
│   │   └── generators/
│   ├── validation/
│   │   ├── hard_constraint_gate.*
│   │   ├── reachability_grid.*
│   │   ├── fairness_metrics.*
│   │   └── runtime_safety_guard.*
│   ├── gameplay/
│   │   ├── player.*
│   │   ├── quantum_dash.*
│   │   ├── dissonance.*
│   │   ├── bullet_pool.*
│   │   ├── collision.*
│   │   └── plan_realizer.*
│   ├── graphics/
│   │   ├── cymatic_renderer.*
│   │   ├── telegraph_renderer.*
│   │   ├── palette.*
│   │   └── debug_overlay.*
│   ├── particles/
│   │   ├── particle_pool.*
│   │   ├── particle_emitters.*
│   │   └── particle_renderer.*
│   ├── replay/
│   │   ├── seed_bank.*
│   │   ├── decision_trace.*
│   │   ├── run_record.*
│   │   └── replay_player.*
│   └── level_runtime/
│       ├── level_loader.*
│       ├── timeline_player.*
│       └── custom_level_index.*
├── shared/
│   ├── schemas/
│   ├── math/
│   ├── serialization/
│   └── identifiers/
├── toolchain/
│   ├── analysis/
│   ├── ingestion/
│   ├── packaging/
│   ├── quality_diversity/
│   └── personas/
├── data/
│   ├── patterns/
│   ├── policies/
│   ├── archetypes/
│   ├── music/
│   └── fallback/
├── assets/
│   ├── shaders/
│   ├── samples/
│   └── presets/
├── tests/
│   ├── unit/
│   ├── integration/
│   ├── property/
│   ├── replay/
│   ├── concurrency/
│   ├── performance/
│   └── fixtures/
└── docs/
    ├── progress.md          # stato milestone ed evidenze (AGENTS §5.1)
    ├── adr/                 # decisioni registrate (AGENTS §1.1)
    ├── architecture.md
    ├── build.md
    ├── dependencies.md
    ├── level-format.md
    ├── ai-policy-format.md
    └── replay-format.md
```

### 21.1 Target CMake

```text
cymatica_shared
cymatica_core
cymatica_audio
cymatica_music
cymatica_intelligence
cymatica_generation
cymatica_validation
cymatica_gameplay
cymatica_graphics
cymatica_particles
cymatica_replay
cymatica_game
cymatica_toolchain          optional
cymatica_tool_cli           optional
cymatica_lab_cli            optional
```

### 21.2 Direzione delle dipendenze

Direzione ammessa:

- `shared` non dipende da moduli engine;
- `core` dipende solo da `shared`;
- `music`, `generation`, `validation` e `replay` dipendono da `shared/core`;
- `intelligence` dipende dai contratti di `music`, `generation`, `validation` e `replay`, non dai rispettivi dettagli grafici o audio-device;
- `gameplay`, `audio` e `graphics` consumano piani e contratti stabili;
- `cymatica_game` compone i moduli senza invertire le dipendenze;
- `toolchain` può riusare i moduli headless, ma il runtime non dipende dalla toolchain.

Vincoli:

- `intelligence` non dipende da raylib;
- `validation` funziona headless;
- `music` produce intenti senza richiedere device audio;
- `audio` non dipende dal gameplay concreto;
- `graphics` non è autorità collisione;
- toolchain e ML non entrano nel target game per default.

### 21.3 Interfacce piccole

Esempio:

```cpp
class IDirectorPolicy {
public:
    virtual ~IDirectorPolicy() = default;
    virtual ExperienceTarget computeTarget(
        const DirectorInput&) const = 0;
};

class IHardValidator {
public:
    virtual ~IHardValidator() = default;
    virtual ValidationResult validate(
        const PatternCandidate&,
        const ValidationContext&) const = 0;
};
```

Non creare una gerarchia profonda. Dove possibile preferire value types, funzioni pure e composizione.

---

## 22. Contratti dati principali

### 22.1 Audio telemetry e control

```cpp
struct ChannelFrame {
    float energy;
    float onset;
    float pitchHz;
    float density;
    float confidence;
};

struct AudioTelemetryFrame {
    std::uint64_t transportEpoch;      // §7.5
    std::uint64_t renderCursor;        // primo frame ancora da sintetizzare
    std::uint64_t presentationCursor;  // stima del frame udibile
    PresentationEstimateQuality presentationQuality; // invalid/estimated/measured
    float bpm;
    float beatPhase;                   // vista derivata, mai autorità
    float beatConfidence;
    ChannelFrame pulse;
    ChannelFrame body;
    ChannelFrame texture;
    ChannelFrame vector;
    float globalEnergy;
    float spectralFlux;
    std::uint32_t detectedArchetypeId;
};

// Solo parametri continui latest-value. Pause/resume/stop e one-shot sono
// comandi identificati con acknowledgement (§20.7), non campi di questo frame.
struct AudioControlFrame {
    float dissonance;
    float playerPerformance;
    std::uint32_t requestedArchetypeId;
};
```

### 22.2 World snapshot

```cpp
struct WorldSnapshot {
    Vec2 playerPosition;
    Vec2 playerVelocity;
    float dashCooldownSeconds;
    float resonance;
    float dissonance;
    FixedSpan<HazardSummary> hazards;
    FixedSpan<BarrierSummary> barriers;
    ArenaBounds arena;
    std::uint32_t activeBulletCount;
    std::uint32_t activeParticleCount;
};
```

### 22.3 Pattern candidate

```cpp
struct PatternCandidate {
    PatternId pattern;
    PatternParameters parameters;
    DescriptorVector descriptors;
    EventBuffer events;
    OccupancyForecast occupancy;
    FairnessMetrics fairness;
    CostEstimate cost;
    UtilityBreakdown score;
    RandomKey generationKey;
};
```

### 22.4 Planned event

```cpp
struct PlannedEvent {
    EventId id;
    PlannedEventType type;
    std::uint64_t startSampleFrame;
    std::uint64_t telegraphSampleFrame;
    std::uint32_t generatorId;
    FixedPayload payload;
    std::uint32_t flags;
};
```

### 22.5 Policy version

```cpp
struct PolicyVersion {
    std::uint32_t schema;
    std::uint32_t major;
    std::uint32_t minor;
    std::uint64_t contentHash;
};
```

---

## 23. Formati dati

### 23.1 Policy file

```json
{
  "schema_version": 1,
  "policy_id": "standard-adaptive",
  "policy_version": "0.1.0",
  "difficulty_corridor": {
    "pressure_min": 0.40,
    "pressure_max": 0.68
  },
  "adaptation": {
    "max_delta_per_phrase": 0.08,
    "hysteresis": 0.05,
    "cooldown_phrases": 1
  },
  "planning": {
    "horizon_bars": 4,
    "commit_beats": 1,
    "candidate_budget": 32,
    "beam_width": 4
  }
}
```

### 23.2 Pattern schema

Ogni pattern file deve essere validato al caricamento. Il progetto deve fornire JSON Schema o validatore equivalente per:

- tipi;
- range;
- ID univoci;
- generator registrato;
- compatibilità archetipi;
- telegraph non negativo;
- hard limits;
- transform chain valida.

### 23.3 `.cymlevel`

Durante sviluppo, `.cymlevel` è una directory ispezionabile:

```text
Example.cymlevel/
├── level_info.json
├── music_intent.json
├── plan_timeline.json
├── audio/
│   ├── mix.ogg
│   ├── pulse.ogg
│   ├── body.ogg
│   ├── texture.ogg
│   └── vector.ogg
├── replay/
│   └── generation_trace.json
└── preview/
    └── cover.png
```

Prima di caricare contenuti esterni (M9), imporre limiti a byte/file/durata/entità, percorsi relativi confinati al pacchetto, rifiuto di traversal e link che escono dal root, codec ammessi e validazione semantica della timeline. Hash verificano integrità, non fiducia nell'autore. Vietare codice eseguibile nel pacchetto. Il formato archivio/distribuzione rimane da decidere; per ora non supporre che una directory sia un ZIP.

File opzionali devono essere dichiarati nel manifest. Il runtime deve supportare almeno:

- timeline + audio mix;
- timeline + quattro ruoli;
- Music Intent + seed per rigenerazione compatibile;
- livello fittizio senza audio per test.

### 23.4 Schema evolution

Regole:

- `schema_version` obbligatorio;
- parser fail-fast su major incompatibile;
- migration tool per formati persistenti;
- unknown fields ignorabili solo se specificato;
- enum serializzati con stringhe stabili;
- float non finiti vietati;
- hash dei contenuti registrato.

---

## 24. Rendering, particelle e fisica

### 24.1 Separazione autoritativa

```text
Gameplay primitive = collisione e regole
Shader field       = rappresentazione cimatica
Particle           = VFX, salvo tipo esplicitamente gameplay
```

### 24.2 Particle system

Resta custom, object-pooled e budgeted. Può essere guidato da campi cimatici, ma non deve influenzare implicitamente il validator.

Budget iniziale:

- 2.000–5.000 particelle CPU;
- pool preallocato;
- no allocation per frame;
- LOD e drop cosmetico;
- fallback Android;
- GPU particles soltanto dopo profiling.

### 24.3 Bullet system

Requisiti:

- deterministic update;
- pool preallocato;
- primitive collisione semplici;
- stable IDs;
- spawn da `PlannedEvent`;
- separazione telegraph/hazard;
- debug overlay di hitbox e traiettorie.

### 24.4 Motore fisico esterno

Non necessario per il core. Valutarlo soltanto per modalità specifiche o tooling, senza sostituire la simulazione deterministica dei pattern.

### 24.5 Visual design

Palette, layer, motion, HUD diegetico, archetipi e accessibilità sono definiti in `DESIGN.md`.


---

## 25. Testing strategy

### 25.1 Test unitari

Obbligatori per:

- beat/bar/phrase conversion;
- Euclidean rhythm;
- modal quantizer e voice-leading helpers;
- stable random mixer e test vector;
- seed derivation e stream isolation;
- pattern parameter validation;
- score function;
- hysteresis e challenge corridor;
- player model filters;
- fixed timestep accumulator;
- serialization/versioning;
- object pools;
- field sampler;
- collision primitives;
- safe landing assist.

### 25.2 Property-based test

Proprietà consigliate:

- con stesso profilo numerico e budget logico: stesso input + stessa policy + stesso seed = stesso piano; in Standard live i deadline miss vanno registrati e consumati dal replay exact;
- cambiare `vfx_stream` non cambia gameplay trace;
- ogni candidato accettato mantiene reachable set non vuoto;
- ogni hazard ha telegraph precedente;
- nessun evento è schedulato nel passato;
- valori normalizzati restano in `[0,1]`;
- pool non supera capacity;
- parser non accetta NaN/Infinity;
- una policy invalida non provoca crash;
- un planner senza candidati produce fallback.

### 25.3 Metamorphic test

- esecuzione a 60 e 120 FPS produce stesso stato fixed-step;
- rendering disabilitato non cambia gameplay;
- particelle disabilitate non cambiano collisioni;
- ordine dei job offline non cambia output indicizzato per RandomKey;
- variazioni non autoritative del sound design non cambiano il plan;
- replay exact riproduce hash periodici dello stato.

### 25.4 Concurrency test

- producer/consumer con scheduling casuale;
- overflow bounded;
- proprietà di ownership degli slot e numero bounded di operazioni;
- nessun frame parziale;
- device stop/start;
- planner deadline miss;
- shutdown durante plan in corso;
- sanitizers dove supportati.

### 25.5 Fuzzing

Target:

- JSON schema/parser;
- pattern parameters;
- plan timeline;
- replay file;
- arithmetic dei sample frame;
- transform chain;
- malformed `.cymlevel`;
- archive quality-diversity.

### 25.6 Automated playtest

Ogni build significativa esegue un set ridotto di seed con più personas. Nightly/locale esteso:

- migliaia di seed;
- distribuzione deaths;
- unreachable rate;
- planner fallback rate;
- pattern coverage;
- archetype coverage;
- mean/percentile entity count;
- deadline misses;
- duplicate sequence rate;
- recovery debt violations.

### 25.7 Golden traces

Conservare fixture piccole con:

- policy version;
- seed;
- expected decision IDs;
- expected plan hash;
- state hash ogni N tick.

Aggiornare golden soltanto con review esplicita, non automaticamente dopo un fallimento.

### 25.8 Manual test

- qualità e ripetitività musicale;
- game feel del movimento;
- leggibilità;
- fairness percepita;
- sorpresa senza caos;
- transizioni archetipi;
- audio crackle;
- input lag;
- photosensitivity profile;
- controller/mouse parity;
- qualità dei recovery;
- comportamento dopo lunga durata.

---

## 26. Metriche e osservabilità

### 26.1 Metriche del planner

- decision latency p50/p95/p99;
- candidate count;
- valid candidate ratio;
- rejection reasons;
- fallback rate;
- score distribution;
- archive hit rate;
- planning horizon coverage;
- commit slack;
- runtime intervention rate;
- System 1: latenza di retrieval, candidati recuperati/scartati, chiamate al validator per decisione, accepted ratio dei top-K, Recall@K offline contro il planner di riferimento (§11.7.6).

### 26.2 Metriche gameplay

- hit rate per pattern family;
- graze rate;
- dash correction rate;
- safe area percentile;
- pressure target vs measured;
- recovery debt;
- time in challenge corridor;
- deaths per archetype;
- player model confidence.

### 26.3 Metriche di varietà

- pattern family entropy;
- transform entropy;
- n-gram repetition su sequenze pattern;
- descriptor coverage;
- novelty medio e varianza;
- distanza tra run con seed differenti;
- distanza tra retry con stessa traccia e seed differente.

### 26.4 Debug overlay

Modalità debug deve poter mostrare:

- music clock;
- beat/bar/phrase;
- pacing state;
- ExperienceTarget;
- player model e confidence;
- pattern corrente/prossimo;
- commit/planning horizon;
- reachability grid;
- safe routes;
- score breakdown;
- seed/decision ID;
- entity budgets;
- audio underrun e planner deadline miss.

### 26.5 Trace levels

```text
OFF
ERRORS_ONLY
DECISIONS
FULL_CANDIDATES
REPLAY_EXACT
```

Il livello `FULL_CANDIDATES` non è adatto a release normale e deve essere bounded o scritto fuori dal path realtime.

---

## 27. Performance budget iniziale

### 27.1 Windows vertical slice

| Voce | Target |
|---|---:|
| Gameplay fixed step | 120 Hz iniziale |
| Rendering | 60 FPS minimo, 120 desiderabile |
| Frame time a 60 FPS | < 16,6 ms |
| Sim tick p95 | < 4 ms |
| Planner medio per decision point | < 4 ms worker time |
| Planner hard budget iniziale | 8 ms, configurabile |
| Audio allocation | 0 dopo init |
| Planner deadline miss | < 0,1% in soak test |
| Runtime safety interventions | prossime a zero su catalogo validato |
| Audio underrun | 0 in test nominale |

I numeri sono budget iniziali da validare, non garanzie già dimostrate. Prima del gate M7 fissare CPU, GPU, RAM, OS, build Release, risoluzione, qualità, sample rate, buffer audio e scena/seed di carico. Riportare p50/p95/p99 e massimi, non soltanto media. A 120 Hz il tick dispone di circa 8,33 ms e a 60 FPS possono essere necessari due tick più rendering: sommare i costi del main thread. Documentare warmup, durata e denominatore dei deadline miss; zero underrun in un test finito non è garanzia universale.

### 27.2 Candidate budget adattivo

In Standard live, se il worker è sotto pressione (in Pure Seed applicare invece §15.9):

1. ridurre candidate count;
2. ridurre beam width;
3. usare archive lookup;
4. evitare trasformazioni costose;
5. scegliere fallback;
6. non ridurre telegraph o fairness.

### 27.3 Entity budget

Separare:

- gameplay bullets;
- barriers;
- vector actors;
- telegraphs;
- particles;
- trails;
- debug primitives.

Le entità cosmetiche sono le prime da degradare.

### 27.4 Android futuro

- fixed tick configurabile 60/120;
- quality tiers;
- shader fallback;
- particle budget adattivo;
- planner budget ridotto;
- benchmark su device reali;
- test AAudio/miniaudio backend;
- nessuna inferenza ML obbligatoria.

---

## 28. Ambiente di sviluppo e dipendenze

### 28.1 Requisiti Windows

| Requisito | Stato | Note |
|---|---:|---|
| Windows 10/11 x64 | obbligatorio | target primario |
| Git | obbligatorio | repository |
| Visual Studio/Build Tools C++ | obbligatorio | MSVC |
| CMake | obbligatorio | build |
| Ninja o MSBuild | uno richiesto | preset documentato |
| PowerShell | consigliato | bootstrap e script |
| Python 3 | opzionale inizialmente | tool/lab futuri |
| Antigravity IDE | consigliato | non requisito di build |

### 28.2 Dipendenze core

| Dipendenza | Scopo | Acquisizione | Vincolo |
|---|---|---|---|
| raylib | window/input/render/shader | `FetchContent` pinned | versione registrata |
| miniaudio | device, mixing, DSP | snapshot ufficiale vendored | sorgente, commit/tag e licenza registrati |
| test framework | unit/property test | una sola soluzione pinned | Catch2 v3, vedere ADR-0001 |
| JSON parser/schema | contenuti e trace | valutare minimal/pinned | non introdurre più parser |

Versioni, commit, checksum e toolchain di M0 sono registrati in `docs/adr/0001-m0-toolchain-dependencies.md` (record unico; non duplicarli qui).

In M0 raylib è responsabile di finestra/input/render; il modulo audio raylib deve essere disabilitato nella configurazione pinned, e un solo target possiede `MINIAUDIO_IMPLEMENTATION` e il device. Verificare l'opzione disponibile nel commit raylib scelto (tipicamente `SUPPORT_MODULE_RAUDIO`) e il link finale; non aprire due motori audio indipendenti. Non è un errore già osservato, perché la build non esiste ancora.

Una libreria JSON diventa probabilmente necessaria prima dei formati data-driven. La scelta deve essere esplicita in M0/M1 e documentata; non implementare un parser JSON artigianale.

### 28.3 Dipendenze AI

Il CIE v1 non richiede framework AI esterni. Algoritmi di utility, search, player model e validator sono implementati nel progetto.

Possibili future:

| Dipendenza | Uso | Fase |
|---|---|---|
| ONNX Runtime | inferenza opzionale | post vertical slice |
| Random123 o equivalente | counter-based RNG | solo se preferito all’implementazione interna |
| libreria optimization/QD | lab offline | valutazione separata |
| Python scientific stack | esperimenti offline | ambiente isolato |

Ogni dipendenza deve superare review di licenza, portabilità, dimensione, manutenzione e beneficio.

### 28.4 Dipendenze tool/custom music

| Dipendenza | Uso | Regola |
|---|---|---|
| FFmpeg | decode/convert/normalize | tool-only, path configurabile |
| audio-separator | backend sperimentale stem | tool/research-only |
| modelli Demucs/MDX | stem | cache esterna, non committare |
| NatuStem | riferimento | non dipendenza automatica |
| Essentia/librosa-equivalent | MIR | valutare dopo formato stabile |
| Strudel | sketch musicale | research-only salvo decisione diversa |

### 28.5 Dependency inventory

`docs/dependencies.md` deve contenere:

- versione/tag/commit;
- URL/provenienza;
- checksum se manuale;
- licenza;
- target che la usa;
- modalità acquisizione;
- piattaforme;
- owner interno;
- data ultimo review;
- rischi.

### 28.6 Download agentici

Un agente può scaricare dipendenze se consentito, ma deve:

- usare fonte ufficiale;
- pinning esplicito;
- non installare globalmente senza approvazione;
- non eseguire binari non verificati;
- registrare i passaggi;
- fornire alternativa manuale;
- non rendere la build dipendente dalla cache personale.

---

## 29. Repository e distribuzioni

### 29.1 Monorepo

Gioco, lab e tool restano nello stesso repo finché condividono formati e cicli di sviluppo.

### 29.2 Distribuzioni

```text
dist/
├── game/
├── tools/
├── lab/       # interno, non necessariamente pubblico
└── examples/
```

Il gioco non distribuisce automaticamente:

- ONNX Runtime;
- Python;
- FFmpeg;
- modelli stem;
- Strudel;
- NatuStem;
- archive/debug trace non necessari.

### 29.3 CMake options

```cmake
option(CYMATICA_BUILD_GAME "Build game runtime" ON)
option(CYMATICA_BUILD_TESTS "Build tests" ON)
option(CYMATICA_BUILD_TOOLS "Build offline tools" OFF)
option(CYMATICA_BUILD_LAB "Build AI/QD lab" OFF)
option(CYMATICA_ENABLE_ONNX "Enable optional ONNX models" OFF)
option(CYMATICA_ENABLE_FFMPEG "Enable FFmpeg tool integration" OFF)
option(CYMATICA_ENABLE_TRACE "Enable extended decision traces" OFF)
```

---

## 30. Tool offline, NatuStem e custom music

### 30.1 Ordine corretto

1. stabilizzare Music Intent;
2. stabilizzare pattern/plan format;
3. creare validator/inspect CLI;
4. importare musica senza stem;
5. studiare NatuStem;
6. scegliere backend stem;
7. aggiungere separazione opzionale;
8. mantenere fallback mix-only.

### 30.2 NatuStem

NatuStem resta un riferimento tecnico per:

- setup FFmpeg;
- `audio-separator`;
- modelli;
- CPU/GPU;
- naming output;
- log;
- overwrite;
- gestione errori.

La pipeline CYMATICA deve essere headless, testabile e orientata a manifest. Il riuso diretto va deciso solo dopo uno spike e una license review.

### 30.3 CLI target

```bash
cymatica-tool validate Level.cymlevel
cymatica-tool inspect Level.cymlevel
cymatica-tool analyze song.wav --out work/song
cymatica-tool ingest song.mp3 --out work/song
cymatica-tool package work/song --out CustomLevels/Song.cymlevel
```

### 30.4 Manifest ingestion

Deve registrare:

- input hash;
- decoder/versione;
- sample rate;
- durata;
- backend stem;
- modello/versione;
- parametri;
- file prodotti;
- confidence;
- warning;
- tempi;
- licenze/provenienza modello dove applicabile.

---

## 31. Sviluppo agentico

### 31.1 Fonti normative

- `CYMATICA_Specifica_Agentica_Sviluppo.md`: architettura, roadmap, contratti;
- `DESIGN.md`: game, visual, audio e UX design;
- `AGENTS.md`: regole operative concise;
- schema e test: comportamento eseguibile.

### 31.2 Task slicing

Ogni task deve:

- appartenere a una milestone;
- avere acceptance criteria;
- modificare moduli minimi;
- includere test;
- aggiornare documentazione quando cambia un contratto;
- riportare comandi eseguiti;
- evitare lavoro futuro non richiesto.

### 31.3 Decision record

Decisioni che cambiano policy generativa, fairness, timing o replay devono essere registrate nel documento o in ADR dedicato.

### 31.4 No “AI magic”

L’agente non deve introdurre modelli, framework o prompt runtime per soddisfare genericamente il requisito “AI”. Deve indicare:

- problema preciso;
- baseline non-ML;
- metrica;
- dataset;
- costo runtime;
- fallback;
- licenza;
- criterio di rimozione.



### 31.5 Controllo agentico e osservazione audiovisiva incrementale

**Stato:** accepted for implementation come direzione e invarianti, richiesta del titolare del 2026-10-07. [Analisi dedicata](docs/agent_control_observability.md) per motivazioni, alternative, profili proposti, rischi e procedure; §32 resta fonte della roadmap. Nome provvisorio CYMATICA Agent Control Plane (CACP), distinto dal CIE.

- Contratto versionato indipendente da MCP/IPC; primo runner locale M2, adapter MCP valutato successivamente. Nessun browser, FFmpeg, modello o nuovo framework introdotto implicitamente.
- Azioni agente attraverso lo stesso dispatcher normalizzato del player, senza scritture dirette allo stato. Test semantici L0/L1 e test OS/device L2 producono evidenze distinte. Batch finiti con ID, tick target/effettivo, epoch, capacity e ack; rifiuto esplicito dei comandi scaduti e nessuna retrodatazione.
- L'LLM pianifica, un controller locale bounded esegue; tool/network/model non sono nel percorso di risposta realtime del giocatore. Step/run-until solo offline senza device live. Osservazione della sessione umana read-only per default, controllo esclusivo agente solo in modalità dichiarata; disconnect/focus loss/epoch change rilasciano input e invalidano batch.
- Snapshot con ownership (§20.6), artefatti correlati a run/build/profili, tick, epoch, sample-frame, sequence per canale, tempo monotono e qualità di presentazione (§7.5). Cattura render con interpolazione dichiarata; gap e skew espliciti, nessuna atomizzazione fittizia fra thread.
- AudioProbe opzionale copia PCM finale in slot preallocati SPSC: nessuna allocazione, lock, I/O, analisi o IPC nella callback. Consumer lento causa gap contabilizzati, non blocco né overwrite di dati posseduti. Worker gestisce storia, WAV/metriche e lifetime; solo il coordinatore produce eventi audio. Il tap misura sintesi, non audio acustico del device.
- VisualProbe player/debug separati; sequenze timestampate e audio prima dello streaming. Readback sul thread grafico, codifica/esportazione nei worker, queue bounded; consumo lento degrada cattura/live senza bloccare il gioco. La diagnostica deve essere disattivabile e profilata OFF/ON.
- Artefatti fuori hash gameplay, ma deadline miss/interventi causati dall'overhead continuano a invalidare Pure Seed come §15.9. Exact replay usa input effettivi e decisioni accettate, non soltanto seed. Giudizi agentici supplementari, validator e playtest restano autoritativi nei propri ambiti.
- Cattura opt-in confinata al gioco, output locale e bounded, indicatore/stop per sessioni umane; invio esterno esplicito. Gateway locale diagnostico disabilitato nella distribuzione normale, senza esecuzione arbitraria o scritture fuori artefatti.

Prima dei task dipendenti registrare D-AO-01–06 descritti nell'analisi: nucleo dispatcher comune del tono e schema/trasporto (D-AO-01), buffer e profilo tono (D-AO-02) prima di AC-AO-01A in M2; estensione movement/dash, scheduling per tick, ordine multi-source e capture (D-AO-03) in M3; scenario/clip M4; soglie di latenza e live M5; codec/client solo nello spike M6–M7. Non anticipare scelte future. I gate di implementazione e misura sotto controllo del progetto sono mandatory; la consegna di immagini/audio/video al modello e il giudizio agente sono verifiche supplementari capability-dependent, con esito distinto `passed`, `failed`, `blocked` o `unsupported` documentato. La capacità assente nel client non impedisce la chiusura dell’implementazione della milestone e non produce PASS percettivo. Questa regola vale anche per il video M6/M7; report dello spike e benchmark locale restano obbligatori, così come tutti i gate originali umani/hardware/accessibilità. Difetti del progetto, anche scoperti dal giudizio supplementare, restano da risolvere secondo AGENTS §5.1.

---

## 32. Roadmap revisionata

La milestone attiva iniziale resta **Milestone 0**. La vertical slice completa termina con Milestone 7. Le milestone sono obiettivi futuri, non feature presenti.

Gate trasversali, senza cambiare numerazione:

- **M0:** scegliere pin/toolchain/test framework e unico owner audio; README e inventario descrivono soltanto ciò che esiste.
- **M1:** congelare transport/replay profile e protocollo ownership; formalizzare il contratto player minimo richiesto dal validator.
- **M3:** implementare movimento/collisione e dash minimo headless, oppure un envelope senza dash esplicito. La giocabilità di 60–90 secondi richiede già questo nucleo; M5 lo completa, non lo introduce da zero.
- **M4:** validare primitive player reali, composizione dei pattern e boundary; smoke test di controllo/leggibilità prima di M5–M7. Non rimandare tutta la validazione umana alla fine.
- **M5:** completare risorse e game feel; ogni modifica al movimento/dash invalida certificati e richiede regressioni M4.
- **M6:** testare adattamento e replay separatamente dalle deadline hardware; fissare metriche prima di misurarle.
- **M7:** sequenza significativa di 60–90 secondi e soak di almeno 10 minuti sono gate complementari. Nessuno sostituisce playtest umano o benchmark.

I primi smoke test devono dimostrare movimento leggibile, un pattern, un telegraph e un fallback, prima di espandere il catalogo e il DSP. Le scelte creative ancora aperte sono elencate in DESIGN §24 con milestone limite.

### Milestone 0 — Repository, build e dependency inventory

**Obiettivo:** baseline Windows riproducibile.

Deliverable:

- struttura monorepo minima;
- `CMakePresets.json`;
- `docs/build.md`, con toolchain/generatore e versioni verificate;
- `docs/dependencies.md`;
- `docs/progress.md` con decisioni di ingresso ed evidenze (AGENTS §5.1);
- raylib pinned, modulo audio raylib disabilitato (§28.2);
- miniaudio snapshot pinned, unico owner del device;
- test framework scelto;
- target `cymatica_game` e `cymatica_tests`;
- finestra, input, tono audio e shader test;
- CI Windows se disponibile.

Accettazione:

- clean checkout configurabile e compilabile da comandi documentati;
- `ctest` esegue e supera almeno un test reale del framework scelto;
- eseguibile avviabile e chiudibile senza crash;
- tono udibile e shader di prova visibile, verificati manualmente con procedura registrata;
- un solo `MINIAUDIO_IMPLEMENTATION` e un solo device aperto nel binario finale, verificato su build e link;
- dipendenze e licenze censite;
- decisioni M0 registrate in `docs/progress.md` o ADR;
- nessun tool futuro richiesto.

### Milestone 1 — Tempo, seed e contratti deterministici

**Obiettivo:** fondazione del runtime non deterministico riproducibile.

Deliverable:

- `MusicClock` sample-based;
- fixed timestep;
- `MusicPosition`;
- `SeedBank` e `RandomKey` stable;
- stream IDs separati;
- `AudioTelemetryFrame` e `AudioControlFrame`;
- scambio thread-safe;
- `RunRecord` minimale;
- golden test seed/clock.

Accettazione:

- stesso seed produce stessi test vector;
- VFX stream non modifica gameplay stream;
- 60/120 FPS producono stato equivalente;
- snapshot concorrenti coerenti;
- zero allocation nella callback dopo init.

### Milestone 2 — Music Intent e musica procedurale v1

**Obiettivo:** prima forma musicale simbolica condivisa.

Deliverable:

- form/phrase skeleton;
- Pulso/Corpo/Trama/Vettore;
- Euclidean rhythm;
- scale/modi;
- motif memory minima;
- `MusicIntentEvent`;
- canale `SetTempoMap` sample-accurate e thread-safe (Game -> Audio) con acknowledgement per cambi tempo e time signature;
- scheduler ahead-of-time;
- synth/mix base;
- telemetria coerente.

Accettazione:

- musica continua 3 minuti senza crackle;
- almeno due sezioni distinguibili (inclusa transizione di tempo/metrica via `SetTempoMap`);
- intent e audio restano sincronizzati;
- stesso seed produce stesso event trace;
- la callback esegue, non pianifica.

**Estensione agentica (§31.5):** congelare D-AO-01/02 prima del task e introdurre il nucleo minimo `InputAction`/`ActionState` del tono comune a SPACE fisico e runner, con normalizzazione e applicazione sul coordinatore; il runner non scrive direttamente `AudioControlFrame`. `AC-AO-01A` è mandatory: runner hold/release, tap PCM/WAV, verifica quantitativa 220 → 440 → 220 Hz, manifest, gap/overflow e RT safety, più prova SPACE OS automatica oppure manuale con evidenza. `AC-AO-01B` è supplementare capability-dependent: audio consegnato al client e giudizio agente registrato; capacità esterna assente/bloccata non impedisce la chiusura M2 né produce PASS. Integrare AudioProbe con Music Intent (`AC-AO-02`, mandatory): zero allocazioni/blocchi, overflow e lifecycle verificati, onset correlati e limiti/capacità dell'adapter dichiarati. Eventuale frame statico diagnostico è opzionale e non anticipa il contratto VisualProbe M3. Questi nuovi gate non modificano i PASS storici M1.

### Milestone 3 — Pattern catalog e generatore costruttivo

**Obiettivo:** produrre livelli da Music Intent senza adattamento.

Deliverable:

- pattern schema;
- registry nativo;
- almeno cinque pattern family;
- transform chain;
- candidate generator;
- plan/event scheduler;
- bullet/barrier pools;
- telegraph primitives;
- debug view;
- tavola delle invarianti visive (`DESIGN.md` §13.0, §13.3) applicata anche ai placeholder.

Accettazione:

- catalogo validato al caricamento;
- pattern deterministici per key;
- nessun hazard privo di telegraph;
- budget entità rispettato;
- livello di 60–90 secondi giocabile con policy fissa.

**Estensione agentica (§31.5, AC-AO-03):** estendere il dispatcher comune M2 a movement/dash, scheduling per tick e ordine multi-source; VisualProbe player/debug con sequenze timestampate e audio. Accettazione: press/release e confini tick verificati, ciclo telegraph/attivazione documentato nella sequenza, gap/skew e interpolazione dichiarati. D-AO-03 prima dell'implementazione; non aspettare M5 per controllare lo spike.

### Milestone 4 — Fairness validator e headless simulation

**Obiettivo:** rifiutare violazioni di raggiungibilità nel modello dichiarato e misurare i suoi limiti.

Deliverable:

- occupancy forecast;
- reachability grid;
- movement/dash envelope;
- hard constraint gate;
- fairness metrics;
- runtime safety guard;
- headless simulation;
- property tests.

Accettazione:

- zero violazioni nelle fixture approvate e nei test del modello dichiarato, inclusi movimento tra campioni e composizione;
- candidati invalidi spiegati;
- safety guard non blocca il frame;
- fallback attivabile e musicale;
- batch di almeno 1.000 seed senza crash.

**Estensione agentica (§31.5, AC-AO-04):** step/run-until solo offline, scenari versionati (nome `.cymtest` proposto), replay e clip pre/post evento. Accettazione: hash coerenti su input/decisioni effettivi, rigetto late/epoch/capacity, confronto capture OFF/ON e incompletezza clip segnalata; il validator continua a valutare composizione/swept transitions e rifiutare unknown. Congelare D-AO-04.

### Milestone 5 — Player loop completo

**Obiettivo:** game feel centrale.

Deliverable:

- movimento;
- Dissonanza;
- Quantum Dash;
- safe landing assist;
- Graze;
- Risonanza;
- Drop Shock;
- particelle CPU;
- gamepad e mouse/tastiera;
- HUD diegetico minimo.

Accettazione:

- controlli leggibili e comparabili;
- nessuna modifica nascosta delle hitbox;
- dash affidabile;
- graze non farmabile banalmente;
- partita completa con game over/restart.

**Estensione agentica (§31.5, AC-AO-05):** osservazione live read-only della sessione umana, preview timestampata con clip locale e test OS/device end-to-end. Accettazione: disconnect non altera gli input umani, input agente rilasciati in sessione controllata, età/gap visibili; latenza e overhead OFF/ON/consumer lento misurati con tastiera/gamepad entro soglie fissate prima della misura in D-AO-05. Telemetria software non certifica latenza fisica input-to-photon/acoustic; regressioni M4 obbligatorie per cambi player.

### Milestone 6 — CIE Director v1 e livello adattivo

**Obiettivo:** trasformare la generazione in experience management.

Deliverable:

- `PlayerModel` multidimensionale;
- `ExperienceTarget`;
- pacing state machine;
- recovery debt;
- candidate scoring;
- `CandidateRetriever` (System 1, §11.7) con baseline deterministica a distanza pesata intera e top-K stabile;
- novelty memory;
- stochastic top-k selector;
- planning/commit horizon;
- planning worker/deadline;
- explanation trace;
- preset Standard e Pure Seed.

Accettazione:

- adattamento solo su boundary consentiti;
- nessun rubber-banding immediato;
- planner deadline miss gestito con fallback;
- trace spiega ogni scelta, inclusi candidati recuperati ed esclusi dal System 1;
- System 1 non ammette mai un candidato senza System 2: test che lo dimostrano anche con retriever volutamente errato;
- top-K identico a parità di input, policy e catalogo, indipendente dall'ordine di caricamento;
- Pure Seed produce replay stabile;
- target pressure e pressione misurata convergono entro tolleranza definita.

**Estensione agentica (§31.5, AC-AO-06):** esposizione read-only di PlayerModel/retrieval/scoring/trace, correlazione di input, scelte e interventi; exact replay completo. Spike bounded sul video/audio continuo (D-AO-06): report con costo readback/codifica, età osservazioni, gap/skew e supporto reale del client. Accettazione dello spike è un esito documentato, non l'obbligo di introdurre un codec o promettere riflessi LLM.

### Milestone 7 — Archetipi e vertical slice di qualità

**Obiettivo:** dimostrare l’identità di CYMATICA.

Deliverable:

- Sintetico e Organico completi;
- almeno un terzo archetipo dimostrativo;
- archetype policies data-driven;
- palette/motion secondo `DESIGN.md`;
- saturazione;
- transizioni;
- accessibility profile;
- performance pass, incluse le metriche System 1 di §26.1;
- sessione infinita di almeno 10 minuti.

Accettazione:

- archetipi riconoscibili a vista e nel gameplay, incluso il riconoscimento da screenshot senza HUD;
- firme invarianti e test di luminanza/daltonismo di `DESIGN.md` §13.0 e §13.3 superati;
- variazione senza perdita di leggibilità;
- run diverse mostrano diversità misurabile;
- nessun audio underrun nominale;
- 60 FPS minimo sul target Windows;
- vertical slice ritenuta divertente da playtest umano.

**Estensione agentica (§31.5, AC-AO-07):** corpus audiovisivo di sequenze dense, transizioni e profili accessibili, distinguendo player e debug. Accettazione: report percettivo supplementare e benchmark del candidato streaming con soglie congelate prima della misura, capacità client ed esito passed/failed/blocked espliciti; se non idoneo, conservare sequenze/clip e registrare il limite, senza dichiarare streaming consegnato. Nessuna sostituzione di playtest umano, hardware o criteri originali M7.

### Milestone 8 — AI Lab e quality-diversity

**Obiettivo:** costruire una libreria ampia di pattern validati.

Deliverable:

- `cymatica_lab_cli`;
- batch generation;
- descriptor extraction;
- MAP-Elites o equivalente;
- procedural personas;
- archive format;
- dashboard/report testuale;
- curation/blacklist;
- archive interrogabile dal `CandidateRetriever` (§11.7, §18.8);
- dataset di decisioni System 1/System 2 per Recall@K e per eventuali retriever appresi (§11.7.6).

Accettazione:

- archive con coverage misurabile;
- pattern valutati su più personas;
- runtime funziona senza archive;
- archive versionato e validato;
- nessun candidato invalido entra come elite;
- Recall@K del retriever baseline misurato contro il planner di riferimento, con K e soglia fissati prima della misura.

**Estensione agentica (§31.5, AC-AO-08):** il Lab riusa contratto/scenari e manifest delle sonde; risultati indicizzati per versioni/seed/persona e informazioni disponibili al controller. Accettazione: confronti riproducibili e corpus senza leakage, controller privilegiati/percettivi distinti e runtime indipendente da agenti/adapter esterni.

### Milestone 9 — Pacchetti, timeline e tool CLI

**Obiettivo:** preparare contenuti finiti e custom.

Deliverable:

- `.cymlevel` schema;
- loader;
- migration/versioning base;
- `validate`, `inspect`, `package`;
- custom level index;
- generated finite export;
- generation trace opzionale.

Accettazione:

- fixture valida caricata;
- fixture corrotta rifiutata chiaramente;
- Generated Track esportabile e rigiocabile;
- tool non richiesto per compilare game.

### Milestone 10 — Custom audio mix-only

**Obiettivo:** generare livelli da brani senza stem.

Deliverable:

- FFmpeg integration tool-only;
- duration/loudness/beat/onset/feature extraction;
- section segmentation iniziale;
- Music Intent timeline con confidence;
- mapping a quattro ruoli approssimato;
- fallback robusti.

Accettazione:

- più generi producono livelli caricabili;
- assenza di beat stabile gestita;
- confidence guida la conservatività;
- nessuna dipendenza tool entra nel game runtime.

### Milestone 11 — NatuStem spike e stem separation opzionale

**Obiettivo:** valutare e aggiungere separazione come miglioramento, non requisito.

Deliverable:

- report NatuStem;
- decisione backend;
- manifest ingestion;
- optional audio-separator/ONNX path;
- CPU-only baseline;
- cache;
- fallback mix-only;
- license/model review.

Accettazione:

- fallimento stem non impedisce il pacchetto;
- modelli non committati;
- output e provenance registrati;
- runtime game indipendente.

### Milestone 12 — Android technical preview

**Obiettivo:** verificare portabilità reale.

Deliverable:

- NDK/Gradle build;
- input touch/controller;
- device audio tests;
- quality tiers;
- performance profile;
- fallback fixed tick/particle/shader.

Accettazione:

- app avviabile su device reale;
- audio stabile;
- latenza misurata;
- 60 FPS sul profilo target o backlog documentato;
- CIE rispetta budget ridotti.

### Milestone 13 — Studio/editor

**Obiettivo:** offrire authoring visuale dopo la stabilità dei formati.

Possibili stack:

- C++/raylib per UI minima;
- Flutter desktop per UI complessa;
- web app separata;
- altra soluzione valutata con prototipo.

Nessuna scelta definitiva prima di M9–M10.

---

## 33. Backlog prioritizzato

### P0 — Fondazione e vertical slice

- M0–M7;
- clock/seed/replay;
- Music Intent;
- pattern grammar;
- validator;
- CIE v1;
- player model;
- tre archetipi;
- test e trace.

### P1 — Robustezza generativa

- AI Lab;
- quality-diversity;
- personas;
- archive;
- long-run repetition metrics;
- accessibility tuning;
- challenge presets.

### P2 — Contenuti finiti/custom

- `.cymlevel`;
- tool CLI;
- Generated Track;
- mix-only custom;
- NatuStem spike;
- stem separation opzionale.

### P3 — ML avanzato

- contextual bandit;
- surrogate fitness;
- learned player model;
- section/archetype classifier;
- music continuation model;
- automatic curation support.

### P4 — Espansione

- Android;
- editor;
- community sharing;
- co-op locale;
- leaderboard Pure Seed;
- modding sicuro.

---

## 34. Decision log

| Decisione | Stato | Motivazione |
|---|---:|---|
| Procedurale infinito prima del custom | Accettata | valida il nucleo del gioco |
| CIE ibrido, non modello neurale monolitico | Accettata | controllo, spiegabilità e fallback |
| Livello come policy a orizzonte mobile | Accettata | adattamento senza perdere coerenza |
| Music Intent condiviso da audio e gameplay | Accettata | sincronizzazione semantica |
| Hard constraints prima dello scoring | Accettata | fairness non negoziabile |
| Player model multidimensionale | Accettata | abilità non riducibile a un numero |
| Adattamento con isteresi e limiti | Accettata | evita rubber-banding |
| Planning worker fuori audio/game hot path | Accettata | deadline e stabilità realtime |
| Fallback prevalidati | Obbligatori | continuità in caso di errore/ritardo |
| Seed stream indipendenti | Obbligatori | isolamento del non determinismo |
| Exact e semantic replay distinti | Accettata | debug vs condivisione |
| Counter/index-based random keys | Raccomandata | stabilità rispetto all’ordine delle chiamate |
| Pattern data-driven, generatori nativi | Accettata | sicurezza e testabilità |
| Scripting runtime utente iniziale | Rifiutato | superficie di rischio inutile |
| Reachability validator conservativo | Obbligatorio | prevenzione pattern inevitabili |
| MAP-Elites/QD offline | Accettato come M8 | diversità di qualità |
| Procedural personas | Accettate come lab | playtest scalabile |
| ML/ONNX nel vertical slice | Rifiutato | non necessario e rischioso |
| ML come componente opzionale | Accettato | beneficio misurabile e fallback |
| C++20/CMake/raylib/miniaudio | Accettato | stack core |
| Sistema particellare custom | Accettato | identità visuale e controllo |
| Motore fisico esterno core | Rifiutato | non necessario |
| Tool nello stesso repo, dist separata | Accettato | condivisione formati senza bloat |
| NatuStem come riferimento | Accettato | accelera future decisioni |
| Strudel/Tidal come riferimento | Accettato | grammatica musicale, non runtime |
| `DESIGN.md` separato | Accettato | elimina ridondanza e rende normativo il design |
| System 1 / System 2 nel CIE (§11.7) | Accettata per M6 | riduce le validazioni costose senza delegare la sicurezza |
| System 1 baseline = distanza pesata intera su descriptor di dominio | Accettata per M6 | deterministica, ispezionabile, adatta a Pure Seed, nessuna dipendenza |
| Encoder testuali (BGE, MiniLM o simili) nel runtime CIE | Rifiutato | nessuna informazione utile oltre ai descriptor strutturati; costo, replay, Android |
| Encoder semantici per ricerca nel catalogo da tooling/editor | Differito post-M9, tool-only | utile per query in linguaggio naturale; valutare modelli multilingua |
| Retriever/scorer appreso | Differito post-M8 | ammesso solo se batte la baseline secondo §11.7.6 |

---

## 35. Rischi residui e mitigazioni

| Rischio | Impatto | Mitigazione |
|---|---:|---|
| musica procedurale monotona | alto | form planner, motif memory, curation, sound design |
| generatore “random ma senza intenzione” | alto | pacing, ExperienceTarget, planning horizon |
| pattern unfair | critico | hard gate, reachability, safety guard, personas |
| adattamento percepito come trucco | alto | isteresi, boundary, preset, nessuna modifica retroattiva |
| planner in ritardo | alto | worker bounded, archive, fallback, metriche |
| replay instabile | medio | versioning, RandomKey, exact trace, golden tests |
| esplosione combinatoria | alto | grammar compatibility, pruning, budget, QD offline |
| AI non spiegabile | medio | explanation record, debug overlay, trace |
| player model errato | alto | confidence, fallback, limiti, disattivazione |
| costo validator | medio | griglia coarse, cache, headless profiling |
| visual spettacolari ma illeggibili | critico | `DESIGN.md`, layer semantici, accessibility |
| dipendenze/licenze | medio | inventory, pinning, review |
| Android audio/performance | alto | differimento, device tests, quality tiers |
| dataset insufficiente per ML | medio | core non-ML, non addestrare senza obiettivo |
| custom music con analisi debole | medio | confidence, mix-only fallback, tool offline |

---

## 36. Open questions

Le seguenti decisioni non bloccano M0, ma vanno risolte prima della milestone indicata.

1. ~~Quale test framework C++ adottare? — M0.~~ Risolta: Catch2 v3, ADR-0001.
2. Quale parser/schema JSON adottare? — M0/M1.
3. Confermare il profilo 120 Hz iniziale e i criteri di compatibilità per eventuali profili 60 Hz. — M1; rivalidazione M5.
4. Quale algoritmo stable random interno usare? — M1.
5. Quanto deve durare il commit horizon per BPM estremi? — M3/M4.
6. Quale discretizzazione reachability offre il miglior compromesso? — M4.
7. Congelare regola del dash, disponibilità e obbligatorietà per preset. — prima della giocabilità M3 e della validazione M4; tuning M5.
8. Quale metrica definisce pressione osservata? — M6.
9. Quanto adattamento è comunicato esplicitamente al giocatore? — M6/DESIGN.
10. Quali coppie di archetipi possono fondersi entro il contratto a policy primaria unica di §17.6? — M7.
11. Quali dimensioni QD sono davvero ortogonali e utili? — M8.
12. Quale algoritmo QD implementare o importare? — M8.
13. Quali personas minime correlano con playtest umani? — M8.
14. Licenza definitiva di CYMATICA? — prima di distribuzione pubblica.
15. Condivisione di pacchetti contenenti audio protetto? — M9/M10.
16. Backend MIR e stem? — M10/M11.
17. Editor C++, Flutter o web? — M13.
18. Dimensioni, normalizzazione e quantizzazione del `RetrievalDescriptor`, e come stimarlo senza generare gli eventi completi. — M6 (§11.7).
19. Valori iniziali di K, quote di diversità e soglia Recall@K di accettazione. — K e quote in M6; soglia fissata prima della misura M7/M8.
20. Un retriever appreso supera la baseline deterministica secondo §11.7.6? — dopo M8, solo con dataset M8.

---

## 37. Criterio di successo della vertical slice

La vertical slice non è riuscita soltanto perché genera musica e proiettili. È riuscita quando:

- il giocatore riconosce una forma musicale;
- il livello sembra reagire senza barare;
- due run hanno identità diversa ma pari leggibilità;
- il ritmo anticipa il pericolo;
- il director alterna pressione e recupero;
- nessuna violazione rilevata nel corpus dichiarato e nessun candidato unknown ammesso; i limiti del modello e del playtest sono documentati;
- un bug è riproducibile tramite run record;
- il planner può fallire senza interrompere la partita;
- Sintetico e Organico risultano diversi nel suono, nella forma e nella strategia;
- il gioco resta divertente dopo più retry, non soltanto sorprendente al primo avvio.

---

## 38. Riferimenti tecnici e scientifici

Questi riferimenti supportano le scelte progettuali; non sono dipendenze automatiche.

- Mouret, J.-B.; Clune, J. — *Illuminating Search Spaces by Mapping Elites*: https://arxiv.org/abs/1504.04909
- Khalifa, A. et al. — *Talakat: Bullet Hell Generation through Constrained Map-Elites*: https://arxiv.org/abs/1806.04718
- Yannakakis, G. N.; Togelius, J. — *Experience-Driven Procedural Content Generation*: https://www.um.edu.mt/library/oar/handle/123456789/29274
- Summerville, A. et al. — *Procedural Content Generation via Machine Learning*: https://arxiv.org/abs/1702.00539
- Holmgård, C. et al. — *Automated Playtesting with Procedural Personas through MCTS with Evolved Heuristics*: https://arxiv.org/abs/1802.06881
- Salmon, J. et al. — *Parallel Random Numbers: As Easy as 1, 2, 3*: https://random123.com/
- TidalCycles documentation, patterning e randomness: https://tidalcycles.org/docs/
- Strudel: https://strudel.cc/
- raylib: https://www.raylib.com/
- miniaudio: https://miniaud.io/
- Android Oboe low-latency audio: https://developer.android.com/games/sdk/oboe/low-latency-audio
- ONNX Runtime mobile: https://onnxruntime.ai/docs/tutorials/mobile/
- NatuStem: https://github.com/naturewhisp/NatuStem
- audio-separator: https://github.com/nomadkaraoke/python-audio-separator

---

## 39. Raccomandazione finale

La prossima implementazione non deve partire da un “modello AI” generico. Deve partire da quattro fondazioni verificabili:

1. **Music Intent condiviso**;
2. **randomness riproducibile e isolata**;
3. **pattern data-driven con hard constraints**;
4. **planner a orizzonte mobile con fallback**.

Soltanto dopo queste basi il player model e l’adattamento hanno uno spazio sicuro in cui operare. L’AI di CYMATICA deve essere riconoscibile non perché usa una rete neurale, ma perché costruisce intenzionalmente una sessione musicale e ludica coerente, varia, responsiva e spiegabile.

