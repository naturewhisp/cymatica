# CYMATICA M2.4 — Piano di integrazione compositiva e DSP v1

- **Stato:** **proposta di implementazione, da confermare in ingresso M2.4**; non sostituisce la roadmap normativa.
- **Baseline:** commit `460bc286378b473218bc07f751e3615964626a11` (2026-10-13).
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

### 4.2 Unità voci/identità

- **Contratto vigente:** massimo otto DSP voice slot, fissi per ruolo, due ciascuno, preallocati. *Numero di oscillatori interni allo slot* e *numero di note indipendenti* non vanno confusi; un'espansione composita che aggiri il limite va sottoposta a modifica ADR.
- `NoteOn` e relativo `NoteOff` si correlano tramite almeno `(role, eventId)` più **unicità/ambito di validità** dell'ID. Se `eventId` è univoco nella run, documentare overflow e restart; se è riutilizzabile, la chiave deve includere un generation token/epoch appropriato. Non basta `activeNoteEpoch_`.
- Al voice stealing conservare ownership/token di slot: un vecchio `NoteOff` non interrompe il nuovo occupant. Testare steal al confine di blocco e nel mezzo di un micro-fade.
- Note con `durationFrames` esplicito: gestire NoteOff tardivo, perso, duplicato, non matching e superiore al budget; nessuna release infinita. Le note sustain legittime non devono essere troncate dal timeout **globale M2.3** di 1 s dopo epoch.
- `Pause`, `Stop`, replay reset, device restart, errore trasporto, nuove run: definire comportamento sicuro di tail, resumption e flush. Un silenzio immediato può essere necessario per stop tecnico; altrimenti preferire micro-fade anti-click con comportamento sample-accurate e deterministico.
- `voiceDropCount`, `orphanNoteCount` e `staleNoteOffCount` (nomi proposti, non API accettate) aiutano CACP e offline diagnosis. Telemetria esclusivamente bounded.

### 4.3 Palette timbrica

| Ruolo | Ipotesi iniziale | Vincolo artistico | Esperimento alternativo |
|---|---|---|---|
| Pulso | body sinusoidale/FM + rumore/transient breve | onset preciso e headroom per gameplay | piccolo set di transienti licenziati |
| Corpo | wavetable/subtractive + inviluppo espressivo | stabilità armonica anche con una diade | Karplus/waveguide semplice come layer distintivo |
| Trama | wavetable/noise filtrato con delay shared | texture percepita senza polifonia eccessiva | piccoli elementi percussivi/KS, granular solo spike |
| Vettore | wavetable/FM con modulazione/glide bounded | linea tematica leggibile, non sweep casuale | FM 2 vs 3 operatori |

Non tutte queste tecniche devono essere implementate nella prima iterazione. Preferire **una firma timbrica udibile e rifinita per ruolo** rispetto alla quantità di preset. Verificare antialiasing alla frequenza massima supportata, numerica dei filtri a bassa ampiezza, gain staging e separazione spettro/telegraph.

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
3. DSP: zero heap/lock/logging nella callback, bounded work, no NaN/Inf/denorm issues, range frequency/gain, antialiasing, peak/true peak controllo finale.
4. Composizione: same-seed trace identico, seed isolation, trasformazioni che preservano invarianti, novelty controllata, risoluzione delle frasi, ruolo dominante distinguibile.
5. CACP: onset e sample timing correlati a intent, WAV continuo e manifest validi; confronto con evidenza dell'ascolto e non solo analisi FFT.

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
