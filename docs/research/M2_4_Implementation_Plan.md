# CYMATICA M2.4 — Piano di integrazione compositiva e DSP v1

- **Stato:** **piano operativo allineato ai contratti d'ingresso M2.4 ratificati in [ADR-0003](../adr/0003-m2-decisions-contracts.md)** (2026-10-14); non sostituisce la roadmap normativa.
- **Baseline:** commit `460bc286378b473218bc07f751e3615964626a11` (2026-10-13), aggiornato con formalizzazione D-M2-04..07 (2026-10-14).
- **Fonti:** [ricerca revisionata](M2_4_Deep_Research_Reviewed.md), [review metodologica](M2_4_Research_Review.md), [Spec §16/§32](../../CYMATICA_Specifica_Agentica_Sviluppo.md), [ADR-0003](../adr/0003-m2-decisions-contracts.md), [DESIGN §8/§16](../../DESIGN.md).
- **Owner del piano:** Coordinatore sviluppo M2.4; scelta artistica finale del titolare. Non attribuire un'autorità normativa autonoma al documento.

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

## 3. Sequenza di sviluppo proposta

| Unità di lavoro | Deliverable | Prova minima | Condizione di uscita |
|---|---|---|---|
| **M2.4-A — Linguaggio v1** | Scheda artistica dei quattro ruoli, riferimenti di ascolto e anti-pattern; una grammatica a due sezioni; tipi interni di phrase/motif e note di decisione | Traccia simbolica esemplificativa spiegabile | Scelte riproducibili, pochi parametri espliciti; nessun cambio ADR implicito |
| **M2.4-B — Composizione pura** | Form/Phrase plan, 2–3 motivi relativi, trasformazioni conservative/developmental, ritmo con Bjorklund+accenti, funzione armonica e voicing bounded | Golden event trace, metamorphic tests di trasposizione/trasformazione, test seed isolation | Forma e memoria osservabili nel trace, nessun algoritmo non bounded |
| **M2.4-C — Polifonia e lifetime** | `VoiceSlot[8]`, 2 per ruolo, ADSR, pairing `NoteOn`/`NoteOff`, gestione epoch, stealing e tail di release | PCM test multilivello: note sovrapposte, off ritardato, cross-epoch, steal, pause/stop, overflow | Nessun cross-cancel o nota appesa; assenza di allocazioni/lock callback |
| **M2.4-D — Timbri e mix** | Prime firme soniche dei quattro ruoli; oscillatori antialiased/precomputati ove necessario, FM/wavetable selettivi, filtri/inviluppi/bus bounded | Offline WAV multi-seed, test clipping, stabilità, transient/click, CPU/profiling | Suono non monotono, budget voce/effetti rispettato, export tracciabile |
| **M2.4-E — Due sezioni** | Prima run pianificata con esposizione, sviluppo, contrasto e almeno un ritorno trasformato; transizione `SetTempoMap` | Same-seed event trace, cambio tempo/metrica e prova audibile tra sezioni | Materiale pronto per M2.5; nessun PASS estetico auto-dichiarato |
| **M2.4-F — R&D controllata** | A/B compositivo baseline vs gerarchia, e A/B timbrico con renderer costanti; decision log | Protocollo [Evaluation](M2_4_Evaluation_Protocol.md), WAV e giudizi espliciti | Conclusione documentata: accettare, iterare o differire, con limiti |

Le unità B, C e D possono avere worktree separati se l'integrazione non rompe ownership e API; non integrare più di una nuova famiglia DSP prima della misura del relativo costo. M2.4-F è un *checkpoint artistico di qualità* non una sostituzione dei test obbligatori normativi.

## 4. Politiche progettuali iniziali (da decidere prima dell'implementazione dipendente)

### 4.1 Motivi, forma e scoring

- Motivo relativo a scala e griglia; identificatore interno stabile e invarianti trasformazionali. Una frase ha un `goal` armonico e un ruolo dominante.
- Candidate generator costruisce un insieme **finito e con limite hard** di opzioni; `hard constraints` prima della valutazione estetica; score a pesi versionati solo se giustificato.
- Il tie-break deve essere totale, stabile e indipendente dall'ordine dei container. Semi gerarchici separati per run/section/phrase/role/decision; preservare la policy RNG M1.
- Prima prova: variante A (scala + Euclidean), variante B (A+planner/motif). Il voice-leading e la variante C entrano solo dopo che B è riproducibile.
- Armonia: grafo finito di **stabilità → preparazione → tensione → risoluzione**, con vocaboli modali e accordi volutamente ridotti; non codificare una grammatica tonale universale.
- `tension` è **multidimensionale musicale** (armonia/ritmo/registro/timbro/densità), distinta dalla Dissonanza di salute.
- **Schema compositivo di prova per la run di 3 minuti (non template obbligatorio):** forma con esposizione/sviluppo, contrasto e ritorno trasformato. Durate e sezioni sono obiettivi di orchestrazione indicativi: fissare i veri `sampleFrame` tramite `MusicClock`, non partendo da minuti arrotondati.
  1. *Esposizione e sviluppo:* `Pulso` stabilisce la griglia, `Corpo` presenta fondamento armonico e `Vettore` espone `Motif_0`; la seconda frase introduce `Trama` e una trasformazione conservativa. A 120 BPM in 4/4, un eventuale cambio dopo 36 battute cade a 72 secondi ed è un bar boundary valido; è **solo un esempio**, non una durata fissa.
  2. *Contrasto:* introdurre `SetTempoMap` soltanto al beat boundary accettato dal clock (bar boundary per cambi di metrica); `Pulso` e `Trama` adottano la nuova griglia, mentre il pad già attivo di `Corpo` segue il lifecycle cross-epoch. Durata della sezione ricavata dal numero di barre/frasi nel nuovo tempo, non dall’ora `1:15` arbitraria.
  3. *Ritorno e risoluzione:* ripresentare un `Motif_0` riconoscibile, trasposto o riorchestrato, e chiudere la frase/cadenza su boundary derivati dal clock. Adattare le durate musicali per un rendering continuo di almeno 180 s con finale documentato, senza cambi di metrica a metà battuta.

### 4.2 Unità voci/identità

- **Contratto vigente:** massimo otto DSP voice slot, fissi per ruolo, due ciascuno, preallocati. *Numero di oscillatori interni allo slot* e *numero di note indipendenti* non vanno confusi; un'espansione composita che aggiri il limite va sottoposta a modifica ADR.
- **Pairing e rilascio failsafe (proposta):** il scheduler M2.3 emette già `NoteOn`/`NoteOff` per `durationFrames > 0`. Non introdurre un secondo rilascio non idempotente: testare un fallback interno allo slot con scadenza assoluta `sampleFrame + durationFrames` nella timeline logica a 48 kHz, usato quando manca il `NoteOff`, non un countdown cieco in frame del device. Un `NoteOff` matching deve poter avviare il Release una sola volta; gestire eventi tardivi, duplicati, persi, fuori ordine e post-steal. La semantica delle note con `durationFrames == 0` è una decisione d’ingresso da registrare; non assumere un gate implicito.
- **Voice stealing con micro-rilascio misurabile:** applicare la policy di selezione ADR-0003 (preferire inviluppo più avanzato, due slot per ruolo) e rispettare il `sampleFrame` della nuova nota. Confrontare micro-fade di 2 ms, crossfade bounded e coda DSP transitoria che conservi una parte appropriata dello stato uscente. Nessuna di queste tecniche da sola garantisce matematicamente assenza di click o di artefatti timbrici: validare valore/derivata del PCM, ascolto e costo realtime. Un token interno impedisce vecchi `NoteOff` sul nuovo occupante, senza nuovi campi pubblici obbligatori.
- **Continuità cross-epoch e revisione del piano:** preservare una nota già attiva in `SetTempoMap` con `NoteOff` matching, senza assumere che gli eventi futuri di epoch precedente restino tutti validi. Definire separate politiche per eventi committed (frame garantiti), eventi non committed (ripianificazione) e note sostenute, compatibili con la SPSC e con l’ordinamento M2.3. `Pause` recuperabile congela il transport secondo la policy da scegliere; `Stop`/nuova run eliminano il piano precedente. Non imporre flush di tutte le voci durante una pausa senza una decisione motivata.
- `Pause`, `Resume`, `Stop`, replay reset, device restart, errore trasporto e nuova run: definire una matrice di stati che specifichi cursore logico, slot/inviluppi, queue/epoch e uscita sonora. Se il device emette subito silenzio, non dichiarare eseguito un fade di 2 ms; una coda udibile richiede rendering bounded e una transizione compatibile con Stop. Registrare differenze rispetto al tono monofonico M2.3 e aggiungere test sample-accurate.
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

## 6. Decisioni aperte e criteri di promozione a ADR

| Questione | Baseline | Trigger per una nuova decisione normativa |
|---|---|---|
| 8 voci/2 per ruolo | Invariata | Benchmark/ascolto dimostrano fallimento riproducibile e miglioramento superiore alle alternative sparse |
| `MusicIntentEvent` | Invariato | Necessità concreta di ID strutturali per replay/CIE con contratto pubblico, non semplice eleganza |
| Motif grammar | Proposta | Evidenza di coerenza senza monotonia su seed e ascolti pilota |
| Renderer KS/granulare | Spike opzionale | Vantaggio timbrico riconoscibile e costo DSP ammissibile |
| Perceptual metric threshold | Nessun threshold artistico normativo M2.4 | Studio pilota, misure affidabili e scelta deliberata del titolare |
| 2 ms / 512 frame | Target già documentato ADR-0003 | Misure su hardware reali potrebbero richiedere profili/soglie distinti: cambiare tramite ADR, non nascondere il miss |
| Semantica strutturale musica→geometria | Non pubblica in M2.4 | Prima della generazione di geometria dipendente da motivi in M3/M6 definire shared plan e fairness validation |

## 7. Handoff a M2.5

Consegnare: build/commit, profilo sonoro, seed e versioni, almeno un WAV 3 minuti con 2 sezioni e transizione, event trace golden, manifest CACP, run di stress 8 voci, report callback e note sulle scelte timbriche, evidenze di ascolto chiaramente separate per umano/agente, e gap noti. M2.5 deve verificare **tutti** i criteri obbligatori della specifica: un checkpoint artistico non è un sostituto dei PASS tecnici e viceversa.

Le proposte sperimentali differite vanno nei backlog M3–M8 secondo [roadmap_music_research_addendum.md](../roadmap_music_research_addendum.md), non inserite silenziosamente nel runtime.
