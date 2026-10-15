# CYMATICA M2.4 — Review indipendente della Deep Research sull'architettura audio

- **Data:** 2026-10-15.
- **Stato:** ricerca tecnica revisionata, **non normativa**; nessuna modifica al runtime attestata.
- **Baseline verificata:** `main` a `090dc2a45cf3785a2055c492415f9151321ea18a` (2026-10-14).
- **Input:** rapporto fornito dal titolare, *CYMATICA — Deep Research conclusiva sull'architettura audio M2.4*; addendum antecedente su transport/sidechain, entrambi revisionati criticamente. I marcatori `turn...search...` o `filecite` della sessione originale **non sono citazioni trasferibili al repository**; qui sono sostituiti da collegamenti pubblici e durevoli.
- **Autorità:** [Specifica](../../CYMATICA_Specifica_Agentica_Sviluppo.md) > [DESIGN](../../DESIGN.md) > [AGENTS](../../AGENTS.md) > [ADR-0003](../adr/0003-m2-decisions-contracts.md) e [ADR-0004 ratificato](../adr/0004-m2-4-audio-renderer-safety-proposal.md) > [piano operativo](../progress.md) > ricerca. La review qui conserva lo stato storico delle **evidenze**, non annulla la ratifica architetturale del 2026-10-15.

## 1. Esito della review

**Raccomandazione architetturale favorevole, ma non prova comparativa definitiva:** mantenere per M2.4 il **renderer nativo C++20** con gli otto voice slot e i quattro bus attualmente approvati, privilegiando il **riuso selettivo di moduli DSP auditati** quando ciò migliora il rapporto qualità / complessità. È l'opzione **B** del rapporto: non impone né DaisySP, né Airwindows, né un nuovo framework.

Un host CLAP/VST3 **non è necessario come dipendenza del runtime M2.4**. L'eventuale valutazione di Surge XT come riferimento artistico e di un piccolo host CLAP offline può rimanere esperimento successivo e indipendente, previa autorizzazione. Il valore proprietario di CYMATICA risiede nel motore di composizione e nella sua relazione con gameplay, geometria e tempo, non nell'obbligo di riscrivere ogni oscillatore o filtro.

**Decisione ancora richiesta al titolare:** se accettare formalmente (a) la preferenza per riuso DSP selettivo, (b) una protezione terminale digitale più forte del soft-clipper corrente e (c) la roadmap del monitoraggio LUFS/true peak. La ricerca non modifica automaticamente D-M2-06/D-M2-07 o i gate di M2.5.

## 2. Verifiche indipendenti delle principali affermazioni

| Argomento | Risultato della verifica | Conseguenza CYMATICA |
|---|---|---|
| `MusicIntentEvent`, 48 kHz, 120 Hz, epoch | Contratti già presenti in ADR-0003 e M1/M2.3 | Preservare IR, `MusicClock` e scheduler |
| CLAP | Il formato ammette eventi `CLAP_EVENT_TRANSPORT` con offset campionario e note/parametri timestamped | Possibile adapter derivativo senza secondo clock; capacità del plugin specifico da qualificare |
| VST3 | `ProcessContext` è fornito per process block; il sub-block splitting è una soluzione dell'host per boundary esatti | Non confondere sample-accurate degli eventi nota con aggiornamento intra-call del `ProcessContext` |
| MIDI Clock e sidechain | Il primo sincronizza dispositivi; il secondo usa segnali audio come controllo | Non inserire MIDI Clock o sidechain nel critical path temporale fra `MusicClock` e DSP |
| CACP | Il sistema esistente è **Control & Observation Plane**, con runner normalizzati e `AudioProbe` | Può comandare scenari e misurare, ma **non fornisce** il clock realtime |
| Airwindows | Repository e processori diffusi sotto licenza MIT, con obblighi di conservazione avviso | Candidati per integrazione mirata, previo audit modulo per modulo |
| DaisySP | Repository principale MIT; esiste un progetto distinto DaisySP-LGPL (LGPL-2.1) | Verificare esattamente sorgenti, dipendenze e licenze dei moduli selezionati |
| Surge XT | Sorgente GPL-3.0-or-later; plugin CLAP/VST3 e Surge XT Effects | Ottimo confronto di qualità; fork/embedding e redistribuzione richiedono analisi legale |
| VST3 SDK | SDK ufficiale attualmente MIT | Licenza dell'SDK **non** trasferita automaticamente a plugin e asset |
| ITU-T H.872 §9.3.1 | Raccomandazione per game audio output scaling: integrated 30 min attorno a −23 LUFS ±2 LU, true peak max −1 dBTP; eccezioni/ulteriori feature nel testo | Ottimo riferimento **proposto**, non target CYMATICA già ratificato né garanzia acustica |
| BS.1770-5 / EBU Tech 3341 | Misure K-weighted/gated; Meter Momentary 400 ms, Short-term 3 s, Integrated | Separare loudness da sample peak, true peak e livello acustico reale |

### Distinzioni tecniche da preservare

- **Determinismo del piano** ≠ identità bit-a-bit del PCM su qualunque CPU, driver o plugin. Il trace simbolico canonico resta riproducibile; il PCM richiede un profilo/ambiente di riferimento identificato.
- **Plugin headless** ≠ fork che elimina la UI. Le API permettono normalmente di elaborare senza creare l'editor; lo stripping non è una ottimizzazione DSP dimostrata.
- **Numero di eventi NoteOn inviati** ≠ numero reale di oscillatori/voice DSP in un plugin non controllato. Il budget di due voci per ruolo è contrattualmente diretto solo nel renderer nativo.
- **`eventId` uint64** ≠ `note_id` int32 nei plugin. Un eventuale host richiede mapping bounded, privo di riuso prematuro e di collisioni durante note attive/NoteOff tardivi.
- **Sample-accurate del protocollo** ≠ prova di comportamento sample-accurate di tutti i plugin. Testare transport, note-off, automazione, Pause/Resume e delay con fixture PCM.
- **Sample peak ≤0 dBFS** ≠ true peak limitato, loudness sicura o ascolto fisicamente sicuro; il dispositivo e il volume dell'utente non sono calibrati dal gioco.
- **CACP controlla scenari** tramite il Coordinator, ma non si interpone nella catena `MusicClock → renderer`. La callback deve mantenere boundedness indipendentemente dalla telemetria.

## 3. Alternative e forza delle evidenze

| Opzione | Idoneità architetturale preliminare | Stato evidenze |
|---|---|---|
| A — C++20 DSP tutto proprietario | Coerente ma reimplementa primitive mature; costo di rifinitura artistica ignoto | Nessun A/B attuale |
| **B — C++20 nativo + moduli auditati** | **Preferenza motivata**: conserva ownership, budget, portabilità; apertura a riuso DSP | Fonti/licenze verificate; CPU e qualità dei singoli moduli **da misurare** |
| C — CLAP come renderer primario | Trasporto potente ma host completo, stato e distribuzione aumentano complessità | API note; performance di Surge su CYMATICA ignote |
| D — VST3 come renderer primario | Come C, più gestione sub-block ai confini transport | API note; qualifica dei plugin ignota |
| E — nativo + backend plugin opzionale | Potenziale forte per confronti offline/sound design, ma non necessario in M2.4 | Ipotesi futura, non deliverable M2.4 |

I pesi e punteggi `1..5` del rapporto sorgente sono **stime editoriali**, non dati sperimentali. Il 25% attribuito alla conformità ai contratti correnti favorisce inevitabilmente l'architettura preesistente (rischio di *status-quo bias*). La preferenza B è difendibile anche senza far passare i punteggi come prova: minimizza la superficie di nuovo hosting mentre consente migliorie DSP controllate. Il giudizio sulla **qualità musicale relativa** resta sospeso fino ad A/B a pari loudness.

## 4. Architettura candidata e confini di responsabilità

```text
MusicClock 48k e Coordinator (autoritativi)
   |
Form/Phrase Planner -> MusicIntentEvent / committed scheduler
   |
Native VoiceSlot[8], 2 per ruolo -> synth DSP auditato
   |
4 bus musicali -> role FX preallocati -> Music Mix
                                           |
Future SFX / UI (se presenti) --------------+--> Game Master
                                                  |
                                        gain staging + finite guard
                                                  |
                                         FINAL PROTECTION (*)
                                                  |
                                        AudioProbe -> device
                                                  |
                                      worker CACP / meter offline
```

`(*)` Il **principio** di protezione terminale è ratificato dall'[ADR-0004](../adr/0004-m2-4-audio-renderer-safety-proposal.md); algoritmo, ceiling e ordine rispetto a resampling/gain rimangono oggetto dello [studio tecnico](M2_4_Audio_Safety_Design.md) e non sono feature implementate. Qualsiasi amplificazione o conversione **dopo** il limiter può invalidare il suo ceiling; la validazione riguarda i campioni effettivamente consegnati al dispositivo.

I ruoli Pulso/Corpo/Trama/Vettore rimangono concetti di composizione e telemetria. Per il micro-ducking già previsto è razionale iniziare dagli onset di Music Intent, mentre un sidechain dal PCM è utile quando serve seguire l'energia effettiva del segnale. Nessuna obbligatorietà di plugin o MIDI viene introdotta in M2.4.

## 5. Ammissione di moduli DSP terzi: checklist minima

Un algoritmo open source non è automaticamente pronto per una callback audio. Per ogni **modulo concreto**, prima dell'uso, registrare:

1. Repository, commit/tag e SHA di eventuale file copiato; licenza esatta, avvisi richiesti e provenienza degli asset.
2. API e ciclo di vita; init fuori callback; buffer/stato preallocati; nessun lock, I/O, logging o heap in processing.
3. Numero massimo bounded di operazioni, memoria, parametri, automazioni e comportamenti di errore. Cercare `rand()` o RNG globale; ricondurre ogni eventuale casualità a stato indipendente deterministico o dimostrarne l'irrilevanza.
4. Test sui casi estremi: NaN/Inf, denormali, frequenze limite, cutoff/risonanza, feedback, input improvvisi, sample rate 44.1/48 kHz.
5. Profiling release 8 voci + bus + master + eventuale AudioProbe, nei carichi peggiori; p50/p95/p99/p99.9/max, budget complessivo <=2.0 ms a 512/48k nel profilo normativo.
6. PCM/WAV a intent identico, livello compensato; ascolto A/B umano e osservazione supplementare CACP.
7. Ingresso nel [registro dipendenze](../dependencies.md) **soltanto all'adozione reale**, con versioni fissate. Il materiale sperimentale non è una dipendenza attiva.

Non adottare tutte le librerie indicate nel rapporto per principio. Un modulo proprietario semplice e verificato resta una scelta valida quando l'integrazione esterna non offre vantaggi dimostrabili.

## 6. Decisioni, incrementalità e sicurezza

- **Inc-1:** voice lifecycle, 8 slot, epoch e contract tests invariati; non far dipendere la logica delle voci da Surge/CLAP/DaisySP.
- **Prima di Inc-2:** scelta delle primitive DSP da auditare; approvazione dell'eventuale nuovo profilo di protezione finale, inclusi limiti di latenza/oversampling e impatto sul budget. D-M2-06 attualmente garantisce solo protezione bounded del **sample peak**.
- **Inc-2:** implementare soltanto i DSP necessari ai 4 ruoli e misurare prestazioni. Nessun host plugin obbligatorio.
- **Inc-3:** Music Planner e determinismo non cambiano in base alla fonte dei filtri/oscillatori.
- **Inc-4:** fixture A–B–A' (96 battute, 8.777.142 frame); benchmark callback e misure CACP ON/OFF; eventuali prime misure EBU come analisi esplorativa.
- **Inc-5 / M2.5:** correlazione intent↔PCM, analisi di sample peak e true peak e dossier tecnico/percettivo; includere la protezione terminale ratificata; nuovi gate numerici/di conformità **soltanto se ratificati** prima dell'implementazione.
- **M3 e oltre:** quando si introdurranno SFX/UI, instradarli nello **stesso master finale**, o deliberare esplicitamente la soluzione equivalente; non affermare che il limiter protegga sorgenti esterne al mixer.
- **Sessioni lunghe:** un audit rolling 30 min non è verificabile con la sola fixture di ~183 s; prevedere una prova di durata adeguata prima di dichiarare applicata la raccomandazione H.872.

La sicurezza fisica dell'udito non è deducibile dal solo PCM. Nessuna soglia LUFS o dBTP consente di garantire la pressione sonora alle cuffie non calibrate. Per criteri e possibili ratifiche vedere [safety design](M2_4_Audio_Safety_Design.md) e [ADR-0004 ratificato](../adr/0004-m2-4-audio-renderer-safety-proposal.md).

## 7. Findings e disposizioni

| ID | Gravità | Osservazione | Disposizione |
|---|---|---|---|
| AUD-R1-01 | **HIGH** | Soft-clipper bounded di D-M2-06 non certifica true peak né raccomandazione safe-listening | Proposta ADR distinta prima di Inc-2; mantenere verità dello stato corrente |
| AUD-R1-02 | **HIGH** | Monitoring LUFS esteso non deve dipendere dalla ring AudioProbe senza garantire acquisizione completa | Worker/analisi bounded, dichiarare gap, prova rolling 30m separata |
| AUD-R1-03 | MEDIUM | Rapporto originale impiega citazioni temporanee, non copiabili in GitHub | Bibliografia primaria durevole inclusa di seguito |
| AUD-R1-04 | MEDIUM | Ranking numerico editoriale favorisce i contratti attuali; qualità artistica non misurata | Decisione B motivata come scelta prudente, non superiorità empirica |
| AUD-R1-05 | LOW | La ricerca suggerisce aggiornamenti normativi troppo ampi prima di conoscere costi e target | Limitare le proposte a renderer e protezione; non riaprire M1/M2.3 |

**Verdetto sul rapporto:** **OK come base di decisione architetturale**, con queste riserve documentate. **Non** è prova della superiorità timbrica del renderer B, né autorizzazione automatica a un nuovo limiter o dipendenza.

## 8. Fonti primarie consultabili

- [ITU-T H.872, Safe listening for video gameplay and esports (2024), in vigore](https://www.itu.int/rec/T-REC-H.872-202410-I/en), §9.1–9.4 (in particolare §9.3.1).
- [WHO/ITU, Global standard for safe listening video gameplay and esports (2025)](https://www.who.int/publications/i/item/9789240108028).
- [ITU-R BS.1770-5 (2023), loudness e true-peak](https://www.itu.int/rec/R-REC-BS.1770-5-202311-I).
- [EBU Tech 3341, EBU Mode metering (2023)](https://tech.ebu.ch/publications/tech3341); [EBU Loudness](https://tech.ebu.ch/loudness/).
- [CLAP `events.h` (transport e note/param automation)](https://github.com/free-audio/clap/blob/main/include/clap/events.h).
- [VST3 ProcessContext (SDK)](https://steinbergmedia.github.io/vst3_doc/vstinterfaces/structSteinberg_1_1Vst_1_1ProcessContext.html).
- [Steinberg VST3 SDK, licenza MIT](https://github.com/steinbergmedia/vst3sdk).
- [Airwindows, sorgenti MIT](https://github.com/airwindows/airwindows/blob/master/LICENSE).
- [DaisySP releases: separazione MIT/LGPL](https://github.com/daisyaudio/DaisySP/releases), [DaisySP-LGPL](https://github.com/daisyaudio/DaisySP-LGPL).
- [Surge XT, repository e licenza GPL](https://github.com/surge-synthesizer/surge).

**Nota sulle fonti:** i repository upstream citati come `main`/`master` sono dinamici; pin del commit e riesame della licenza del modulo specifico sono richiesti **prima** di incorporare codice. Nessun benchmark reale di CPU/qualità CYMATICA contro Surge è stato eseguito per questo documento.
