# CYMATICA M2.4 — Piano di integrazione compositiva e DSP v1

- **Stato:** **documento di ricerca e architettura compositiva allineato ai contratti d'ingresso M2.4 ratificati in [ADR-0003](../adr/0003-m2-decisions-contracts.md)** (2026-10-14); la sequenza operativa vincolante di sviluppo è esclusivamente articolata negli incrementi **Inc-1..Inc-5** definiti in `docs/progress.md`. Non sostituisce la roadmap normativa.
- **Baseline:** commit `460bc286378b473218bc07f751e3615964626a11` (2026-10-13), aggiornato con formalizzazione D-M2-04..07 (2026-10-14).
- **Fonti:** [ricerca revisionata](M2_4_Deep_Research_Reviewed.md), [review metodologica](M2_4_Research_Review.md), [Spec §16/§32](../../CYMATICA_Specifica_Agentica_Sviluppo.md), [ADR-0003](../adr/0003-m2-decisions-contracts.md), [DESIGN §8/§16](../../DESIGN.md).
- **Owner del piano:** Coordinatore sviluppo M2.4; scelta artistica finale del titolare. Non attribuire un'autorità normativa autonoma al documento.


> **Ratifica renderer/safety, 2026-10-15:** la [ricerca audio revisionata](M2_4_Audio_Renderer_Research_Review.md) e lo [studio safe listening](M2_4_Audio_Safety_Design.md) hanno condotto al [ratificato ADR-0004](../adr/0004-m2-4-audio-renderer-safety-proposal.md): **renderer nativo C++20**, riuso DSP solo dopo audit, **no host plugin obbligatorio**, protezione digitale terminale comune **sempre attiva/fail-closed** e tre domini di monitoring (callback, CACP worker, offline). Le parti pertinenti di D-M2-06/D-M2-07 sono emendate. **Inc-1 è nativo e indipendente dalle nuove scelte DSP/master**; richiede autorizzazione distinta. Nessun modulo terzo, limiter o metering è ancora implementato; Inc-1..Inc-5 rimangono la sequenza operativa.
>
> **Gate locale pre-Inc-2:** prima del codice del master, scegliere/approvare algoritmo e topologia della protezione (SRC, gain, latency e ceiling di prova), verificandoli su worst-case e nel budget callback totale D-M2-07. Qualificare singolarmente ogni modulo DSP esterno prima dell'uso. In Inc-4/Inc-5 raccogliere misure sample peak, stime true peak **indipendenti**, LUFS M/S/I e validità capture: **parametri sperimentali**, non nuovi gate H.872. La fixture ~183 s non dimostra il rolling 30 min. CACP non modifica `MusicClock` e il limiting resta attivo con worker OFF.

## 1. Principio e limiti di scope

M2.4 deve consegnare **una prima composizione dotata di forma, ricorrenza e personalità timbrica**, non un software di produzione musicale generico. Restano obbligatori i deliverable M2 già definiti: quattro ruoli, Bjorklund, scala/modo, memoria minima, otto voci (2/ruolo), synth/mix, due sezioni e sincronismo deterministico. Il gate finale di M2 è M2.5, non questo piano.

Non introdurre in M2.4: modelli neurali, training di dataset, factor-oracle live, solver illimitato, synth granulare aperto, voice pool dinamico, MIDI come sostituto di `MusicIntentEvent`, nuovi formati pubblici di level o modifica non autorizzata di ADR-0003. Tutte le nuove strutture compositive sono **interne al planner**, salvo decisione successiva.

## 2. Separazione di responsabilità

```text
Core/deterministic planner (mai nella callback)
  RunForm / SectionPlan / PhrasePlan / MotifMemory
  HarmonicFunction / BoundedVoicing / RhythmPolicy
  4-role realization + MusicalDiagnosticTrace
                   |
                   v
   MusicIntentEvent (ABI/IR attuale D-M2-02)
                   |
                   v
  AudioScheduler / SPSC (D-M2-03)
                   |
                   v
  AudioEngine: VoiceSlot[8], DSP, bus, mix
                   |
                   v
  miniaudio / AudioProbe / telemetry
```

`MotifMemory`, `PhrasePlan`, `HarmonicFunction` sono oggetti concettuali **non ancora contratti/API**. Non imporre la memorizzazione permanente dei motivi in `MusicIntentEvent`. Per la futura proiezione geometrica servirà invece un piano semantico condiviso, da formalizzare all'ingresso di M3 senza duplicare la timeline.

## 3. Sequenza operativa autoritativa (Inc-1..Inc-5) e mappatura dei workstream di ricerca

La **sequenza operativa autoritativa e vincolante** di implementazione per M2.4 è definita unicamente dagli incrementi indipendenti **Inc-1..Inc-5** in `docs/progress.md`. A ciascun incremento corrisponde un profilo diagnostico CACP parallelo e proporzionato alle funzionalità introdotte, per consentire la verifica in prima persona con manifest quantitativi RFC 8259 e ispezione multimodale/ascolto degli artefatti:
- **Inc-1 (Voice Allocator & Lifecycle):** implementa i contratti normativi di M2.4-C (allocazione a 8 voci, pairing, steal tails statiche, freeze su Pause, Stop distruttivo) + CACP `voice_probe_v1` (scenario `voice_test`) per validazione voice stealing e transizioni.
- **Inc-2 (Sintesi 4 Ruoli & 4 Bus):** implementa le firme timbriche e il routing di M2.4-D (primitive analitiche, bus dedicati, micro-ducking, protezione terminale comune ratificata in ADR-0004; soft-clipper solo eventuale stadio intermedio) + CACP `timbre_bus_probe_v1` (scenario `timbre_test`) per validazione timbri e protezione terminale.
- **Inc-3 (Music Planner Gerarchico & Candidate Search):** implementa la generazione simbolica di M2.4-A e M2.4-B (motivo relativo, trasformazioni pure, scoring deterministico, fallback bounded) + CACP `planner_probe_v1` (scenario `planner_test`) per validazione traccia simbolica e decisioni.
- **Inc-4 (Fixture A-B-A' 96 battute & Profiling Realtime):** implementa la forma tripartita a tre sezioni A-B-A' e la transizione di M2.4-E, unitamente al profiling Release $\le 2,0\text{ ms}$ + CACP `fixture_probe_v1` (scenario `fixture_test`) per validazione run 3 minuti e profiling $\le 2,0$ ms.
- **Inc-5 (Integrazione CACP, Artefatti WAV & Handoff M2.5):** implementa l'esportazione degli artefatti, i test quantitativi, consolidamento della suite diagnostica CACP completa e il dossier CACP di M2.4-F per il gate M2.5.

| Unità di lavoro | Deliverable | Prova minima | Condizione di uscita |
|---|---|---|---|
| **M2.4-A — Linguaggio v1** | Scheda artistica dei quattro ruoli, riferimenti di ascolto e anti-pattern; una grammatica a due sezioni; tipi interni di phrase/motif e note di decisione | Traccia simbolica esemplificativa spiegabile | Scelte riproducibili, pochi parametri espliciti; nessun cambio ADR implicito |
| **M2.4-B — Composizione pura** | Form/Phrase plan, 2–3 motivi relativi, trasformazioni conservative/developmental, ritmo con Bjorklund+accenti, funzione armonica e voicing bounded | Golden event trace, metamorphic tests di trasposizione/trasformazione, test seed isolation | Forma e memoria osservabili nel trace, nessun algoritmo non bounded |
| **M2.4-C — Polifonia e lifetime** | `VoiceSlot[8]`, 2 per ruolo, ADSR, pairing `NoteOn`/`NoteOff`, gestione epoch, stealing e tail di release | PCM test multilivello: note sovrapposte, off ritardato, cross-epoch, steal, pause/stop, overflow | Nessun cross-cancel o nota appesa; assenza di allocazioni/lock callback |
| **M2.4-D — Timbri e mix** | Prime firme soniche dei quattro ruoli; oscillatori antialiased/precomputati ove necessario, FM/wavetable selettivi, filtri/inviluppi/bus bounded | Offline WAV multi-seed, test clipping, stabilità, transient/click, CPU/profiling | Suono non monotono, budget voce/effetti rispettato, export tracciabile |
| **M2.4-E — Tre sezioni A-B-A'** | Prima run pianificata con esposizione, contrasto e ritorno variato in forma A–B–A′; transizioni `SetTempoMap` | Same-seed event trace, cambio tempo/metrica e prova audibile tra sezioni | Materiale pronto per M2.5; nessun PASS estetico auto-dichiarato |
| **M2.4-F — R&D controllata** | A/B compositivo baseline vs gerarchia, e A/B timbrico con renderer costanti; decision log | Protocollo [Evaluation](M2_4_Evaluation_Protocol.md), WAV e giudizi espliciti | Conclusione documentata: accettare, iterare o differire, con limiti |

Le unità B, C e D possono avere worktree separati se l'integrazione non rompe ownership e API; non integrare più di una nuova famiglia DSP prima della misura del relativo costo. M2.4-F è un *checkpoint artistico di qualità* non una sostituzione dei test obbligatori normativi.

## 4. Politiche progettuali ratificate in ADR-0003 (Invarianti e Profili)

### 4.1 Motivi, forma e scoring

- Motivo relativo a scala e griglia; identificatore interno stabile e invarianti trasformazionali. Una frase ha un `goal` armonico e un ruolo dominante.
- Candidate generator costruisce un insieme **finito e con limite hard** di opzioni; `hard constraints` prima della valutazione estetica; score a pesi versionati solo se giustificato.
- Il tie-break deve essere totale, stabile e indipendente dall'ordine dei container. Semi gerarchici separati per run/section/phrase/role/decision; preservare la policy RNG M1.
- Prima prova: variante A (scala + Euclidean), variante B (A+planner/motif). Il voice-leading e la variante C entrano solo dopo che B è riproducibile.
- Armonia: grafo finito di **stabilità → preparazione → tensione → risoluzione**, con vocaboli modali e accordi volutamente ridotti; non codificare una grammatica tonale universale.
- `tension` è **multidimensionale musicale** (armonia/ritmo/registro/timbro/densità), distinta dalla Dissonanza di salute.
- **Schema compositivo di prova per la run di 3 minuti (fixture-m2-4-v1, profilo sperimentale versionato):** forma tripartita A–B–A′ con 32 battute per sezione (96 battute totali, durata nominale 182,857125 s = 8.777.142 campioni a 48 kHz).
  1. *Sezione A (Esposizione e sviluppo):* Re Dorico, 120 BPM 4/4 (32 battute = 128 semiminime = 64,0 s = 3.072.000 campioni a 48 kHz). `Pulso` stabilisce la griglia euclidea fondamentale, `Corpo` presenta il fondamento armonico, `Vettore` espone il motivo principale `Motif_0`, `Trama` introduce contrappunto nella seconda metà.
  2. *Sezione B (Contrasto e tensione):* Transizione `SetTempoMap` su confine di battuta esatto a Sol Eolio, 140 BPM 4/4 (32 battute = 128 semiminime = 2.633.142 campioni a 48 kHz; inizio al frame assoluto 3.072.000, fine al frame assoluto 5.705.142). Il pad di `Corpo` sostiene la transizione cross-epoch; `Pulso` e `Trama` adottano nuove configurazioni euclidee; `Vettore` esegue variazioni trasformazionali del motivo. `MusicClock::setTempoMap()` riconcilia esattamente `deltaF == boundary.value` accettando il boundary al frame 2.633.142 (non utilizzare valori arrotondati come 2.633.143 che causerebbero `Rejected`).
  3. *Sezione A′ (Ritorno e cadenza):* Transizione `SetTempoMap` su confine di battuta esatto di ritorno a Re Dorico, 120 BPM 4/4 (32 battute = 128 semiminime = 64,0 s = 3.072.000 campioni a 48 kHz; inizio al frame assoluto 5.705.142, fine al frame assoluto 8.777.142). Ripresentazione variata di `Motif_0` e cadenza finale risolutiva. Triadi orchestrate distribuendo le note tra Corpo (max 2 voci) e Trama/Vettore.

### 4.2 Unità voci/identità

- **Contratto vigente:** massimo otto DSP voice slot, fissi per ruolo, due ciascuno, preallocati. *Numero di oscillatori interni allo slot* e *numero di note indipendenti* non vanno confusi; un'espansione composita che aggiri il limite va sottoposta a modifica ADR.
- **Pairing, rilascio failsafe e note aperte (`accepted for implementation` in ADR-0003):** lo scheduler emette `NoteOn`/`NoteOff` per `durationFrames > 0`. `AudioScheduler::dispatchDueEvents()` mantiene gli eventi staged durante la backpressure SPSC e ritenta il dispacciamento (distinguendo rigorosamente backpressure ordinaria, eventi tardivi e drop effettivo con NoteOff assente). Per eventi con `durationFrames > 0`, lo slot applica un timer failsafe interno su scadenza assoluta `sampleFrame + durationFrames` nella timeline logica a 48 kHz per prevenire note appese da drop effettivo. Per `durationFrames == 0` (gate aperto, pad di Corpo), il rilascio musicale ordinario è controllato esclusivamente dal matching `NoteOff` inviato dal planner all'orizzonte della frase/sezione. Come salvaguardia fail-closed di sicurezza, `AudioEngine` impone un ceiling `audioMaxLeaseDurationFrames` indipendente dalla struttura interna del planner e non inferito da `MusicClock` (es. bound conservativo di 36 battute sul tempo nominale; alla scadenza interviene il rilascio forzato in `Release` con incremento di `orphanNoteRecoveredCount`). Le transizioni `SetTempoMap` sono non distruttive e preservano le note attive in tutti i ruoli (inclusi i pad in Sustain) senza sweep distruttivo. Rimosso il timeout monofonico temporaneo di 1s; recupero deterministico globale garantito a confini, furti di voce o reset forzato a `Idle` su Stop/Reset/Nuova Run.
- **Voice stealing bounded con steal tails statiche (`accepted for implementation` in ADR-0003):** onset esatto del nuovo `NoteOn` sample-accurate senza ritardo; la voce uscente viene trasferita su una steal tail statica preallocata nello slot (max 1 tail per slot, 8 totali nel sistema) con rampa lineare discendente a zero di 2 ms (96 campioni a 48 kHz) come de-click progressivo dell'uscita (snapshot decay). Su saturazione da furti ravvicinati (< 96 campioni), la steal tail preallocata ricattura l'ampiezza istantanea per canale del segnale sommato in uscita ($S_{\text{ch}} = y_{\text{tail, ch}}[0] + y_{\text{voice, ch}}[0]$ senza clamp prematuro pre-master) e riavvia una nuova rampa lineare di 2 ms sulla medesima steal tail, garantendo decadimento continuo senza leak o allocazioni dinamiche, e incrementando `voiceDropCount`. Nessuna assunzione teorica di invarianza C0 universale o assenza a priori di click: l'efficacia del de-click deve essere validata empiricamente in Inc-1 con analisi PCM, misura dei picchi e ascolto multimodale su furti ravvicinati. Ordinamento totale deterministico di selezione (Regole 0-2): Regola 0 = slot `Idle` prioritario (0 prima di 1); Regola 1 = voce in Release (minore ampiezza $|A_0| < |A_1|$, `startFrame` più remoto, `slotIndex` 0 prima di 1); Regola 2 = voci attive non in Release (`startFrame` più remoto, `slotIndex` 0 prima di 1). Bonifica a 0.0f dei valori non finiti (`NaN`/`Inf`) tracciata in `dspAnomalyCount`.
- **Continuità cross-epoch e confine di commitment (`accepted for implementation` in ADR-0003):** note attive proseguono inalterate attraverso `SetTempoMap`; eventi nella finestra di lookahead SPSC (150 ms) sono committed e i loro `NoteOn` mantengono il `sampleFrame` nominale garantito; i `NoteOff` cross-epoch matching per `(role, eventId)` con epoch precedente vengono regolarmente onorati. Il planner ripianifica unicamente le intenzioni non committed nel Mutable Planning Horizon. L'implementazione storica M2.3 (`audio_engine.cpp`), che scartava indistintamente tutti i NoteOn di vecchia epoch, viene formalmente dichiarata come soggetta a migrazione durante lo sviluppo M2.4, con riesame motivato dei test storici (`[scheduler][epoch_drop]`).
- **Matrice stati di trasporto (`accepted for implementation` in ADR-0003):** distinzione rigorosa tra dominio logico e dominio fisico d'uscita. Nel dominio logico, `Pause` applica il freeze atomico immediato $O(1)$ di cursore, pianificazione, consumo SPSC, fasi, inviluppi ADSR e LFO al frame campionario nominale del comando ($t_{\text{pause}}$); `Stop` richiede l'arresto della generazione nel Coordinator e l'invalidazione dei piani futuri nel suo stato privato, mentre in `AudioEngine` resetta forzatamente a `Idle` tutti gli 8 slot (fasi azzerate, inviluppi spenti, `generationToken` incrementato), drena bounded la SPSC ed emette `AudioCommandAck::Applied` al confine logico effettivo. Nel dominio fisico, a `Pause` viene applicata una tail fisica d'uscita di 2 ms ($N_{\text{tail}} = \lfloor 0.002 \times f_{s,\text{device}} \rfloor$, 96 frame a 48 kHz, 88 frame a 44.1 kHz) all'indice di campione fisico $n_{\text{device, pause}}$ per prevenire click acustici; silenzio numerico assoluto (0.0f) garantito nel buffer device a partire da $n_{\text{device, pause}} + N_{\text{tail}}$. Se perviene `Resume` durante la coda fisica ($< N_{\text{tail}}$ campioni), la rampa inverte verso 1.0f dall'ampiezza fisica istantanea senza discontinuità di livello; a regime il trasporto riprende dal medesimo frame logico congelato con fade-in di 2 ms. Se perviene `Stop` ordinario durante la coda di de-click, l'attenuazione a zero prosegue regolarmente senza tagli netti (o applica il fade di 2 ms). Ordinamento deterministico sequenze ravvicinate intra-blocco con priorità assoluta di Stop/Emergency Mute su Pause/Resume. `Emergency Mute` applica silenzio immediato senza fade per guasti o terminazioni forzate.
- `voiceDropCount`, `orphanNoteCount` e `staleNoteOffCount` (nomi proposti, non API accettate) aiutano CACP e offline diagnosis. Telemetria esclusivamente bounded.

### 4.3 Palette timbrica, candidati DSP e gain staging

| Ruolo | Ipotesi iniziale | Vincolo artistico | Esperimento alternativo |
|---|---|---|---|
| Pulso | body sinusoidale/FM + rumore/transient breve | onset preciso e headroom per gameplay | piccolo set di transienti licenziati |
| Corpo | wavetable/subtractive + inviluppo espressivo | stabilità armonica anche con una diade | Karplus/waveguide semplice come layer distintivo |
| Trama | wavetable/noise filtrato con delay shared | texture percepita senza polifonia eccessiva | piccoli elementi percussivi/KS, granular solo spike |
| Vettore | wavetable/FM con modulazione/glide bounded | linea tematica leggibile, non sweep casuale | FM 2 vs 3 operatori |

Non tutte queste tecniche devono essere implementate nella prima iterazione. Preferire **una firma timbrica udibile e rifinita per ruolo** rispetto alla quantità di preset. Verificare antialiasing alla frequenza massima supportata, numerica dei filtri a bassa ampiezza, gain staging e separazione spettro/telegraph.

#### Profilo sperimentale di sintesi C++20 (proposte da verificare, non nuove prescrizioni ADR)

L’obiettivo è un timbro distintivo e riproducibile per ciascun ruolo, con DSP per-sample bounded, senza allocazioni o lock, nel target callback già accettato di 2 ms / 512 device frame a 48 kHz. Le tecniche seguenti sono **alternative misurabili**: selezionarle mediante WAV a intent identico, profiling e ascolto; non implementarle tutte in M2.4-A/D.

1. **Pulso — corpo percussivo:** sine/FM o forma d’onda opportunamente band-limited con pitch envelope rapido e transient noise generato da seed isolato. Reset della fase solo se desiderato per la firma del transient; verificare l’attacco, la continuità e il bilanciamento tra punch, headroom e feedback ludico. Tempi dell’inviluppo (es. 15–40 ms) sono valori iniziali da confrontare.
2. **Corpo — basso/pad:** confrontare wavetable band-limited o PolyBLEP con filtri SVF/TPT e biquad opportunamente implementati, stabilizzati e sottoposti a smoothing. Non vietare per principio Direct Form I/II: scegliere la topologia dopo stress test su cutoff, risonanza, modulazione, precisione numerica e costo. Il suono deve conservare continuità nei sustain cross-epoch.
3. **Trama — texture/contrappunto:** FM a due operatori e, come spike opzionale, Karplus–Strong/digital waveguide con delay line statica e interpolazione frazionaria quando necessaria. A 48 kHz una linea ideale di 160 campioni corrisponde a 300 Hz; non esiste una soglia universale di errore grave *sopra* 300 Hz. Una linea da 1.024 campioni impone invece una frequenza minima nominale vicina a 46,9 Hz prima della fase del filtro. Studiare tuning e stabilità del loop nel range realmente usato.
4. **Vettore — lead:** FM a due operatori, wavetable o PolyBLEP con glide/vibrato deterministici. Un eventuale indice `I(t)` dipendente da `timbreParam` va limitato in relazione a frequenza modulante, sideband significative e Nyquist, con controllo spettrale; scalarlo come `1/f_c` non prova assenza di aliasing. Valutare oversampling/bandlimiting soltanto se giustificati dal profilo CPU.
5. **Stabilità e smoothing:** controllare NaN/Inf, velocità dei controlli, code di inviluppo e subnormali. FTZ/DAZ dove supportato o flush software degli stati sono candidate tecniche di mitigazione, valutate rispetto a portabilità e correttezza numerica. Smoothing one-pole 5–10 ms è un intervallo di prova per **controlli continui**, non un obbligo per NoteOn, transienti o azioni discrete; versionare i parametri selezionati.
6. **Bus, mix ed effetti:** mantenere un numero statico/bounded di bus per i quattro ruoli. Esplorare sidechain leggero e delay condiviso soltanto se migliorano leggibilità e coesione senza violare il budget; gain di ruolo (ad esempio −6/−8/−10/−6 dB) sono punti di ascolto iniziali e **non** garantiscono headroom sommando otto segnali in fase. Misurare somma peggiore, transient peak, true peak e margine master. Se si sceglie un soft-clipper/limiter, dimostrare che la funzione sia effettivamente bounded nel dominio d’ingresso o imporre un ceiling esplicito; una forma razionale di `tanh` non è automaticamente limitata a ±1.
7. **Telemetria per ruolo:** definire misure bounded e interpretabili sui bus/slot (onset dal trigger, energy dal segnale o da inviluppo, density da occupancy, pitch dalla voce rilevante, confidence dal criterio noto). Confrontare feature e costo con eventuali analisi offline; non eseguire FFT ridondanti in callback senza motivazione e test. La telemetria non sostituisce la traccia simbolica autoritativa.

Per ciascuna scelta registrare parametro/profilo, seed, intento identico, WAV, evidenza uditiva, costo medio e coda della distribuzione, massimi osservati e possibili regressioni. Solo le decisioni che alterano contratti accettati, ABI o voice budget richiedono revisione dell’ADR; il tuning reversibile resta versione/profilo sperimentale.

## 5. Realtime, determinismo e test

### Deadline e budget

L'addendum a D-M2-03 introdotto nel commit `460bc28637` fissa **target ≤ 2,0 ms per blocco da 512 device frame** nella configurazione 48 kHz: circa il 18,75% di `512/48000 s = 10,667 ms`. Il precedente test M2.3 a 50 ms rileva hanging/loop non bounded ma **non certifica audio realtime**. Il rispetto del target deve essere **misurato**, non assunto.

- Misurare costo di sintesi/mix su release build, sequenze realistiche e worst-case 8 voci + release/steal + transizioni, con CACP disattivato/attivato e queue stress; registrare CPU, OS, device, quantum effettivo e compilers.
- Registrare percentili (p50/p95/p99/p99.9), massimo osservato, underrun, drop e gap; il massimo empirico non è worst-case garantito. Evitare output I/O del profiler nella callback; trasmettere contatori e timestamp in ring preallocato.
- 44,1/48 kHz e dimensioni buffer diverse richiedono contabilità e limiti misurati nel dominio **device**, non interpretare 96 frame device come 2 ms in ogni configurazione.
- Eseguire soak e ascolto hardware durante M2.5, con toolchain e macchina identificate; non promettere zero underrun "su qualsiasi configurazione" senza test.

### Determinismo

Canonical output per M2.4: piano simbolico/versionato, `MusicIntentEvent` ordinati, seed, decision trace, stato di forma e metriche pertinenti. Non asserire bit-identità PCM fra backend, ISA/CPU e piattaforme senza prova. Test golden non devono verificare il valore pubblicato dalla stessa funzione che implementano.

### Test tecnici obbligatori da prevedere

1. Pairing multilivello `(role,eventId)`: note sovrapposte, `NoteOff` out-of-order, epoch cambio, voice stealing, flush/stop, release interrotto, note zero-length.
2. Scheduler: timestamp esatti inclusi confini di blocco, meter/tempo change intra-block, old/future epoch, saturation budget e late event.
3. DSP: zero heap/lock/logging nella callback, bounded work e misure di deadline, PCM finito (no NaN/Inf), mitigazione subnormali profilata, range validi di frequenza/gain, test aliasing per ogni renderer scelto, headroom/campionamento e true peak analizzati separatamente, discontinuità/click ai boundary osservati senza una soglia arbitraria resa mandatory; `ChannelFrame` con semantica/versione e telemetria coerenti.
4. Composizione: same-seed trace identico, seed isolation, trasformazioni che preservano invarianti, novelty controllata, risoluzione delle frasi, ruolo dominante distinguibile.
5. CACP (`AC-AO-02`): onset e sample timing correlati a trace/intent sul WAV di `AudioProbe`, con offset e conversioni device/logical dichiarati; integrità dei 3 minuti, validità manifest, prove cross-epoch dei pad e giudizio percettivo separato dalle misure automatiche. Rispettare i criteri obbligatori esistenti senza inventarne di nuovi.

## 6. Quadro normativo consolidato ADR-0003 e perimetro di future revisioni

Tutte le decisioni d'ingresso per la sottofase M2.4 sono formalmente definite e ratificate in [ADR-0003](../adr/0003-m2-decisions-contracts.md) (decisioni D-M2-04..07, stato `accepted for implementation (review remediation pending)`). Nessuna decisione d'ingresso architetturale rimane aperta per l'avvio della sequenza operativa Inc-1..Inc-5.

La tabella seguente riepiloga il perimetro dei contratti congelati rispetto a eventuali future revisioni normative (riservate a milestone successive o a evidenze empiriche formali):

| Ambito | Stato in ADR-0003 / M2.4 | Condizione per eventuale revisione normativa futura |
|---|---|---|
| Pool 8 voci statiche (2/ruolo) | Normativo (`accepted for implementation`) | Benchmark/ascolto dimostrano fallimento riproducibile e miglioramento superiore alle alternative sparse |
| `MusicIntentEvent` (IR pubblico) | Normativo invariato (D-M2-02) | Necessità concreta di ID strutturali per replay/CIE con contratto pubblico in M3+, non per comodità interna |
| Bounded Candidate Search & Tie-Break | Normativo (`accepted for implementation`) | Vincoli hard di sistema; tie-break deterministico totale e stabile; scoring e pesi sono profili versionati |
| Fixture A-B-A' 96 battute (~182,86s) | Profilo sperimentale versionato (`fixture-m2-4-v1`) | Calibrazione artistica o test di cambi metrici ulteriori approvati come profili aggiuntivi, non per presunti limiti matematici |
| Palette timbrica e Gain staging | Profilo sperimentale versionato (`synth-palette-v1`) | Taratura fine mediante ascolto e WAV; future estensioni timbriche (KS, wavetable) subordinate a misura CPU |
| Target callback $\le 2,0\text{ ms}$ / 512 frame | Normativo Release (D-M2-07) | Evidenze su target hardware eterogenei in M2.5; mai declassare il target a runtime senza revisione ADR |
| Semantica musica $\to$ geometria | Esclusa da M2.4 (riservata a M3+) | Prima della generazione di geometria dipendente da motivi in M3/M6 definire shared plan e fairness validation |

## 7. Handoff a M2.5

Consegnare: build/commit, profilo sonoro, seed e versioni, almeno un WAV 3 minuti con 2 sezioni e transizione, event trace golden, manifest CACP, run di stress 8 voci, report callback e note sulle scelte timbriche, evidenze di ascolto chiaramente separate per umano/agente, e gap noti. M2.5 deve verificare **tutti** i criteri obbligatori della specifica: un checkpoint artistico non è un sostituto dei PASS tecnici e viceversa.

Le proposte sperimentali differite vanno nei backlog M3–M8 secondo [roadmap_music_research_addendum.md](../roadmap_music_research_addendum.md), non inserite silenziosamente nel runtime.
