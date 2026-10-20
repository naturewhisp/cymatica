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
- Per **M2.4 Inc-4** (2026-10-20): D-M2-08..D-M2-11 in [ADR-0005](0005-m2-4-inc4-entry-decisions.md) precisano D-M2-05 (espansione della fixture, orizzonte di pianificazione) e D-M2-07 (metodologia di profiling, scenari CACP) senza modificarne gli invarianti.

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
     - `durationFrames == 0` costituisce un gate aperto il cui rilascio musicale ordinario è controllato esclusivamente da un evento matching esplicito `NoteOff` su `(role, eventId)`. Il planner M2.4 ne limita l'uso iniziale ai pad armonici sostenuti di `Corpo`, senza introdurre un divieto strutturale irreversibile nel contratto pubblico. Rimosso il timeout monofonico temporaneo di 1s (48.000 campioni), consentendo ai pad di sostenere legittimamente lo stadio di `Sustain`.
     - *Distinzione dei domini di lease e recoverability fail-closed:*
       - **Dominio compositivo (Planner Lease):** il planner gestisce il ciclo di vita musicale della nota aperta collegandola all'orizzonte di frase e sezione, ed è contrattualmente responsabile dell'invio del matching `NoteOff` alla cadenza o al termine del sustain pianificato.
       - **Dominio audio (AudioEngine Fail-Closed Bound):** a tutela dell'hard-realtime contro la perdita effettiva di un `NoteOff`, anomalie di canale o blocchi del thread di coordinamento, `AudioEngine` impone un massimale deterministico di sicurezza `audioMaxLeaseDurationFrames` indipendente dalla struttura interna del planner (non inferibile da `MusicClock`, che gestisce unicamente metrica e tempo, ma configurato come ceiling conservativo sul tempo nominale, es. pari a un bound di sicurezza di 36 battute = 3.456.000 campioni a 120 BPM a 48 kHz). Alla scadenza `audioMaxLeaseDeadline = startFrame + audioMaxLeaseDurationFrames` in assenza di `NoteOff`, interviene un rilascio di sicurezza forzato (Release con micro-fadeout di 2 ms, incrementando `orphanNoteRecoveredCount` nella telemetria).
       - **Continuità cross-epoch non distruttiva:** in coerenza con il punto 6, le transizioni di tempo/metrica non distruttive (`SetTempoMap`) **preservano le note attive in tutti i ruoli**, inclusi i pad di `Corpo` in `Sustain`. Non viene applicato alcuno sweep distruttivo su `SetTempoMap`. Il pad sostiene la transizione cross-epoch senza troncamenti artificiali e senza richiedere estensioni ABI o nuovi tipi di eventi in `MusicIntentEvent`/`ScheduledAudioEvent`. Il `NoteOff` matching emesso dal planner (anche con epoch precedente) viene regolarmente onorato al suo frame nominale garantito.
     - *Comportamento di trasporto sul lease:*
       - Durante `Pause`: poiché la timeline logica è congelata, il decorso del lease di sicurezza è anch'esso congelato al frame logico $t_{\text{pause}}$ (calcolato sui frame logici effettivamente renderizzati), evitando scadenze anomale durante la pausa.
       - Su `Stop`, `Reset` o Nuova Run: tutte le note aperte vengono forzatamente resettate a `Idle`.
       - In caso di nuove note sul ruolo: interviene la policy deterministica di voice stealing (punto 5).
     - *Tassonomia degli eventi e delle anomalie di scheduling:*
       - *Backpressure SPSC ordinaria:* lo scheduler trattiene l'evento staged nel buffer locale e ne ritenta il dispacciamento al blocco successivo senza scarto né drop;
       - *Evento tardivo (Late event):* evento pervenuto con `sampleFrame` inferiore al cursore corrente; viene scartato o processato al frame corrente con incremento della telemetria;
       - *Drop effettivo (Actual drop) o mancata emissione di NoteOff:* intercettato determiniscamente dal bound di sicurezza fail-closed `audioMaxLeaseDeadline`.
     - *Verifiche previste in Inc-1:* pad cross-epoch sostenuto attraverso `SetTempoMap` senza drop, rilascio di sicurezza allo scadere di `audioMaxLeaseDeadline` in assenza di NoteOff o con planner fermo, congelamento del lease durante `Pause`, reset immediato a `Idle` su Stop/Reset.
  5. **Voice Stealing Bounded con Steal Tails statiche:**
     - Al frame campionario esatto del nuovo `NoteOn`, la voce subentrante attacca immediatamente il suo inviluppo, preservando l'onset ritmico sample-accurate senza alcun ritardo.
     - La voce uscente viene trasferita su una *steal tail* secondaria, statica e preallocata all'interno dello slot, che applica una rampa lineare discendente a zero di esattamente 2 ms (96 campioni a 48 kHz). Trattasi di un de-click rapido sull'energia d'uscita (snapshot decay), non di una continuazione della simulazione di filtri/oscillatori multipli concorrenti nello stesso slot.
     - Limite rigido: esiste al massimo 1 steal tail attiva per slot (massimo 8 steal tails simultanee nel sistema, preallocate senza heap allocation).
     - *Saturazione (steal su steal entro 96 campioni):* se interviene un ulteriore furto sullo stesso slot prima dell'estinzione della rampa ($n < 96$ campioni), la steal tail preallocata ricattura l'ampiezza istantanea per canale del segnale sommato in uscita:
       $$S_{\text{ch}} = y_{\text{tail, ch}}[0] + y_{\text{voice, ch}}[0]$$
       (valutata per canale senza clamp prematuro pre-master, delegando la protezione della dinamica al bus e al limiter master) e riavvia una nuova rampa di de-click lineare discendente a zero di 2 ms ($N_{\text{tail}} = 96$ frame a 48 kHz):
       $$y_{\text{tail, ch}}[k] = S_{\text{ch}} \cdot \left(1.0\text{f} - \frac{k}{N_{\text{tail}}}\right)$$
       incrementando `voiceDropCount` nella telemetria. Questo meccanismo previene accumulo di code, non richiede una seconda steal tail né allocazioni dinamiche e assicura una transizione progressiva dell'energia d'uscita.
     - *Nessuna invarianza C0 universale a priori:* Non si postula aprioristicamente un'invarianza formale $C^0$ o l'assenza assoluta di click per qualsiasi condizione di fase estrema: l'efficacia del de-click deve essere misurata e validata empiricamente in Inc-1 tramite analisi dei campioni PCM, picchi pre/post-master, e ascolto multimodale su furti ravvicinati ripetuti.
     - *Ordinamento totale deterministico di selezione (Voice Allocator Lessicografico):* se entrambi i 2 slot del ruolo sono occupati, la selezione dello slot da rubare segue un ordinamento totale, deterministico e privo di ambiguità:
       - **Regola 0 (Disponibilità immediata):** se almeno uno slot del ruolo è `Idle`, viene selezionato lo slot `Idle` (a parità, `slotIndex` 0 prima di 1). Nessun furto necessario.
       - **Regola 1 (Voci in Release):** se $\ge 1$ slot si trova nello stadio `Release`, il furto seleziona obbligatoriamente tra gli slot in `Release`. Se entrambi sono in `Release`, la selezione avviene secondo l'ordine lessicografico stretto:
         1. minore ampiezza istantanea dell'inviluppo $|A_0| < |A_1|$;
         2. `startFrame` più remoto (voce avviata prima);
         3. minore `slotIndex` (0 prima di 1).
       - **Regola 2 (Voci attive non in Release: Attack/Decay/Sustain):** se entrambi gli slot sono attivi non in Release, la selezione avviene secondo:
         1. `startFrame` più remoto (voce attiva da più tempo);
         2. minore `slotIndex` (0 prima di 1).
       - **Bonifica floating-point e tracciamento anomalie:** qualsiasi valore non finito (`NaN`/`Inf`) rilevato nel comparatore viene bonificato a $0.0\text{f}$ per garantire la stabilità dell'ordinamento E contestualmente registrato incrementando il contatore atomico di telemetria `dspAnomalyCount`.
  6. **Confine di commitment e riconciliazione SPSC cross-epoch:**
     - *Mutable Planning Horizon:* frasi e sezioni future mantenute nelle strutture del Coordinator/Planner, non ancora trasferite alla SPSC; liberamente modificabili, invalidabili o ripianificabili in caso di cambio metrica/tempo (`SetTempoMap`) o interventi di gameplay.
     - *Committed Horizon:* eventi già trasferiti nella `SpscQueue<ScheduledAudioEvent, 256>` nella finestra di lookahead nominale (150 ms = 7.200 campioni = 18 tick a 120 Hz). Una volta inseriti nella SPSC, gli eventi sono **garantiti** al loro `sampleFrame` nominale.
     - *Semantica di SetTempoMap (non distruttivo) e migrazione M2.4:* su transizione di tempo/metrica con incremento di `transportEpoch`, le note già attive proseguono inalterate; i `NoteOff` cross-epoch matching per `(role, eventId)` con epoch precedente vengono regolarmente onorati al loro frame nominale; gli eventi `NoteOn` futuri già committed nella SPSC mantengono il loro `sampleFrame` garantito e non vengono scartati. Il planner ripianifica unicamente le intenzioni nel Mutable Planning Horizon (non committed) adattandole alla nuova mappa di tempo.
     - *Stato migrazione rispetto a M2.3 (COMPLETATA in Inc-1):* la logica di `AudioEngine` (`minValidEpoch_`) distingue ora rigorosamente i cambi non distruttivi (`SetTempoMap`, `Pause/Resume`) dalle vere revoche distruttive (`Stop`, `Reset`, nuova run), preservando le voci attive e gli eventi committed attraverso le transizioni non distruttive.
  7. **Matrice Stati di Trasporto (`Pause`/`Resume` vs `Stop`/`Reset`):**
     - *Separazione rigorosa tra dominio logico e dominio fisico:*
       - **Dominio del Trasporto Logico:**
         - All'accettazione del comando `Pause`, il cursore logico, la pianificazione, il consumo dalla SPSC, le fasi degli oscillatori, gli inviluppi ADSR, gli LFO e le scadenze nominali si congelano **immediatamente** ($O(1)$) al frame campionario nominale del comando ($t_{\text{pause}}$). Non si ritarda il congelamento logico di frame né si consumano eventi futuri.
         - All'accettazione del comando `Stop`, il Coordinator arresta il proprio loop di generazione compositiva e invalida i piani futuri nel proprio stato privato (`Mutable Planning Horizon`); `AudioEngine` al boundary del blocco audio invalida e drena bounded gli eventi staged nella SPSC, resetta forzatamente tutti gli 8 slot a `Idle` (fasi azzerate, inviluppi spenti, `generationToken` incrementato), ed emette `AudioCommandAck::Applied`.
         - `AudioCommandAck::Applied` viene emesso da `AudioEngine` in corrispondenza del confine logico effettivo (il blocco in cui ha effetto il congelamento o lo stop della simulazione audio).
       - **Dominio dell'Uscita Fisica (Physical Output Tail sul Device):**
         - Per prevenire click acustici, a `Pause` viene applicata una **output-only de-click tail** lineare di 2 ms esclusivamente nel buffer d'uscita del device a partire dall'indice di campione fisico di attivazione $n_{\text{device, pause}}$. La durata fisica è calcolata esattamente come $N_{\text{tail}} = \lfloor 0.002 \times f_{s,\text{device}} \rfloor$ campioni device (96 frame a 48 kHz, 88 frame a 44.1 kHz).
         - Lo stato logico `Paused` coincide per $\le N_{\text{tail}}$ campioni fisici con un'uscita in attenuazione di de-click; il silenzio numerico assoluto ($0.0\text{f}$) nel buffer del device è garantito tassativamente a partire da $n_{\text{device, pause}} + N_{\text{tail}}$.
         - In caso di `Resume` pervenuto durante la coda fisica ($< N_{\text{tail}}$ campioni da `Pause`), la rampa fisica inverte la direzione verso 1.0f a partire dall'ampiezza fisica istantanea corrente, prevenendo discontinuità di livello. Al `Resume` completato, la timeline logica riprende l'avanzamento esattamente dal medesimo frame congelato $t_{\text{pause}}$.
         - In caso di `Stop` ordinario, l'ampiezza istantanea per canale di ciascuno slot (scalata da `transportRampLevel_`) viene catturata nella rispettiva `stealTail` con un'unica rampa lineare discendente di $N_{\text{tail}}$ campioni e `transportRampLevel_` viene ripristinato a `1.0f`, evitando decadimenti quadratici, riemissioni fantasma durante `Paused` o silenziamenti permanenti su successivo `Start`.
       - **ACK e ordinamento sequenze ravvicinate intra-blocco:** Comandi multipli nello stesso blocco vengono risolti determiniscamente applicando lo stato risultante; comandi `Stop` o `Emergency Mute` hanno priorità assoluta e annullano transizioni di pausa in corso.
     - `Emergency Mute`: azzeramento immediato del buffer audio senza fade, riservato esclusivamente a condizioni anomale o errori fatali in cui un gradino transitorio è accettato a fronte di un muting istantaneo.
- **Stato:** `accepted for implementation — implementato e verificato in Inc-1 (2026-10-19)`.

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
- **Profilo sperimentale iniziale versionato (`fixture-m2-4-v1` / `experimental initial profile`):**
  - *Forma tripartita A–B–A′ con 32 battute per sezione (96 battute totali):*
    - **Sezione A (Esposizione e sviluppo):** Re Dorico, 120 BPM 4/4 (32 battute = 128 semiminime = 64,0 s = 3.072.000 campioni a 48 kHz). Pulso stabilisce la griglia euclidea fondamentale, Corpo presenta il fondamento armonico, Vettore espone il motivo principale `Motif_0`, Trama introduce contrappunto nella seconda metà.
    - **Sezione B (Contrasto e tensione):** Transizione `SetTempoMap` su confine di battuta esatto a Sol Eolio, 140 BPM 4/4 (32 battute = 128 semiminime = $\lfloor 128 \times 48.000 \times 60 / 140 \rfloor = 2.633.142$ campioni a 48 kHz; inizio al frame assoluto 3.072.000, fine al frame assoluto 5.705.142). Il pad di Corpo sostiene la transizione cross-epoch; Pulso e Trama adottano nuove configurazioni euclidee; Vettore esegue variazioni trasformazionali del motivo (inversione/trasposizione).
    - **Sezione A′ (Ritorno e cadenza):** Transizione `SetTempoMap` su confine di battuta esatto di ritorno a Re Dorico, 120 BPM 4/4 (32 battute = 128 semiminime = 64,0 s = 3.072.000 campioni a 48 kHz; inizio al frame assoluto 5.705.142, fine al frame assoluto 8.777.142). Ripresentazione variata di `Motif_0` e cadenza finale risolutiva.
  - *Durata nominale totale:* $8.777.142 / 48.000 = 182,857125\text{ s}$ (durata musicale esatta $64 + 54,857143 + 64 = 182,857143\text{ s}$, troncata al frame), conforme al requisito $\ge 180\text{ s}$ di Spec §32 e AC-AO-02.
  - *Precisione matematica e validazione MusicClock:* l'algoritmo di riconciliazione `MusicClock::setTempoMap()` richiede `deltaF == boundary.value` e accetta con precisione il delta di 2.633.142 frame per 128 semiminime a 140 BPM. Non tentare di forzare a mano 2.633.143 o 8.777.143 (che causerebbero `Rejected`), e non alterare i BPM o la metrica per un presunto errore matematico inesistente. I confini temporali devono essere sempre ricavati tramite `MusicClock::frameAtBeat()` e `frameAtBar()`.
  - *Nota metodologica:* questa fixture costituisce un profilo di prova versionato per Inc-3/Inc-4. Il planner procedurale non deve codificare tonalità, BPM o progressioni come costanti architetturali immutabili nel codice.
- **Precisazione Inc-4 (2026-10-20):** espansione della fixture in 24 frasi, tabella armonica, maschere di ruolo, tensioni, pad cross-sezione e orizzonte di pianificazione con gestione degli epoch sono fissati in [ADR-0005](0005-m2-4-inc4-entry-decisions.md) D-M2-08/D-M2-09.
- **Stato:** `accepted for implementation` (con profilo `fixture-m2-4-v1` come `experimental initial profile` per Inc-3/Inc-4).

---

### D-M2-06: Architettura DSP dei 4 Ruoli, 4 Bus Musicali, Gain Staging e Protezione Master (Spec §16.8, §16.12, §20.2, §32)

- **Problema:** Definire l'infrastruttura di sintesi audio polifonica a 8 voci per i 4 ruoli musicali (Pulso, Corpo, Trama, Vettore), il routing dei bus, la sicurezza numerica e la prevenzione del clipping sul master, nel rispetto dei vincoli hard-realtime.
- **Invarianti architetturali normativi (`accepted for implementation — Gate pre-Inc-2 chiuso 2026-10-19`):**
  1. **Quattro bus preallocati dedicati:** un bus stereo float32 per ciascun ruolo (`Pulso`, `Corpo`, `Trama`, `Vettore`). Ciascun bus accumula in modo deterministico le uscite dei propri 2 voice slot ($k_{\text{scale}} = 0.25\text{f}$) e delle rispettive steal tails attive. Nessuna allocazione dinamica, nessun lock, nessun I/O nella callback realtime.
  2. **Renderer nativo C++20 puro, senza dipendenze esterne né campioni esterni in M2.4 (ADR-0004 AUD-D-01, Gate pre-Inc-2 chiuso):** per Inc-2 è qualificata l'implementazione **C++20 nativa pura autocontenuta** in `engine/audio/` con **zero dipendenze di terze parti** (`docs/dependencies.md` invariato). Nessun host plugin, libreria monolitica o file audio esterno richiesto dal renderer v1.
  3. **Sicurezza numerica e floating-point:** campioni e stati dei filtri/oscillatori garantiti finiti; rilevamento e bonifica attiva di `NaN`/`Inf` con incremento di `dspAnomalyCount`; flush portatile dei subnormali (`|v| < 1e-20f ? 0.0f : v`); smoothing esponenziale one-pole ($\tau = 5\text{ ms}$) dei parametri continui.
  4. **Protezione master terminale comune Zero-Latency (ADR-0004 AUD-D-02, Gate pre-Inc-2 chiuso):** tutte le sorgenti audio controllate da CYMATICA convergono, prima della consegna digitale al backend (senza SRC post-limiter poiché il renderer opera direttamente a `impl->sampleRate`), nella catena terminale:
     `8 VoiceSlots + steal tails -> rampa di de-click di trasporto -> 4 Role Buses + Ducking -> somma Music Bus (+ tono diagnostico) -> Master Finite Guard -> MasterLimiter unico zero-latency (L=0) con clamp finale [-0.89125f, +0.89125f] -> pOutput & AudioProbe Tap` (ordine emendato in ADR-0004 §4.3; nessun soft-clipper implementato in M2.4).
     Il `MasterLimiter` opera a latenza zero ($L=0$, preservando gli onset sample-accurate e il confine di silenzio $n_{\text{pause}} + N_{\text{tail}}$) con rilevatore causale euristico (picco campione + 3 interpolatori frazionari a 3 coefficienti + curvatura $\sqrt{x^2 + 0.65(\Delta x)^2 + 0.35(\Delta^2 x)^2}$), soglia interna $C_{\text{lim}} = -1{,}80\text{ dBFS}$ ($0{,}8128\text{f}$; valori e topologia autoritativi in ADR-0004 §4.1/§4.3), attacco $0\text{ ms}$, hold $2\text{ ms}$, rilascio $50\text{ ms}$ e hard safety ceiling $C_{\text{max}} = -1.0\text{ dBFS}$ ($0.89125\text{f}$). La verifica indipendente offline del true peak utilizza il filtro polifase standard a 48 tap di ITU-R BS.1770-4/-5 Annex 2 con zero padding ai bordi (`analysis::estimateTruePeakDbtp`); il risultato è una stima, non una garanzia.
  5. **Telemetria per ruolo (`ChannelFrame`):** per ciascun ruolo, `AudioTelemetryFrame` pubblica metriche calcolate direttamente dai bus: RMS energy (misurata post-bus e pre-master per rappresentare fedelmente l'energia udibile), envelope peak, pitch prevalente e onset trigger autoritativi desunti da Music Intent. Nessuna FFT ridondante nella callback.
- **Profilo iniziale qualificato per Inc-2 (`synth-palette-v1` / `accepted for implementation in Inc-2`):**
  - *Pulso:* percussione/kick sintetico (oscillatore sinusoidale con pitch sweep esponenziale rapido da $f_{\text{start}} = \min(3 f_0, 180\text{ Hz})$ a $f_0$ con $\tau_p = 8\text{ ms}$) + generatore di transient click/noise band-limited deterministico da `core::staffordMix13(eventId ^ (k * GOLDEN_RATIO) ^ slotIndex)` (durata 15 ms, filtrato con TPT SVF). `timbreParam` modula la brillantezza del transiente e il decadimento.
  - *Corpo:* oscillatore 2-point PolyBLEP a dente di sega + filtro passa-basso risonante Zavalishin/Simper TPT SVF trapezoidale ($f_c \in [20\text{ Hz}, 0.45 f_s]$, smorzamento $R = 1/(2Q) \in [0.05, 1.0]$ incondizionatamente BIBO stabile) + inviluppo ADSR completo (attacco 60 ms, decay 150 ms, sustain $0.75$, release $\ge 2\text{ ms}$). `timbreParam` controlla cutoff e risonanza.
  - *Trama:* sintesi FM a 2 operatori con bound anti-aliasing di Carson $I_{\max}(f_c, f_m) = \max(0, \frac{0.42 f_s - f_c}{f_m} - 1) \le 4.0$ + micro-delay stereo bounded preallocato (`std::array<float, 24000>` per canale, feedback $|g_{\text{fb}}| \le 0.45$). `timbreParam` modula l'indice FM e il send del delay.
  - *Vettore:* lead melodico agile (2-point PolyBLEP saw) con inviluppo rapido (attacco 10 ms), pitch glide esponenziale one-pole ($\tau = 15\text{ ms}$), LFO vibrato sinusoidale ($5.5\text{ Hz}$) e filtro TPT SVF. `timbreParam` modula apertura del filtro e vibrato depth.
  - *Gain staging iniziale:* Pulso −6 dB ($0,5012$), Corpo −8 dB ($0,3981$), Trama −10 dB ($0,3162$), Vettore −6 dB ($0,5012$). Guadagni calibrabili senza alterare contratti pubblici.
  - *Micro-ducking deterministico:* attenuazione di Corpo fino a −3 dB ($g_{\text{duck}} \in [0.7079, 1.0]$) su attacco di Pulso ($\tau_a = 2\text{ ms}, \tau_r = 120\text{ ms}$), gestito nel bus per liberare headroom sui transienti bassi.
- **Stato:** `accepted for implementation — Gate pre-Inc-2 chiuso (2026-10-19)`.

---

### D-M2-07: Profiling Realtime DSP e Protocollo di Validazione per Handoff M2.5 (Spec §20.2, §31.5, §32)

- **Problema:** Stabilire le condizioni di benchmark e misurazione del rispetto della deadline realtime della callback audio, e definire il protocollo di verifica tecnica e multimodale per il passaggio di consegne da M2.4 a M2.5.
- **Invarianti architetturali normativi (`accepted for implementation`):**
  1. **Target prestazionale realtime:** la callback audio completa (8 voci simultanee attive, ADSR, steal tails, micro-ducking, 4 bus, mix, **protezione terminale `MasterLimiter` zero-latency ratificata in ADR-0004** ed eventuale tap `AudioProbe`) deve operare entro il target $\le 2,0\text{ ms}$ per blocco da 512 frame device a 48 kHz ($<20\%$ del periodo fisico del quantum audio di $10,67\text{ ms}$). Qualsiasi elaborazione nel percorso realtime partecipa allo stesso budget, senza franchigia.
  2. **Profiling lock-free preallocato:** misurazione dei tempi di esecuzione tramite clock monotono ad alta risoluzione (`QueryPerformanceCounter` su Windows o `std::chrono::steady_clock`) all'ingresso e all'uscita di `processBlock`. Raccolta statistica preallocata di p50, p95, p99, p99.9, massimo osservato e contatore di deadline miss (> 2.0 ms e > 10.67 ms). Nessun file I/O o log nella callback. Profiling confrontato con CACP attivo e disattivo.
  3. **Distinzione dei profili di build:** il target $\le 2,0\text{ ms}$ è vincolo per le build Release e per i benchmark controllati. Non applicare il target come asserzione bloccante (hard-fail) nelle build Debug, dove verifiche di consistenza, iteratori di controllo e strumentazioni diagnostiche possono legittimamente superare i 2 ms.
  4. **Protocollo Run continua $\ge 180\text{ s}$:** generazione di una run continua di almeno 180 secondi (fixture A-B-A'), esportando file audio WAV stereo a 48 kHz tramite `AudioProbe` e manifest JSON correlato con metadati completi (`build_id`, `transport_epoch`, `simulation_ticks`, metriche di fase).
  5. **Separazione rigorosa delle evidenze per il gate M2.5:**
     - *Verifiche tecniche vincolanti (Mandatory):* suite CTest 100% superata su Release, Debug e Headless; zero warning del compilatore (`/W4`); zero allocazioni heap o lock nella callback (verificate con hook CRT/aligned tracker); assenza di campioni non finiti (`NaN`/`Inf`); continuità campionaria esatta (zero gap o sample drop); assenza di underrun.
     - *Analisi quantitativa del segnale:* continuità PCM ai confini di epoch e durante voice stealing (assenza di discontinuità rettangolari); assenza di clipping campionato ($|x[n]| \le 0.89125\text{f}$); **stima indipendente** del true peak offline (`analysis::estimateTruePeakDbtp`, ITU-R BS.1770-4/-5 Annex 2, FIR 48 tap); prova della protezione terminale attiva anche con CACP/worker OFF; verifica di silenzio numerico post-terminazione. `-1 dBTP`, `-23 LUFS` e audit H.872 rolling 30 min restano sperimentali/differiti, **non** nuovi gate mandatory di M2.5.
     - *Valutazione percettiva multimodale:* ispezione diretta degli artefatti tramite CACP (`view_file` su manifest e WAV) da parte dell'agente con giudizio esplicito su purezza timbrica, differenziazione dei ruoli e organicità delle transizioni. La valutazione percettiva dell'agente è supplementare (capability-dependent) e non sostituisce l'ascolto e il playtest umano obbligatorio per la chiusura finale di milestone.
- **Precisazione Inc-4 (2026-10-20):** punto di misura, istogramma, carichi, warm-up, ripetizioni, significato di CACP ON/OFF, statistica di PASS (p99.9 ≤ 2,0 ms in Release) e ripartizione offline/device (corsa su device a M2.5) sono fissati in [ADR-0005](0005-m2-4-inc4-entry-decisions.md) D-M2-10/D-M2-11.
- **Stato:** `accepted for implementation`.

---

## 3. Conseguenze e Compatibilità

- **Modulo `engine/core`:** Rimane puramente C++20 standard, indipendente da miniaudio e raylib. Ospita le definizioni di `InputAction`, `ActionState`, `NormalizedAction`, `euclideanRhythm`, `staffordMix13` e le strutture del motivo/planner simbolico.
- **Modulo `engine/audio`:** Estende `AudioCommandType` con `SetTempoMap`, ospita `AudioProbe` (tap SPSC lock-free), il Voice Allocator polifonico a 8 voci (`VoiceSlot[8]`) con steal tails statiche (implementato e verificato in **Inc-1**), e i generatori DSP C++20 nativi dei 4 ruoli con architettura a 4 bus, micro-ducking e `MasterLimiter` zero-latency (implementati e verificati in **Inc-2**).
- **Replay e determinismo:** I parametri congelati in D-M2-01..07 non introducono numeri casuali non tracciati né dipendenze esterne. Il tie-break totale del planner e l'hash stateless `staffordMix13` per il noise percussivo garantiscono replay deterministico e seed isolation invariante.
- **Roadmap:** Inc-1..Inc-3 sono implementati; le decisioni d'ingresso di Inc-4 sono in [ADR-0005](0005-m2-4-inc4-entry-decisions.md). Stato, conteggi dei test ed evidenze sono registrati solo in [progress.md](../progress.md). Le decisioni di questo ADR restano `accepted for implementation`; nessun profilo sperimentale è `validated`.
