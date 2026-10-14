# ADR-0003 — Contratti M2: Music Intent, scheduler, dispatcher del tono e AudioProbe

- **Data:** 2026-10-12
- **Stato:** accepted for implementation
- **Milestone:** M2 — Music Intent e musica procedurale v1 (Sottofase M2.1)
- **Documenti correlati:** `CYMATICA_Specifica_Agentica_Sviluppo.md` §7.5, §16, §20.5–§20.8, §31.5–§31.6, §32; `DESIGN.md` §17.4.1; `docs/agent_control_observability.md` §4, §6, §9, §12, §13; `docs/progress.md`

---

## 1. Contesto

La Milestone 2 stabilisce la prima forma musicale simbolica condivisa di CYMATICA e introduce il controllo agentico e l'osservazione audiovisiva incrementale (CACP, Spec §31.5).
In ottemperanza a `AGENTS.md` §1.1 e alle sottofasi **M2.1** e **M2.4** (Spec §32), questo ADR formalizza i contratti architetturali vincolanti di Milestone 2:
- Per **M2.1** (congelate all'avvio di M2):
  1. **D-M2-01:** Canale thread-safe `SetTempoMap` (Spec §20.7, §32);
  2. **D-M2-02:** Rappresentazione simbolica `MusicIntentEvent` per i 4 ruoli musicali (Spec §16, §20.5, §32);
  3. **D-M2-03:** Voice budget polifonico e scheduling ahead-of-time (Spec §20.7, §32);
  4. **D-AO-01:** Nucleo comune del dispatcher input per il tono (`InputAction`/`ActionState`) e runner locale CACP (Spec §31.5, §32, Analisi CACP §4, §9, §12);
  5. **D-AO-02:** Tap AudioProbe e profilo di verifica del tono `tone_probe_v0` (Spec §31.5, §32, Analisi CACP §6, §9, §12).
- Per **M2.4** (contratti d'ingresso deliberati il 2026-10-14 per la musica procedurale v1 e il synth a 8 voci):
  6. **D-M2-04:** Allocazione polifonica a 8 voci, voice stealing bounded (crossfade 2 ms), note aperte, continuità cross-epoch e matrice stati di trasporto;
  7. **D-M2-05:** Architettura compositiva del Music Planner gerarchico, bounded candidate search e profilo sperimentale fixture A-B-A';
  8. **D-M2-06:** Architettura DSP dei 4 ruoli, 4 bus dedicati, gain staging iniziale e protezione master bounded;
  9. **D-M2-07:** Profiling realtime DSP (target $\le 2,0\text{ ms}$ / 512 frame) e protocollo di validazione per handoff M2.5.

---

## 2. Decisioni

### D-M2-01: Canale thread-safe `SetTempoMap` (Spec §20.7, §32)

- **Problema:** In M1 `AudioEngine` opera a 120 BPM costanti in 4/4. In M2 le sezioni musicali introducono cambi di tempo e metrica che devono essere comunicati dal Coordinator/Game thread alla callback audio in modo sample-accurate, deterministico, lock-free e senza alcuna allocazione dinamica.
- **Opzione scelta:** Estensione di `AudioCommand` con tipo `AudioCommandType::SetTempoMap`.
  - **Payload compatto (trivially copyable):**
    `AudioCommand` incapsula numeratore e denominatore BPM (uint32 ciascuno nel campo `paramU64`) e i valori di time signature (`beatsPerBar` e `timeSignatureDen` compattati in `paramF32` o campi interi).
  - **Esecuzione nella callback audio (`processBlock`):**
    1. Verifica la validità con `MusicClock::isSupportedTempoMap`. Se non valida, invia immediatamente `AudioCommandAck` con stato `AudioCommandStatus::Rejected`.
    2. Se valida, applica la nuova `TempoMap` all'inizio del blocco corrente o al sample frame target indicato nel comando.
    3. Incrementa `transportEpoch` e pubblica l'aggiornamento atomico nella telemetria `AudioTelemetryFrame`.
    4. Emette `AudioCommandAck` con `status = AudioCommandStatus::Applied`, `sampleFrame = renderCursor`, `transportEpoch = transportEpoch`.
- **Alternative considerate:**
  - *`TripleBuffer<TempoMap>`:* Scartato per i cambi di tempo discreti: non garantisce l'ordinamento sample-accurate con gli eventi musicali né fornisce l'acknowledgement esplicito richiesto da Spec §20.7.
  - *Scrittura diretta con mutex:* Vietato categorizedmente dai vincoli hard-realtime della callback (Spec §20.2).
- **Stato:** `accepted for implementation`.

---

### D-M2-02: Rappresentazione simbolica `MusicIntentEvent` (Spec §16, §20.5, §32)

- **Problema:** Definire la struttura dati simbolica attraverso cui il pianificatore musicale esprime le intenzioni generative per i 4 ruoli musicali indipendenti, mantenendo determinismo, isolamento dei seed e compatibilità con lo scheduler.
- **Opzione scelta:**
  - **Ruoli musicali (`MusicRole`):**
    - `Pulso`: ancoraggio ritmico, percussioni gravi, griglia euclidea fondamentale;
    - `Corpo`: armonia, accordi/pad, linea di basso tonale, definizione modale;
    - `Trama`: arpeggi, decorazioni poliritmiche, micro-contrappunto, texture;
    - `Vettore`: lead direzionale, ascesa/discesa melodica, sweep di tensione.
  - **Struttura `MusicIntentEvent`:**
    ```cpp
    struct MusicIntentEvent {
        std::uint64_t eventId{0};
        std::uint64_t sampleFrame{0};      // Timeline logica 48 kHz
        std::uint64_t tick{0};             // Tick simulazione fixed-step 120 Hz
        MusicRole role{MusicRole::Pulso};
        float pitchHz{220.0f};             // Frequenza nominale
        float velocity{1.0f};              // Intensità normalizzata in [0.0f, 1.0f]
        std::uint32_t durationFrames{0};   // Durata in campioni
        float timbreParam{0.0f};           // Morfologia/filtro per il ruolo in [0.0f, 1.0f]
    };
    ```
  - **Generazione ritmica euclidea:** Algoritmo puro di Bjorklund `euclideanRhythm(steps, pulses, rotation)` con array statici preallocati (`std::array<bool, 32>`), senza heap allocation.
- **Alternative considerate:**
  - *Note MIDI standard:* Scartato per limitazioni di risoluzione dinamica e disallineamento concettuale rispetto ai 4 ruoli cimatica/acustici.
- **Stato:** `accepted for implementation`.

---

### D-M2-03: Voice budget e DSP scheduling ahead-of-time (Spec §20.7, §32)

- **Problema:** Regolare l'invio degli eventi audio dal Coordinator verso `AudioEngine` per prevenire overload computazionali o buffer underrun.
- **Opzione scelta:**
  - **Finestra di lookahead:** Lo scheduler opera anticipando l'invio degli eventi tra 100 ms e 250 ms (valore nominale iniziale: $150\text{ ms} = 7.200\text{ campioni} = 18\text{ tick}$).
  - **Canale `ScheduledAudioEvent`:** `SpscQueue<ScheduledAudioEvent, 256>` con il Coordinator come **unico producer autorizzato** (Spec §20.7).
  - **Voice budget polifonico massimo:** 8 voci simultanee preallocate a tempo di compilazione nel motore audio:
    - 2 voci per `Pulso`;
    - 2 voci per `Corpo`;
    - 2 voci per `Trama`;
    - 2 voci per `Vettore`.
  - **Policy di saturazione:** Se una voce per un ruolo è satura, si applica voice-stealing sulla voce con inviluppo più avanzato con micro-fadeout di 2 ms (senza pop), registrando un contatore `voiceDropCount` nella telemetria senza mai bloccare la callback.
  - **Evoluzione polifonica M2.4 (Rinvio normativo):**
    I dettagli implementativi dell'allocatore a 8 voci, voice stealing bounded con steal tails, note aperte, continuità cross-epoch e matrice stati di trasporto sono formalizzati in **D-M2-04**; i vincoli di budget realtime ($\le 2,0\text{ ms}$) in **D-M2-07**.
- **Stato:** `accepted for implementation`.

---

### D-AO-01: Nucleo dispatcher comune del tono (`InputAction`/`ActionState`) e runner CACP M2 (Spec §31.5, §32)

- **Problema:** Garantire che il controllo del tono (spazio / 220 $\to$ 440 $\to$ 220 Hz) passi esattamente dallo stesso percorso di normalizzazione e applicazione sia quando azionato dal giocatore (tastiera fisica SPACE) sia quando azionato da un runner diagnostico di sviluppo (CACP), eliminando percorsi agent-only o scritture dirette a `AudioControlFrame`.
- **Opzione scelta:**
  - **Definizione azioni:**
    ```cpp
    enum class InputAction : std::uint8_t {
        ToneShift = 0  // Estensibile in M3 con MoveX, MoveY, Dash
    };
    enum class ActionState : std::uint8_t {
        Pressed,
        Held,
        Released
    };
    enum class ActionSource : std::uint8_t {
        PhysicalInput,
        AgentRunner
    };
    struct NormalizedAction {
        InputAction action{InputAction::ToneShift};
        ActionState state{ActionState::Released};
        ActionSource source{ActionSource::PhysicalInput};
        std::uint64_t targetTick{0};
    };
    ```
  - **`InputDispatcher`:**
    - Istanza gestita esclusivamente sul thread di simulazione/coordinamento.
    - Normalizza gli input fisici (raylib `IsKeyPressed`, `IsKeyDown`, `IsKeyReleased`).
    - Accetta batch finiti da un runner diagnostico CACP.
    - Gestisce la mutua esclusione: durante una sessione di test diagnostico, gli ingressi fisici sono disattivati (o la sorgente attiva è esplicitamente dichiarata) per prevenire race non deterministiche.
    - Applica l'azione normalizzata consolidando `masterControl_` in `AudioEngine` (220 Hz su `Released`, 440 Hz su `Held`/`Pressed`).
  - **Runner locale M2 (Control Plane):**
    Modalità di esecuzione autonoma (es. `--cacp-run tone_test`) a scenario finito, senza dipendenze da server MCP esterni o browser. Produce artefatti e manifest in locale.
- **Stato:** `accepted for implementation`.

---

### D-AO-02: Tap AudioProbe e profilo di verifica `tone_probe_v0` (Spec §31.5, §32)

- **Problema:** Verificare quantitativamente il PCM sintetizzato e la reazione al comando di cambio tono senza violare le restrizioni della callback realtime.
- **Opzione scelta:**
  - **Architettura AudioProbe:**
    - Tap SPSC lock-free collocato al termine di `AudioEngine::Impl::processBlock` subito dopo la scrittura dei campioni nel buffer del device.
    - Ring buffer preallocato lock-free (`AudioProbeRingBuffer`) con capacità di $24.000$ campioni stereo float32 ($0.5\text{ secondi} = 192\text{ KB}$).
    - Zero allocazioni, zero lock, zero file I/O nella callback realtime. Se il worker esterno è lento, un contatore atomico incrementa `probeDropCount` senza bloccare l'uscita audio.
    - Worker di esportazione asincrono: legge dalla ring buffer, costruisce la traccia continua ed esporta un file WAV stereo a 48 kHz standard più un manifest JSON correlato.
  - **Profilo di test `tone_probe_v0` (accepted for implementation):**
    - Durata nominale: 3.0 secondi totali (1.0 s baseline a 220 Hz, 1.0 s hold a 440 Hz, 1.0 s release a 220 Hz).
    - Finestre di analisi per pitch estimation: 250 ms per ciascuna fase, escludendo i primi 100 ms successivi alla transizione.
    - Tolleranza quantitativa: $\pm 2\text{ Hz}$ rispetto alle frequenze target (218–222 Hz per baseline/release, 438–442 Hz per hold).
    - Livello minimo RMS: segnale non silente ($> -40\text{ dBFS}$).
  - **Criteri di accettazione distinti:**
    - **`AC-AO-01A` (Mandatory, progetto):** dispatcher comune, tap PCM generato con successo, zero heap alloc e zero lock verificati anche con probe worker bloccato, frequenze 220 $\to$ 440 $\to$ 220 Hz convalidate quantitativamente, prova fisica SPACE (OS) documentata.
    - **`AC-AO-01B` (Capability-dependent, supplementare):** consegna del file/audio al client agente e registrazione del giudizio percettivo. Se la capacità audio nel client è assente, l'esito è registrato come `unsupported` e non blocca la chiusura di M2.
  - **Nota metodologica sui finding del critic (§31.6 vs D-AO-04):**
    In M2 i finding del tooling/critic esistono come evidenze e report discorsivi di test. La formalizzazione dello **Structured Evaluation Result** machine-readable avverrà in M4 con D-AO-04, evitando l'introduzione prematura di schemi machine-readable intermedi in M2/M3.
- **Stato:** `accepted for implementation`.

---

### D-M2-04: Allocazione polifonica a 8 voci, Voice Stealing Bounded, Note Aperte, Continuità Cross-Epoch e Matrice Stati di Trasporto (Spec §16.9, §20.7, §20.8, §32)

- **Problema:** Superare il modello monofonico temporaneo di M2.2/M2.3 (`activeNoteEpoch_`, timeout forzato a 1s) implementando l'allocazione polifonica a 8 voci, il ciclo di vita delle note sostenute (pad di `Corpo`), la gestione deterministica del voice stealing senza click, la riconciliazione causale dei `NoteOff` cross-epoch e la semantica non distruttiva di `Pause`/`Resume` rispetto a `Stop`/`Reset`.
- **Invarianti architetturali normativi (`accepted for implementation`):**
  1. **Pool polifonico fisso a 8 voci:** 8 slot statici preallocati a tempo di compilazione (`VoiceSlot[8]`), ripartiti rigidamente in 2 voci per ruolo:
     - 2 voci per `Pulso` (percussione/kick/transient);
     - 2 voci per `Corpo` (armonia/pad/basso);
     - 2 voci per `Trama` (arpeggi/micro-contrappunto);
     - 2 voci per `Vettore` (melodia lead/sweep).
     Nessuna allocazione dinamica, nessun pool dinamico condiviso, nessuna espansione di voci composite senza revisione formale dell'ADR.
  2. **Identità e accoppiamento delle note:** Correlazione biunivoca rigorosa su `(role, eventId)`. Ogni voice slot conserva: ruolo, `activeEventId`, stato di inviluppo (`Idle`, `Attack`, `Decay`, `Sustain`, `Release`), fase dell'oscillatore, frequenza, parametri DSP e un `generationToken` numerico interno (uint32). Il `generationToken` viene incrementato ad ogni nuova allocazione o furto; un `NoteOff` tardivo o riferito a un occupante precedente viene scartato senza mai interrompere la nuova nota. Gli `eventId` sono generati come identificatori univoci e monotoni all'interno della run dal Coordinator.
  3. **Durata nominale e rilascio failsafe:** Per eventi con `durationFrames > 0`, la scadenza nominale è calcolata come `sampleFrame + durationFrames` sulla timeline logica interna a 48 kHz (con protezione anti-overflow saturante). Lo scheduler emette il `NoteOff` programmato, mentre lo slot implementa un timer failsafe interno su tale scadenza assoluta: se il `NoteOff` previsto viene perso per drop effettivo o anomalie (distinguendo la normale backpressure SPSC dello scheduler, che ritenta il dispacciamento degli eventi staged, dal drop reale o mancata emissione), la voce entra comunque in Release evitando note appese indefinitamente. L'applicazione del `NoteOff` esplicito e del failsafe interno è strettamente idempotente.
  4. **Semantica delle note aperte (`durationFrames == 0`), lease deterministico e recoverability:**
     - `durationFrames == 0` costituisce un gate aperto con rilascio gestito unicamente da un `NoteOff` esplicito. Il planner M2.4 ne limita l'uso iniziale ai pad sostenuti di `Corpo`, senza tuttavia introdurre un divieto strutturale irreversibile nel contratto pubblico.
     - La rimozione del timeout monofonico arbitrario di 1s (48.000 campioni) consente ai pad di rimanere legittimamente nello stadio di `Sustain` attraverso confini di misura, frase ed epoch.
     - *Contratto di lease e ownership deterministica:* la recoverability di una nota aperta durante una run attiva non può fare affidamento esclusivo su eventi contingenti (come futuri furti di voce o lo Stop finale). Il planner gestisce un lease esplicito per ogni nota aperta: la nota appartiene all'orizzonte di una frase/sezione e, qualora l'intenzione compositiva non preveda l'estensione del sustain oltre la cadenza o il confine di sezione, il planner è contrattualmente responsabile dell'emissione del `NoteOff` matching di chiusura.
     - *Recupero deterministico su anomalie:* in caso di drop del `NoteOff` in assenza di nuovi eventi del ruolo, la nota viene forzatamente rilasciata ai confini strutturali di sezione dal Coordinator, oppure resettata in caso di invalidazione del piano, `Stop`, `Reset` o Nuova Run. Se giungono nuove note sullo stesso ruolo, interviene la deterministica policy di voice stealing.
  5. **Voice Stealing Bounded con Steal Tails statiche:**
     - Al frame campionario esatto del nuovo `NoteOn`, la voce subentrante attacca immediatamente il suo inviluppo, preservando l'onset ritmico sample-accurate senza alcun ritardo.
     - La voce uscente viene trasferita su una *steal tail* secondaria, statica e preallocata all'interno dello slot, che applica una rampa lineare discendente a zero di esattamente 2 ms (96 campioni a 48 kHz).
     - La steal tail preserva lo stato della forma d'onda uscente (fase, ampiezza istantanea, stato del filtro) sfumandolo nel buffer dello slot con il segnale della nuova voce.
     - Limite rigido: esiste al massimo 1 steal tail attiva per slot (massimo 8 steal tails simultanee nel sistema, preallocate senza heap allocation).
     - *Saturazione (steal su steal entro 96 campioni):* se interviene un ulteriore furto sullo stesso slot prima dell'estinzione dei 96 campioni, la steal tail precedente viene degradata in modo deterministico con rampa continua ripartendo dall'ampiezza istantanea d'uscita corrente (evitando clamp discontinui rigidi) e riassegnata alla nuova voce uscente, registrando `voiceDropCount` nella telemetria. Non si garantisce aprioristicamente 'zero click' per transizioni ravvicinate estreme; la bontà del de-click va validata con misure PCM e ascolto.
     - *Ordinamento deterministico totale di selezione (Voice Allocator):* se entrambi i 2 slot del ruolo sono attivi, la selezione dello slot da rubare segue un ordinamento totale e deterministico privo di ambiguità:
       1. Priorità 1: slot in stadio di `Release` (tra due in Release, quello con ampiezza istantanea minore; i valori floating-point non finiti `NaN`/`Inf` sono bonificati e trattati come 0.0f per evitare ordinamenti instabili);
       2. Priorità 2: slot con inviluppo nello stadio più anziano o attivo da più tempo (`sampleFrame` di avvio più remoto);
       3. Tie-break finale canonico: a parità assoluta di stato, ampiezza ed età, viene selezionato lo slot con indice minore (`slotIndex` 0 prima di 1).
  6. **Confine di commitment e riconciliazione SPSC cross-epoch:**
     - *Mutable Planning Horizon:* frasi e sezioni future mantenute nelle strutture del Coordinator/Planner, non ancora trasferite alla SPSC; liberamente modificabili, invalidabili o ripianificabili in caso di cambio metrica/tempo (`SetTempoMap`) o interventi di gameplay.
     - *Committed Horizon:* eventi già trasferiti nella `SpscQueue<ScheduledAudioEvent, 256>` nella finestra di lookahead nominale (150 ms = 7.200 campioni = 18 tick a 120 Hz). Una volta inseriti nella SPSC, gli eventi sono **garantiti** al loro `sampleFrame` nominale.
     - *Semantica di SetTempoMap (non distruttivo) e migrazione M2.4:* su transizione di tempo/metrica con incremento di `transportEpoch`, le note già attive proseguono inalterate; i `NoteOff` cross-epoch matching per `(role, eventId)` con epoch precedente vengono regolarmente onorati al loro frame nominale; gli eventi `NoteOn` futuri già committed nella SPSC mantengono il loro `sampleFrame` garantito e non vengono scartati. Il planner ripianifica unicamente le intenzioni nel Mutable Planning Horizon (non committed) adattandole alla nuova mappa di tempo.
     - *Nota di migrazione rispetto a M2.3:* l'implementazione storica di M2.3 (`engine/audio/audio_engine.cpp`, righe 382-418) scarta indistintamente tutti i `NoteOn` con `transportEpoch < currentEpoch`. Questa logica legacy sarà formalmente migrata in M2.4 per distinguere i cambi non distruttivi (`SetTempoMap`, `Pause/Resume`) dalle vere revoche distruttive (`Stop`, `Reset`, nuova run). I test storici (in particolare `[scheduler][epoch_drop]`, nato per validare il drop dopo Pause/Resume) saranno riesaminati e motivatamente aggiornati durante lo sviluppo di M2.4, preservando la copertura contro eventi obsoleti solo per le reali invalidazioni distruttive.
  7. **Matrice Stati di Trasporto (`Pause`/`Resume` vs `Stop`/`Reset`):**
     - *Separazione tra tempo logico e uscita fisica:*
     - `Pause` (Non distruttivo / Freeze logico immediato): all'accettazione del comando `Pause`, il cursore logico, la pianificazione, il consumo dalla SPSC, le fasi degli oscillatori, gli inviluppi ADSR, gli LFO e le scadenze nominali si congelano **immediatamente** al frame campionario del comando. Non si ritarda il congelamento logico di 96 frame. Per prevenire click acustici, viene generato un **output-only de-click tail** fisico di 2 ms (96 campioni a 48 kHz) applicato esclusivamente nel buffer d'uscita del device (attenuando gli ultimi campioni renderizzati). Tale tail fisica non consuma eventi SPSC, non fa avanzare la composizione e non garantisce di per sé assenza percepita di click o DC offset senza verifica PCM ed ascolto. Durante `Paused` il motore emette silenzio numerico (0.0f). Al `Resume`, il motore riprende l'avanzamento esattamente dal medesimo frame logico congelato, applicando un fade-in fisico di 2 ms tornando a `Running`.
     - *ACK, sequenze ravvicinate e ordinamento comandi:* `AudioEngine` pubblica `AudioCommandAck` per ciascun comando. Se un `Pause` e un `Resume` arrivano nello stesso blocco o a distanza ravvicinata, la sequenza viene risolta in modo deterministico applicando lo stato risultante; comandi `Stop` o `Emergency Mute` hanno precedenza e annullano le transizioni di pausa in corso. Se il sample rate del device differisce da 48 kHz, la durata della tail fisica viene scalata ai frame effettivi del device senza alterare il clock logico a 48 kHz.
     - `Stop` (Distruttivo al boundary): comando `Stop` $\to$ de-click d'uscita rapido $\to$ reset forzato di tutti gli 8 slot a `Idle` (fasi azzerate, inviluppi spenti, `generationToken` incrementato), invalidazione e drenaggio bounded della SPSC e delle code comandi, invalidazione del piano nel Coordinator. Nessun evento invalidato viene consumato.
     - `Emergency Mute`: azzeramento immediato del buffer audio senza fade per errori fatali o terminazione forzata.
- **Stato:** `accepted for implementation (review remediation pending)`.

---

### D-M2-05: Architettura Compositiva del Music Planner gerarchico, Bounded Candidate Search e Profilo Sperimentale Fixture A-B-A' (Spec §14, §15.9, §16.2–§16.6, §32)

- **Problema:** Strutturare il generatore compositivo procedurale per garantire forma, memoria tematica, sviluppo cadenzale e continuità armonica su run estese ($\ge 180\text{ s}$), mantenendo determinismo assoluto, isolamento dei seed ed esecuzione confinata fuori dalla callback realtime.
- **Invarianti architetturali normativi (`accepted for implementation`):**
  1. **Pipeline gerarchica multilivello:** `FormPlan` $\to$ `SectionPlan` $\to$ `PhrasePlan` $\to$ `Motif` $\to$ `Bounded Candidate Generator` $\to$ `Hard Constraints` $\to$ `Deterministic Scoring` $\to$ `MusicIntentEvent[]`. Tutte le entità risiedono esclusivamente nel thread del Coordinator. `MusicIntentEvent` (ADR-0003 D-M2-02) rimane l'IR pubblico invariato.
  2. **Rappresentazione simbolica relativa del Motivo (`Motif`):**
     - Struttura puramente interna al planner: identificatore semantico `motifId`, contorno melodico espresso in gradi di scala relativi rispetto alla fondamentale corrente, durate razionali rispetto alla suddivisione metrica, maschera di accenti dinamici e registro target.
     - Trasformazioni pure con conservazione dell'identità: trasposizione modale, inversione di contorno, variazione ritmica euclidea (rotazione/impulsi di Bjorklund), alterazioni cromatiche controllate e riorchestrazione di ruolo.
  3. **Tempi musicali razionali:** confini di frase e sezione rigorosamente derivati dai confini di misura e beat del `MusicClock` a 48 kHz, senza accumulo di float e con zero drift.
  4. **Bounded Candidate Search:** la generazione di ciascuna frase/decisione opera su un insieme finito e piccolo di candidati (profilo iniziale 4–8 opzioni).
  5. **Vincoli Hard (Validazione di sistema):** filtraggio preventivo rigoroso applicato prima di qualsiasi scoring. Include unicamente: validità di parametri ed eventi, rispetto del budget di 2 voci per ruolo, aderenza ai confini della timeline e dell'orizzonte di commitment, capienza della coda SPSC e invarianti strutturali del piano. Vincoli di registro, assenza di cromatismi, consonanza o posizionamento su beat forti NON sono vincoli hard automatici. Il voice stealing è ammesso e può essere penalizzato musicalmente nello scoring se eccessivo.
  6. **Scoring deterministico e tie-break canonico:** funzione di costo versionata che valuta continuità di voice-leading, aderenza al target di tensione/risoluzione e novità controllata. Il tie-break tra candidati a pari merito è totale, stabile e rigorosamente indipendente dall'ordine di allocazione in memoria o dall'iterazione dei container, indicizzato da `RandomKey` (Stafford Mix 13) secondo la policy di determinismo M1.
  7. **Fallback compositivo garantito:** qualora nessun candidato superi i vincoli hard, il planner emette un pattern di fallback predefinito, bounded, valido e tracciabile (es. sustain neutro o silenzio sul ruolo), registrando l'intervento nel trace senza bloccare la generazione né lanciare eccezioni.
  8. **Orchestrazione delle triadi:** gli accordi a tre o più note devono essere realizzati distribuendo le voci tra Corpo (fondamentale e quinta/terza, max 2 voci) e Trama/Vettore, senza violare il budget dei 2 slot di Corpo e senza sacrificare la linea melodica.
- **Profilo sperimentale iniziale versionato (`fixture-m2-4-v1` / `proposed`):**
  - *Forma tripartita A–B–A′ con 32 battute per sezione (96 battute totali):*
    - **Sezione A (Esposizione e sviluppo):** Re Dorico, 120 BPM 4/4 (32 battute = 128 semiminime = 64,0 s = 3.072.000 campioni a 48 kHz). Pulso stabilisce la griglia euclidea fondamentale, Corpo presenta il fondamento armonico, Vettore espone il motivo principale `Motif_0`, Trama introduce contrappunto nella seconda metà.
    - **Sezione B (Contrasto e tensione):** Transizione `SetTempoMap` su confine di battuta esatto a Sol Eolio, 140 BPM 4/4 (32 battute = 128 semiminime = $\lfloor 128 \times 48.000 \times 60 / 140 \rfloor = 2.633.142$ campioni a 48 kHz; inizio al frame assoluto 3.072.000, fine al frame assoluto 5.705.142). Il pad di Corpo sostiene la transizione cross-epoch; Pulso e Trama adottano nuove configurazioni euclidee; Vettore esegue variazioni trasformazionali del motivo (inversione/trasposizione).
    - **Sezione A′ (Ritorno e cadenza):** Transizione `SetTempoMap` su confine di battuta esatto di ritorno a Re Dorico, 120 BPM 4/4 (32 battute = 128 semiminime = 64,0 s = 3.072.000 campioni a 48 kHz; inizio al frame assoluto 5.705.142, fine al frame assoluto 8.777.142). Ripresentazione variata di `Motif_0` e cadenza finale risolutiva.
  - *Durata nominale totale:* $64,0 + (2.633.142 / 48.000) + 64,0 \approx 182,857125\text{ secondi}$ (8.777.142 campioni a 48 kHz), conforme al requisito $\ge 180\text{ s}$ di Spec §32 e AC-AO-02.
  - *Precisione matematica e validazione MusicClock:* l'algoritmo di riconciliazione `MusicClock::setTempoMap()` richiede `deltaF == boundary.value` e accetta con precisione il delta di 2.633.142 frame per 128 semiminime a 140 BPM. Non tentare di forzare a mano 2.633.143 o 8.777.143 (che causerebbero `Rejected`), e non alterare i BPM o la metrica per un presunto errore matematico inesistente. I confini temporali devono essere sempre ricavati tramite `MusicClock::frameAtBeat()` e `frameAtBar()`.
  - *Nota metodologica:* questa fixture costituisce un profilo di prova versionato. Il planner procedurale non deve codificare tonalità, BPM o progressioni come costanti architetturali immutabili nel codice.
- **Stato:** `accepted for implementation (review remediation pending)` (con profilo `fixture-m2-4-v1` come `experimental initial profile`).

---

### D-M2-06: Architettura DSP dei 4 Ruoli, 4 Bus Musicali, Gain Staging e Protezione Master (Spec §16.8, §16.12, §20.2, §32)

- **Problema:** Definire l'infrastruttura di sintesi audio polifonica a 8 voci per i 4 ruoli musicali (Pulso, Corpo, Trama, Vettore), il routing dei bus, la sicurezza numerica e la prevenzione del clipping sul master, nel rispetto dei vincoli hard-realtime.
- **Invarianti architetturali normativi (`accepted for implementation`):**
  1. **Quattro bus preallocati dedicati:** un bus stereo float32 per ciascun ruolo (`Pulso`, `Corpo`, `Trama`, `Vettore`). Ciascun bus accumula in modo deterministico le uscite dei propri 2 voice slot e delle rispettive steal tails attive. Nessuna allocazione dinamica, nessun lock, nessun I/O nella callback realtime.
  2. **Sintesi nativa pura (zero campioni esterni per M2.4):** generazione audio basata su primitive DSP modulari condivise e sintesi analitica C++20, senza dipendenze da file audio esterni o decodifiche su disco, garantendo build pulita, determinismo e indipendenza del toolchain.
  3. **Sicurezza numerica e floating-point:** campioni e stati dei filtri/oscillatori garantiti finiti; rilevamento e bonifica attiva di `NaN`/`Inf`; tecniche di mitigazione dei denormali (FTZ/DAZ su piattaforme supportate o flush periodico a zero); smoothing dei parametri continui.
  4. **Protezione master bounded:** stadio finale di mix a costo computazionale $O(1)$ deterministico, senza lookahead obbligatorio in M2.4. Funzione matematica continua a comportamento bounded per ogni ingresso finito, con clamp di sicurezza a $[-1.0\text{f}, +1.0\text{f}]$. La protezione master previene il clipping digitale hard, ma la corretta gestione dinamica deve essere garantita a monte dal gain staging (il soft-clipper non deve compensare un arrangiamento saturato). Verifica distinta di clipping campionato, true peak offline, headroom e distorsione; non dichiarare assenza di clipping o zero NaN come conseguenza automatica del soft-clipper senza validazione dei campioni.
  5. **Telemetria per ruolo (`ChannelFrame`):** per ciascun ruolo, `AudioTelemetryFrame` pubblica metriche calcolate direttamente dai bus: RMS energy (misurata post-bus e pre-master per rappresentare fedelmente l'energia udibile), envelope peak e onset trigger autoritativi desunti da Music Intent. Nessuna FFT ridondante nella callback.
- **Profilo sperimentale iniziale versionato (`synth-palette-v1` / `proposed`):**
  - *Pulso:* percussione/kick sintetico (oscillatore sinusoidale con pitch sweep esponenziale rapido da 140 a 50 Hz in 25 ms) + generatore di transient click/noise band-limited da seed isolato (durata 15 ms). `timbreParam` modula la brillantezza del transiente e il decadimento.
  - *Corpo:* oscillatore PolyBLEP/wavetable a dente di sega ad alto contenuto armonico + filtro passa-basso risonante State Variable Filter (SVF/TPT) trapezoidale (candidato profilato con stabilità garantita per coefficienti bounded e range di risonanza controllati) + inviluppo ADSR completo (attacco 60 ms, decay 150 ms, sustain sostenibile, release 250 ms). `timbreParam` controlla cutoff e risonanza.
  - *Trama:* sintesi FM a 2 operatori (portante sinusoidale + modulante a rapporto intero/razionale) + micro-delay stereo tempo-sync bounded preallocato (max 0.5s, feedback $<0.5$). `timbreParam` modula l'indice di modulazione FM e il send del delay.
  - *Vettore:* lead melodico agile (PolyBLEP/wavetable) con inviluppo rapido (attacco 10 ms), pitch glide deterministico lineare bounded e LFO vibrato controllato. `timbreParam` modula apertura del filtro e vibrato depth.
  - *Gain staging iniziale:* Pulso −6 dB ($0,501$), Corpo −8 dB ($0,398$), Trama −10 dB ($0,316$), Vettore −6 dB ($0,501$). Guadagni calibrabili senza alterare contratti pubblici.
  - *Micro-ducking deterministico:* attenuazione di Corpo di 2–3 dB su attacco di Pulso con recupero rapido (50–80 ms), gestito come parametro opzionale del profilo di mix per liberare headroom sui transienti bassi.
- **Stato:** `accepted for implementation (review remediation pending)` (con preset e parametri come `experimental initial profile`).

---

### D-M2-07: Profiling Realtime DSP e Protocollo di Validazione per Handoff M2.5 (Spec §20.2, §31.5, §32)

- **Problema:** Stabilire le condizioni di benchmark e misurazione del rispetto della deadline realtime della callback audio, e definire il protocollo di verifica tecnica e multimodale per il passaggio di consegne da M2.4 a M2.5.
- **Invarianti architetturali normativi (`accepted for implementation`):**
  1. **Target prestazionale realtime:** la callback audio completa (8 voci simultanee attive, inviluppi ADSR, steal tails, micro-ducking, 4 bus, mix master, protezione clipping ed eventuale tap `AudioProbe`) deve operare entro il target $\le 2,0\text{ ms}$ per blocco da 512 frame device a 48 kHz ($<20\%$ del periodo fisico del quantum audio di $10,67\text{ ms}$).
  2. **Profiling lock-free preallocato:** misurazione dei tempi di esecuzione tramite clock monotono ad alta risoluzione (`QueryPerformanceCounter` su Windows o `std::chrono::steady_clock`) all'ingresso e all'uscita di `processBlock`. Raccolta statistica preallocata di p50, p95, p99, p99.9, massimo osservato e contatore di deadline miss (> 2.0 ms e > 10.67 ms). Nessun file I/O o log nella callback. Profiling confrontato con CACP attivo e disattivo.
  3. **Distinzione dei profili di build:** il target $\le 2,0\text{ ms}$ è vincolo per le build Release e per i benchmark controllati. Non applicare il target come asserzione bloccante (hard-fail) nelle build Debug, dove verifiche di consistenza, iteratori di controllo e strumentazioni diagnostiche possono legittimamente superare i 2 ms.
  4. **Protocollo Run continua $\ge 180\text{ s}$:** generazione di una run continua di almeno 180 secondi (fixture A-B-A'), esportando file audio WAV stereo a 48 kHz tramite `AudioProbe` e manifest JSON correlato con metadati completi (`build_id`, `transport_epoch`, `simulation_ticks`, metriche di fase).
  5. **Separazione rigorosa delle evidenze per il gate M2.5:**
     - *Verifiche tecniche vincolanti (Mandatory):* suite CTest 100% superata su Release, Debug e Headless; zero warning del compilatore (`/W4`); zero allocazioni heap o lock nella callback (verificate con hook CRT/aligned tracker); assenza di campioni non finiti (`NaN`/`Inf`); continuità campionaria esatta (zero gap o sample drop); assenza di underrun.
     - *Analisi quantitativa del segnale:* continuità PCM ai confini di epoch e durante voice stealing (assenza di discontinuità rettangolari); assenza di clipping campionato ($|x[n]| \le 1.0$); stima del true peak offline; verifica di silenzio numerico post-terminazione.
     - *Valutazione percettiva multimodale:* ispezione diretta degli artefatti tramite CACP (`view_file` su manifest e WAV) da parte dell'agente con giudizio esplicito su purezza timbrica, differenziazione dei ruoli e organicità delle transizioni. La valutazione percettiva dell'agente è supplementare (capability-dependent) e non sostituisce l'ascolto e il playtest umano obbligatorio per la chiusura finale di milestone.
- **Stato:** `accepted for implementation (review remediation pending)`.

---

## 3. Conseguenze e Compatibilità

- **Modulo `engine/core`:** Rimane puramente C++20 standard, indipendente da miniaudio e raylib. Ospiterà le definizioni di `InputAction`, `ActionState`, `NormalizedAction`, `euclideanRhythm`, e le strutture del motivo/planner simbolico.
- **Modulo `engine/audio`:** Estende `AudioCommandType` con `SetTempoMap`, ospita `AudioProbe` (tap SPSC lock-free), il Voice Allocator polifonico a 8 voci (`VoiceSlot[8]`) con steal tails statiche, i generatori DSP dei 4 ruoli, l'architettura a 4 bus con micro-ducking e la protezione master bounded.
- **Replay e determinismo:** I parametri congelati in D-M2-01..07 non introducono numeri casuali non tracciati né dipendenze esterne. Il tie-break totale del planner garantisce replay deterministico e seed isolation invariante.
- **Roadmap:** Con la formalizzazione di questo ADR (attualmente in stato `accepted for implementation (review remediation pending)`), tutte le decisioni d'ingresso per la sottofase **M2.4 — Musica procedurale v1** sono definite e allineate ai rilievi della revisione R1. Lo sviluppo seguirà rigorosamente il piano operativo articolato in **Inc-1..Inc-5** verso il gate d'integrazione **M2.5** una volta approvata la review documentale.
