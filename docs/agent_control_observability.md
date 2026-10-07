# CYMATICA — Analisi del controllo agentico e dell'osservazione audiovisiva

**Data:** 2026-10-07. **Stato:** analisi e piano incrementale richiesti dal titolare; nessuna funzionalità implementata da questa revisione.

La [specifica tecnica](../CYMATICA_Specifica_Agentica_Sviluppo.md) §31.5 definisce gli invarianti architetturali e §32 assegna i gate alla roadmap. [DESIGN](../DESIGN.md) §17.4.1 definisce i vincoli percettivi. Questo documento approfondisce motivazioni, alternative, esperimenti e verifiche; non è uno schema eseguibile né una seconda fonte dei contratti. I profili numerici sotto sono **proposed**, da congelare prima delle rispettive implementazioni.

## 1. Valutazione della proposta e baseline

La proposta è adatta a CYMATICA come infrastruttura permanente di sviluppo: combina controllo attraverso ingressi reali, stato autoritativo e osservazione di immagini/audio. Permette di distinguere errore del modello, errore del percorso input e problema di percezione. Il beneficio cresce insieme al gioco; non richiede di introdurre subito un server MCP, un modello nel runtime o un editor.

La baseline ispezionata è concreta: `apps/cymatica_game/main.cpp` legge `IsKeyDown(KEY_SPACE)` una volta per ciclo di rendering e pubblica un `AudioControlFrame`: 220 Hz a riposo, 440 Hz durante la pressione, ampiezza 0.08. `engine/audio/audio_engine.cpp` applica il controllo nella callback, genera PCM e pubblica telemetria. Il profilo M1 è 48 kHz logici / 120 Hz simulazione; il polling spazio attuale è però legato al loop render, non un comando già schedulato per tick. La telemetria `pulse.pitchHz` è attualmente impostata a 220 Hz: non è un analizzatore del suono prodotto e non prova il cambio di tono.

[progress.md](progress.md) registra M1 conclusa e M2 attiva. La proposta dice di iniziare in M2; il requisito del titolare anticipa il primo caso alla baseline M1. Si pianifica pertanto una **estensione della baseline M1 all'ingresso di M2**, senza riscrivere i PASS storici o attribuire alla vecchia build capacità assenti. Il README viene riallineato al record di avanzamento, senza rivalidare in questo task le evidenze riportate.

Correzioni necessarie alla proposta:

- API semantica e input del sistema operativo sono prove diverse. Condividere la normalizzazione delle azioni non dimostra funzionamento del focus, della tastiera o del gamepad.
- Il tap master prova il PCM sintetizzato, non ciò che è uscito da casse/cuffie. Driver, buffer, mixer di sistema e device richiedono un altro livello di verifica.
- Uno screenshot non prova durata del telegraph, traiettoria, ordine degli eventi o reazione. Occorrono sequenze con timestamp, audio e input correlati.
- Uno stream ricevuto dall'agente non garantisce ragionamento continuo o riflessi compatibili con un bullet hell. Anche il client deve supportare video/audio e la relativa frequenza di consumo.
- Metriche e giudizi multimodali sono segnali con limiti: non sostituiscono validator, benchmark hardware o playtest umano.

## 2. Quattro usi separati

| Uso | Controllo | Osservazioni | Domanda verificabile |
|---|---|---|---|
| L0 semantico | Azioni normalizzate, tick/step headless | Stato, eventi, trace | La regola e la collisione sono corrette? |
| L1 percettivo | Azioni normalizzate registrate | Vista player, PCM, sequenza; debug per diagnosi | Il feedback è presente e leggibile nel tempo? |
| L2 integrazione input | Input OS/device sul gioco in esecuzione | Vista player e audio; stato usato dall'oracolo separato | L'ingresso reale produce l'effetto previsto? |
| Osservazione del giocatore | Nessun input agente | Sessione live player/debug e clip correlate | Che cosa è accaduto prima e dopo l'errore umano? |

In L2 il controller non riceve stato privilegiato se la prova dichiara percezione esclusivamente player. L'oracolo può verificare l'esito sullo stato a posteriori. Ogni run dichiara le informazioni realmente disponibili al controller: usare safe routes o hazard ID cambia il significato del risultato.

L'agente di sviluppo esterno resta separato dal CIE. Un controller locale diagnostico o una persona del Lab non modifica il director e non acquisisce poteri di sicurezza. La sopravvivenza di un bot non dimostra fairness umana; il suo fallimento non dimostra impossibilità.

## 3. Architettura e confini

```text
Agente / runner / CLI
        | richieste bounded e artefatti
Adapter opzionale MCP -- IPC locale -- gateway diagnostico
                                           |
                                  coordinatore del gioco
                                           |
Input umano o agente -> normalizzazione -> azioni -> simulazione/scheduler
                                           |
                       snapshot owned / eventi / cattura frame
                                           |
                                  worker di esportazione

Audio callback -> tap PCM preallocato SPSC -> worker audio -> WAV/metriche
```

Il nome provvisorio è **CYMATICA Agent Control Plane**, protocollo **CACP**. La semantica deve essere indipendente dal trasporto. Per il primo test è preferibile un runner locale con richieste finite e output su file; l'alternativa named pipe consente comandi durante la sessione, ma aggiunge lifecycle, autenticazione e cancellazione. La scelta dell'IPC resta aperta all'ingresso M2. MCP è un adapter esterno successivo, non una dipendenza di `core`, `audio` o `validation`.

Il gateway accetta richieste fuori dai percorsi realtime; il coordinatore è l'unico owner dell'applicazione delle azioni e l'unico producer degli eventi audio. La cattura sul renderer avviene sul thread che possiede il contesto grafico: il worker riceve slot immutabili, non chiama arbitrariamente raylib o legge buffer live. Parsing, codifica, scrittura e analisi audio sono nei worker/tool.

Non si esportano scritture dirette a posizione, salute, RNG, cooldown o piani accettati. Reset e caricamento replay sono operazioni di sessione diagnostica con lifecycle esplicito, non azioni player. Fixture artificiali del validator restano fixture, etichettate come tali.

## 4. Controllo temporale e percorso input

Tre modalità distinte: **offline deterministica**, **live controllata dall'agente**, **live osservata con giocatore umano**. `step` e `run_until` sono solo offline; non arrestano una callback con device attivo né simulano che l'audio già udito possa essere riavvolto. Un replay diagnostico parte in una sessione separata.

L'LLM invia batch temporali finiti. Il controller locale li esegue o applica una policy bounded di durata esplicita. Una micro-policy «verso la safe area» è privilegiata e va etichettata; non è introdotta in M1/M2 e non aggira il safety guard. Per comportamento reattivo in M4/M5 il controller deve dichiarare ritardo osservativo, frequenza, rumore e azioni consentite, altrimenti è un oracolo ideale.

Ogni richiesta porta versione, sessione/epoch, ID idempotente, modalità e tick target. Gli ingressi mantengono press/hold/release, anche per azioni continue. Il coordinatore controlla range, valori finiti, ordine, limiti del batch, tick passati e capacità; l'ack distingue ricezione, accettazione e applicazione effettiva. Gli errori includono almeno `unsupported`, `wrong_mode`, `stale_epoch`, `late`, `queue_full`, `invalid_payload`, `cancelled`. Un retry con stesso ID non ripete un dash. Per il primo profilo i comandi in ritardo sono rifiutati: nessuna retrodatazione o applicazione immediata nascosta.

D-AO-01 congela in M2 il nucleo comune `InputAction`/`ActionState` per press/hold/release del tono: SPACE fisico e runner passano dalla stessa normalizzazione e dalla stessa applicazione sul coordinatore, che pubblica il controllo audio. Nessun percorso agent-only o scrittura diretta del runner a `AudioControlFrame`. M2 definisce sorgente esclusiva, ordine e tempi richiesti/effettivi del test finito; D-AO-03 estende questo nucleo in M3 a movimento/dash, scheduling per tick e ordinamento multi-source. Le regole complete dei batch per tick descritte sopra appartengono a questa estensione M3; M2 non le dichiara già implementate. Nella sessione umana l'agente è read-only per default; il controllo esclusivo richiede un cambio esplicito di modalità visibile al giocatore. Disconnessione, timeout, perdita focus, pausa, reset e shutdown rilasciano gli input agente; i batch della vecchia epoch sono invalidati. Non si mescolano due sorgenti silenziosamente.

Un comando semantico `tone_test.hold` potrà condividere la stessa azione normalizzata della tastiera, ma solo un test OS spazio prova il percorso `IsKeyDown`. Lo smoke legacy va preservato fino alla verifica di equivalenza del nuovo dispatcher. Lo spazio del tono è un comando diagnostico della baseline, non la definizione futura del dash.

## 5. Osservazioni e correlazione dei clock

Gli artefatti correlano `run_id`, build/commit, schema e profili, seed/policy/RNG, origine input, modalità player/debug, sequence, epoch e intervalli tick/sample-frame. Lo snapshot descrive il tick realmente osservato, non quello richiesto. Ogni canale ha sequence propria e segnala buchi: latest-value telemetry non equivale a event log lossless.

Per il tempo distinguere:

- tick autoritativo e sample frame logico del transport;
- indice PCM al sample rate effettivo del device e conversione razionale;
- tempo monotono di input, cattura, invio/ricezione e analisi;
- render frame, tick interpolati e alpha;
- cursore/qualità della stima di presentazione audio e incertezza.

Un reset non rende confrontabili timestamp di epoch diverse. Non sottrarre clock non sincronizzati; sull'esterno si misura round-trip oppure si documenta la sincronizzazione e il suo errore. Durante pausa il transport può restare fermo mentre il device produce silenzio: conservare anche la timeline monotona/device della cattura. Una coppia frame+audio deve dichiarare skew e intervalli coperti; non promettere snapshot globale atomico di thread indipendenti.

## 6. AudioProbe: ascolto e misura

Il tap opzionale si colloca sul PCM finale consegnato dal motore al device, dopo mix/gain del gioco, prima della presentazione hardware. In M1 estesa/M2 registra il tono; in M2 evolve al mix procedurale. La callback copia in slot preallocati con singolo producer/consumer, bounded rispetto al massimo blocco supportato. Non alloca, non scrive file, non fa FFT, codifica, IPC, log o attesa. Overflow o blocco oltre capacity produce contatore e gap; mai overwrite dello slot posseduto dal consumer, mai producer che avanza l'indice del reader.

La storia «ultimi N secondi» è costruita nel worker, non tramite una seconda lettura concorrente della ring SPSC. Avvio/arresto del tap e lifetime dei buffer richiedono handshake fuori callback; liberare memoria soltanto dopo quiescenza. Saturazione della cattura degrada l'evidenza, non blocca il device.

WAV PCM è il primo formato: registra sample rate, canali, formato, frame iniziale/finale e segmenti mancanti. Frequenza stimata, RMS, sample peak, DC e conteggio clipping sono sufficienti per il tono. In M2 si aggiungono onset e fase rispetto al Music Intent. LUFS richiede algoritmo/finestra/gating definiti; true peak non coincide con sample peak. Una discontinuità può essere un attacco intenzionale: il detector di crackle va validato con fixture e ascolto, non usato come prova assoluta.

Per driver/device utilizzare, quando autorizzato e disponibile, loopback di sistema o misura esterna separata dal tap. Il loopback include il percorso software ma non prova da solo l'uscita acustica. Ascolto umano e misure hardware restano evidenze distinte. Le capacità del client si negoziano: se non può ingerire audio, il risultato è «misurato quantitativamente, ascolto agente non eseguito».

## 7. VisualProbe, sequenze e streaming

`player` conserva l'immagine del giocatore e il profilo di accessibilità attivo; `debug` aggiunge ID, forme autoritative e clock. La cattura debug non forza overlay sulla finestra del giocatore. Quando entrambe sono prodotte, condividono identità di render frame e dati di interpolazione; una composizione separata va misurata perché può costare un pass aggiuntivo. La futura mask `semantic` resta differita finché non serve a una diagnosi concreta.

Evoluzione necessaria:

1. **M1 estesa/M2:** clip WAV del cambio tono; eventuale frame di contesto come cattura statica diagnostica opzionale, fuori dal gate del tono e non ancora contratto VisualProbe.
2. **M3:** sequenza ordinata di frame player/debug, manifest dei tempi e audio correlato; permette di esaminare telegraph → attivazione → attraversamento.
3. **M4:** buffer circolare nel worker e salvataggio di una finestra pre/post evento; selezione per hit, dash, fallback o intervento tecnico, con limiti di frequenza e quota disco.
4. **M5:** osservazione live read-only della sessione umana tramite flusso di frame timestampati; registrazione locale con qualità sufficiente per analizzare il timing. Non attendere M7 per il primo live.
5. **M6–M7:** spike su video/audio continuo a latenza ridotta e successiva valutazione. Codec, container, trasporto e client si scelgono solo dopo il benchmark; un esito negativo conserva sequenze/clip e documenta il limite.

«Live» e «registrazione completa» richiedono politiche diverse. Il live conserva il frame recente, scarta ritardo accumulato e mostra età/gap; la clip cerca continuità e segnala incompletezza. Se il receiver si ferma, il gioco continua. Preview ridotta non basta a giudicare una hitbox piccola: la clip diagnostica mantiene risoluzione e frequenza adeguate al fenomeno.

Una sequenza PNG+WAV evita subito una dipendenza codec, ma non è già uno stream video consumabile da ogni modello. A 1280×720 RGBA8, un frame raw occupa 3.686.400 byte; 60 frame/s sono 221.184.000 byte/s, prima di copie e overhead. Dieci secondi raw richiedono circa 2,21 GB decimali: non è un budget accettabile implicito. Riduzione preview, compressione worker, buffer bounded e readback GPU asincrono vanno valutati; leggere il framebuffer può introdurre stall anche senza file I/O.

Nessuna adozione automatica di FFmpeg, browser runtime, WebRTC o encoder. Una soluzione con API native Windows può essere studiata, ma non è già scelta né verificata. SDK/codecs nuovi richiedono inventario, pinning dove applicabile, licenze e decisione d'ingresso. La disponibilità del client a ricevere streaming è una verifica dipendente dalle capacità esterne, separata dal gate obbligatorio di produzione e misura del progetto (§10).

## 8. Latenza: che cosa misurare

Il percorso umano resta locale e non attraversa gateway, tool call o modello. A 120 Hz un tick vale circa 8,333 ms; il render nominale a 60 Hz vale circa 16,667 ms. Questi intervalli non sono la latenza totale né la sua garanzia.

| Segmento | Evidenza necessaria | Limite |
|---|---|---|
| Input OS → polling/normalizzazione | Timestamp ingresso osservato e consumo | Il polling non misura il contatto fisico del tasto |
| Azione → applicazione | Tick target/effettivo, ack | Un test schedulato nasconde la variabilità OS |
| Applicazione → immagine | Frame render e presentazione stimata/misurata | Cattura framebuffer non misura fotoni sul display |
| Applicazione → audio | Primo campione con effetto e presentation quality | Il tap non misura suono acustico |
| Cattura → agente | Enqueue, encoding, trasporto, decode, età osservazione | Clock esterni e coda client aggiungono incertezza |
| Agente → azione successiva | Durata inferenza/tool e ack | Nessun budget realtime affidato all'LLM |

In M5 fissare profilo hardware, buffer audio, vsync, risoluzione e scena; confrontare capture OFF/ON/consumer lento, tastiera/gamepad, carico nominale e scena densa. Riportare p50/p95/p99/massimo, numerosità, warmup, drop e suspension, oltre alla frequenza nominale. Soglie di input-to-effect e overhead si congelano **prima** della misura M5; in M7 si verificano sul contenuto finale. Misure input-to-photon/acoustic richiedono hardware/procedura dedicati, non si deducono da ack software.

## 9. Scenario minimo: spazio e cambio di tono

Primo task implementativo in M2: introdurre il nucleo minimo del dispatcher comune con `InputAction`/`ActionState` del tono (D-AO-01), poi collegare SPACE fisico e runner finito sulla baseline M1 alla stessa azione hold/release normalizzata, con tap PCM, manifest e report. In una prova separata, input OS sulla finestra focalizzata verifica il tasto spazio. Se il client non può controllare app native, questa parte resta manuale o usa un adapter locale esplicitamente implementato: non dichiararla automatizzata per il solo successo dell'API semantica.

Profilo sperimentale `tone_probe_v0` (**proposed**, da accettare all'ingresso): 1 s baseline, 1 s hold, 1 s release, 48 kHz logici, canali/rate device registrati; finestre di analisi stabili da 250 ms escludono i primi 100 ms di ogni transizione. Tolleranza iniziale di stima ±2 Hz attorno a 220/440/220 Hz. Non è una soglia di latenza input, né un tuning del gameplay. Una cattura di 3 s stereo float32 a 48 kHz pesa 1.152.000 byte; capacità effettiva della ring e massimo blocco si fissano dopo ispezione del device.

L'identificatore `AC-AO-01` designa due verifiche distinte:

- **`AC-AO-01A` — mandatory, sotto controllo del progetto:** dispatcher comune e runner; press/hold/release attraverso lo stesso ingresso normalizzato; cattura PCM/WAV e analisi quantitativa non tautologica 220 → 440 → 220 Hz nelle finestre previste, con segnale non silente; manifest con tick/epoch, tempi richiesti/effettivi e qualità di presentazione; gap/overflow dichiarati e RT test con tap attivo e consumer stall (zero allocazioni e lock bloccanti). Include la prova SPACE OS sulla finestra focalizzata, automatica oppure con procedura manuale ed evidenza registrata. La sola presenza del campo richiesto a 440 Hz non produce PASS. Un requisito mancante o fallito di 01A impedisce la chiusura M2 secondo AGENTS §5.1.
- **`AC-AO-01B` — capability-dependent, supplementare:** audio effettivamente consegnato al client agente e giudizio percettivo registrato. Esito `passed`, `failed`, `blocked` o `unsupported`; `unsupported` richiede evidenza della capacità assente, `blocked` descrive un impedimento a una capacità prevista. Nessun ascolto è dichiarato senza ingestione effettiva. L'assenza della capacità esterna non blocca la chiusura dell'implementazione M2 e non rende superata 01B. Un fallimento percettivo con possibile difetto del gioco genera un finding da verificare: non viene ignorato perché il criterio è supplementare.

## 10. Roadmap delle verifiche

Questi ID identificano le estensioni in specifica §32; i dettagli dei profili si congelano nei decision record all'ingresso di ogni milestone. I gate del progetto restano obbligatori: produrre/correlare artefatti, verificare budget e contratti e registrare il risultato. La consegna audiovisiva al modello e il suo giudizio sono verifiche supplementari dipendenti dal client, da riportare separatamente anche in M3–M8. In M6/M7 il report dello spike e il benchmark locale sono obbligatori; ricezione/analisi video live da parte del client possono risultare `unsupported`/`blocked` senza impedire la chiusura della milestone. Nessun criterio originale di ascolto umano, accessibilità, hardware o playtest viene reso facoltativo; una capacità mancante nel progetto non si riclassifica come limite del client.

| Milestone | Incremento | Accettazione dell'estensione |
|---|---|---|
| Baseline M1, task in M2 | Tono controllabile e PCM osservabile | `AC-AO-01A` mandatory (dispatcher comune, PCM, RT, manifest e prova OS anche manuale); `AC-AO-01B` supplementare dipendente dal client; nessuna modifica ai PASS storici |
| M2 | Music Intent, AudioProbe e primo contratto versionato | `AC-AO-02`: capture bounded, overflow/lifecycle testati; onset correlati all'intent; adapter dichiara capacità e limiti |
| M3 | Input scheduling del nucleo player e VisualProbe sequenziale | `AC-AO-03`: stesso percorso normalizzato del player, press/release ai confini, sequenza telegraph/hazard con player/debug e audio, gap/skew dichiarati |
| M4 | Step/headless, scenario/replay e clip evento | `AC-AO-04`: stessi hash per input/decisioni accettati; late/epoch/capacity rifiutati, fixture fairness resta autoritativa; pre/post evento e capture OFF/ON confrontate |
| M5 | Osservazione live umana e test input end-to-end | `AC-AO-05`: agente read-only, disconnessione innocua, controlli comparati, latenza/overhead entro soglie prefissate; regressioni validator per cambi player |
| M6 | Explanation trace e spike video/audio continuo | `AC-AO-06`: scelta CIE e interventi correlabili; exact replay completo; report spike con età/gap/costo/client, senza promettere riflessi LLM |
| M7 | Corpus percettivo e gate qualità del video | `AC-AO-07`: player view, sequenze dense, reduced flashes, audio e sincronismo; benchmark streaming con esito esplicito, playtest umano e gate originali preservati |
| M8 | Lab e personas riusano contratti/scenari | `AC-AO-08`: risultati indicizzati/versionati e corpus senza leakage; distingue controller privilegiati, percettivi e umani |
| M9–M11 | Pacchetti/custom audio | Riutilizzo delle sonde per caricamento, confidence, mix/stem; nessuna dipendenza tool nel game |
| M12 | Portabilità | Nuovo profilo input/capture/device misurato su Android, senza assumere 120 Hz o codec Windows |
| M13 | Studio | UI del controllo solo se approvata; WebMCP eventualmente adapter dell'editor |

MCP viene valutato dopo il runner locale M2, con obiettivo M3/M4 se il client lo supporta. Il gate riguarda il contratto e un adapter utilizzabile, non impone un vendor o una GUI. `.cymtest` è un nome proposto per scenari da congelare in M4; prima bastano fixture finite versionate. Non duplicare `RunRecord`/decision trace: lo scenario li referenzia e il manifest lega hash e versioni.

## 11. Evidenze, failure e riservatezza

Ogni run produce un manifest con configurazione, input richiesti/accettati/applicati, eventi, riferimenti a `RunRecord` e trace, artefatti audio/frame, metriche con algoritmo/versione e giudizio agente separato. I risultati dei gate del progetto sono `passed`, `failed`, `not_run`, `blocked` o `inconclusive`; le verifiche supplementari del client distinguono anche `unsupported` documentato; catture incomplete non diventano PASS percettivi. Un timeout non equivale a successo, né un missing frame a telegraph assente.

Gli artefatti diagnostici non entrano nell'hash gameplay; la cattura può comunque alterare scheduling/deadline reali. Il confronto OFF/ON è necessario: ogni intervento o deadline miss continua a influire sull'idoneità Pure Seed secondo la specifica. Il replay exact registra azioni effettive e decisioni accettate; seed da solo non riproduce una run adattiva. Non promettere PCM o pixel bit-identici fra driver/GPU diversi.

Sessione osservata opt-in, indicatore visibile e stop immediato; cattura confinata al gioco e al suo audio, senza microfono o desktop impliciti. Artefatti locali per default, quota/durata e cancellazione definite dal profilo. Invio a modelli esterni distinto dalla semplice registrazione locale. Il gateway è disabilitato nella distribuzione normale, accessibile localmente con permessi di sessione; niente shell, script arbitrari o scritture fuori dalla directory artefatti. Il singolo controllo esclusivo evita anche ingressi conflittuali da più client.

## 12. Decisioni da congelare e rischi

| Decisione futura | Scadenza | Contratti / verifica |
|---|---|---|
| D-AO-01: nucleo input comune, runner/IPC, versione e discovery | Prima di AC-AO-01A in M2 | `InputAction`/`ActionState` tono, normalizzazione condivisa SPACE/runner, press/hold/release, ordine/sorgente esclusiva e tempi del test; alternative file runner/named pipe, schema/limiti, idempotenza, timeout/sessione; capability test separato dal gate progetto |
| D-AO-02: tap e `tone_probe_v0` | Prima del primo task M2 | Capacità in frame/byte, massimo blocco, overflow, lifecycle, rate/channels; RT test e prova tono |
| D-AO-03: estensione dispatcher e visual capture | Prima del nucleo M3 | Estendere il nucleo M2 a movement/dash, scheduling per tick e ordine multi-source; budget readback, FPS/risoluzione; equivalenza input e sequenze |
| D-AO-04: scenario e retention clip | M4 ingresso | Schema `.cymtest` o alternativa, pre/post in s, quota in byte, gap; replay/overflow |
| D-AO-05: latenza/live preview | M5 ingresso | Soglie in ms, preview FPS/byte/s, qualità clip e hardware; OFF/ON/stall, tastiera/gamepad |
| D-AO-06: video continuo | Spike M6, valutazione M7 | Codec/trasporto/client/licenze, p95 età/skew/overhead; alternative sequenze, API native, codec esterno approvato |

Queste scelte sono **deferred**, i valori del tono sono **proposed**. Ogni decision record deve riportare opzione, alternative, motivazione, contratti, parametri con unità, criteri ed esito; solo i principi integrati in specifica sono **accepted for implementation**, nessun parametro nuovo è **validated**. Non si blocca la musica M2 su codec M6 o design M5.

Il rischio principale è costruire un'infrastruttura più grande del gioco: ogni incremento serve un caso verificabile della milestone corrente. Seguono costo di readback/codifica, leakage di informazioni privilegiate, falsa precisione temporale, disponibilità del client e conservazione eccessiva di dati. La mitigazione è una baseline finite-run, capacità negoziate, budget espliciti e risultati onesti su prove non eseguibili.

## 13. Riferimenti verificati

Consultati il 2026-10-07, come riferimenti e non dipendenze:

- [MCP — Tools](https://modelcontextprotocol.io/specification/2025-11-25/server/tools): strumenti con schemi e risultati strutturati; supporta contenuti immagini/audio, non garantisce consumo video live o latenze del modello. L'adapter deve verificare le capacità del client.
- [Chrome — WebMCP](https://developer.chrome.com/docs/ai/webmcp): standard web proposto, API JavaScript/form HTML. Conferma la pertinenza a un eventuale editor web, non impone un browser al gioco nativo.
- [GameWorld — sito del progetto](https://gameworld-project.github.io/): riferimento per benchmark di agenti di gioco; non prova prestazioni o fairness di CYMATICA.
- [miniaudio — manuale](https://miniaud.io/docs/manual/): confini della callback/device. Il tap proposto resta un componente da implementare e verificare sul commit pinned del progetto.

I riferimenti secondari Agent Surface e Doom Agent Arena della proposta non sono assunti come fondamento normativo. L'architettura qui descritta è una valutazione progettuale per CYMATICA, non una proprietà dimostrata dai benchmark citati.
