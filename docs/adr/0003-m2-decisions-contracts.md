# ADR-0003 — Contratti M2: Music Intent, scheduler, dispatcher del tono e AudioProbe

- **Data:** 2026-10-12
- **Stato:** accepted for implementation
- **Milestone:** M2 — Music Intent e musica procedurale v1 (Sottofase M2.1)
- **Documenti correlati:** `CYMATICA_Specifica_Agentica_Sviluppo.md` §7.5, §16, §20.5–§20.8, §31.5–§31.6, §32; `DESIGN.md` §17.4.1; `docs/agent_control_observability.md` §4, §6, §9, §12, §13; `docs/progress.md`

---

## 1. Contesto

La Milestone 2 stabilisce la prima forma musicale simbolica condivisa di CYMATICA e introduce il controllo agentico e l'osservazione audiovisiva incrementale (CACP, Spec §31.5).
In ottemperanza a `AGENTS.md` §1.1 e alla sottofase **M2.1 — Decisioni e contratti** (Spec §32), prima di implementare i task dipendenti (M2.2 per tono e AudioProbe, M2.3 per transport e scheduler, M2.4 per musica procedurale v1) è necessario congelare formalmente cinque decisioni architetturali d'ingresso:

1. **D-M2-01:** Canale thread-safe `SetTempoMap` (Spec §20.7, §32);
2. **D-M2-02:** Rappresentazione simbolica `MusicIntentEvent` per i 4 ruoli musicali (Spec §16, §20.5, §32);
3. **D-M2-03:** Voice budget polifonico e scheduling ahead-of-time (Spec §20.7, §32);
4. **D-AO-01:** Nucleo comune del dispatcher input per il tono (`InputAction`/`ActionState`) e runner locale CACP (Spec §31.5, §32, Analisi CACP §4, §9, §12);
5. **D-AO-02:** Tap AudioProbe e profilo di verifica del tono `tone_probe_v0` (Spec §31.5, §32, Analisi CACP §6, §9, §12).

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
  - **Evoluzione polifonica M2.4 & M2.5 (Addendum architetturale propedeutico):**
    1. *Identità delle voci e mapping NoteOn/NoteOff:*
       In M2.2/M2.3 l'oscillatore di test era monofonico e tracciava l'epoch globale della nota attiva (`activeNoteEpoch_`). In M2.4 viene introdotto il pool statico di 8 voci (`VoiceSlot[8]`, preallocate senza allocazioni dinamiche):
       - 2 voci per `Pulso` (percussione/kick/snare sintetico);
       - 2 voci per `Corpo` (armonia/basso/pad);
       - 2 voci per `Trama` (arpeggi/texture);
       - 2 voci per `Vettore` (melodia lead/sweep).
       Ogni slot polifonico conserva almeno ruolo, `eventId` dell’occupante, stato attivo, fase e inviluppo, con ownership interna che impedisca a `NoteOff` tardivi di interrompere un nuovo occupante. Un `generationToken` di slot è un’opzione di implementazione interna, **non** un campo aggiunto automaticamente agli eventi pubblici. Definire unicità/riuso degli `eventId` prima di integrare le otto voci.
       - **Durata nominale e rilascio sicuro (decisione implementativa M2.4):** In M2.3 `AudioScheduler::scheduleIntent()` genera già `NoteOn` e, se `durationFrames > 0`, un `NoteOff` con lo stesso `(role, eventId)`. Conservare questa semantica finché la decisione d’ingresso M2.4 non approvi un’alternativa. Valutare una scadenza di rilascio *failsafe* interna allo slot, calcolata nella timeline logica a 48 kHz come `sampleFrame + durationFrames` con saturazione: se il `NoteOff` previsto non arriva, la voce non rimane appesa. Un `NoteOff` esplicito e l’eventuale failsafe devono essere idempotenti e verificati su backpressure, duplicati, drop e voice stealing. Definire esplicitamente il caso `durationFrames == 0` (nota aperta/non schedulata o evento da rifiutare) prima della relativa implementazione; nessun countdown in frame del device può sostituire la timeline logica.
       - **Voice stealing con riduzione dei click:** Resta la policy approvata di due voci per ruolo e rilascio breve nominale di 2 ms (96 frame logici a 48 kHz). Una coda DSP bounded (*steal tail*), un crossfade o un inviluppo di micro-rilascio sono **alternative da confrontare**, non garanzie automatiche di assenza di click. L’attacco della nuova nota deve rispettare il sample frame accettato; eventuali code transitorie consumano CPU/memoria e non diventano nuove note indipendenti. Verificare waveform e continuità ai confini, incluse fase e pendenza quando pertinenti, e registrare gli eventuali artefatti residui.
    2. *Ciclo di vita delle note sostenute (Pad di Corpo) e superamento del timeout arbitrario:*
       La salvaguardia temporanea monofonica introdotta in M2.3 (auto-release forzato a 48.000 campioni / 1s post-transizione epoch) viene superata in M2.4. Le note sostenute (es. pad e droni armonici di `Corpo`) mantengono la fase di Sustain legittimamente attraverso i confini di misura, frase ed epoch, terminando unicamente all'arrivo del loro legittimo `NoteOff` (o per voice-stealing all'interno delle 2 voci di quel ruolo con micro-fadeout di 2 ms). Il rilevamento di voci orfane sarà governato dallo stato dell'inviluppo ADSR e dalla durata nominale (`durationFrames`) specificata in `MusicIntentEvent`.
       - **Epoch, piano committed e trasporto:** L’attuale M2.3 incrementa `transportEpoch` su `SetTempoMap`, scarta eventi obsoleti salvo `NoteOff` riconciliabili con una voce attiva e lascia invariata la timeline `sampleFrame` logica. M2.4 deve preservare le voci già attive, rispettare gli eventi effettivamente *committed* e definire come invalidare/ripianificare le intenzioni future non committed con BPM/metrica cambiati: non è corretto né scartare indiscriminatamente tutte le note né conservare indistintamente tutti gli eventi di epoch precedente. Registrare prima dell’implementazione la policy di revisione del piano, i boundary e la riconciliazione delle code SPSC, con test intra-blocco e cross-epoch; non attribuire questa nuova semantica al runtime M2.3 già chiuso.
       - **Pause/Resume vs Stop/Reset (decisione differita a M2.4):** Una pausa recuperabile congela la timeline; definire esplicitamente se mantiene lo stato interno delle voci/inviluppi e come silenziare l’uscita e riprenderla senza click. `Stop`, cambio run e un eventuale `Reset` richiedono invece invalidazione del piano e pulizia dello stato. Non prescrivere un fade in un percorso che genera immediatamente zeri: un tail di 2 ms richiede un render di coda bounded, oppure va dichiarato un arresto tecnico immediato. Il comportamento attuale monofonico M2.3 non è una prova della futura policy polifonica.
    3. *Budget di calcolo e deadline realtime della callback DSP (M2.4 / M2.5):*
       A 48 kHz con quantum effettivo del device di 512 frame, il periodo nominale è $512/48000 \approx 10,67\text{ ms}$. Il test di flooding M2.3 con soglia $< 50\text{ ms}$ verifica la terminazione bounded, ma **non** certifica il rispetto della deadline audio.
       **Target accettato M2.4/M2.5:** la callback completa (8 voci, inviluppi, gestione eventi, mix ed eventuale overhead delle sonde) deve essere profilata rispetto a $\le 2,0\text{ ms}$ per 512 device frame a 48 kHz ($< 20\%$ del quantum). Registrare hardware, backend, sample rate e quantum effettivi, carico, percentili e massimo osservato, deadline miss, underrun e frame persi; eseguire benchmark Release e verifiche su hardware reale. Il rispetto del target non garantisce universalmente zero underrun: profili device differenti, incluso Android, richiedono misure dedicate. Ogni revisione del budget va registrata nell'ADR; restano invariati i gate audio obbligatori di M2.5 (Spec §32).
    4. *Vincoli trasversali per il sintetizzatore M2.4 (senza congelare algoritmi timbrici):*
       - **Realtime e stabilità numerica:** La callback mantiene lavoro bounded, nessuna allocazione/lock/I/O, campioni e stati DSP finiti, e strumenti per rilevare NaN/Inf e rallentamenti da subnormali. FTZ/DAZ dove disponibile, flush controllato degli stati e smoothing dei parametri sono tecniche candidate da selezionare in base a profilo numerico, portabilità, comportamento degli inviluppi e misure; tempi di smoothing e soglie di flush non sono automaticamente valori normativi.
       - **Telemetria di ruolo:** `AudioTelemetryFrame` conserva i quattro `ChannelFrame` (`pulse`, `body`, `texture`, `vector`); in M2.4 definire semantica misurabile di `energy`, `onset`, `pitchHz`, `density` e `confidence` coerente con i quattro bus e l’osservabilità CACP. Derivare preferibilmente feature bounded dallo stato simbolico/DSP anziché da FFT ridondanti nella callback, senza inventare un nuovo campo o dichiarare accuratezze non testate.
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

## 3. Conseguenze e Compatibilità

- **Modulo `engine/core`:** Rimane puramente C++20 standard, indipendente da miniaudio e raylib. Ospiterà le definizioni di `InputAction`, `ActionState`, `NormalizedAction` e `euclideanRhythm`.
- **Modulo `engine/audio`:** Estende `AudioCommandType` con `SetTempoMap`, ospita `AudioProbe` (tap SPSC lock-free) e le strutture di sintesi polifonica a 8 voci per i 4 ruoli musicali.
- **Replay e determinismo:** I parametri congelati in D-M2-01..03 e D-AO-01..02 non introducono numeri casuali non tracciati né dipendenze esterne.
- **Roadmap:** Con il congelamento di questo ADR, la sottofase **M2.1** è completata, sbloccando l'implementazione della sottofase **M2.2 — Controllo e osservazione del tono (AC-AO-01A)**.
