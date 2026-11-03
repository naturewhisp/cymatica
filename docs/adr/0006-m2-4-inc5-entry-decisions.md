# ADR-0006 — Decisioni d'ingresso M2.4 Inc-5 (catena degli eventi, stem, descrittori, pacchetto di valutazione, calibrazione)

- **Data:** 2026-10-26
- **Stato:** `accepted for implementation` (decisioni d'ingresso di Inc-5), **in review del titolare** come fase 5.0. Nessun valore qui riportato è `validated`: i parametri sono profili iniziali versionati, da calibrare e verificare durante le fasi 5a–5e e al gate M2.5.
- **Milestone:** M2 — sottofase M2.4, incremento Inc-5.
- **Owner:** coordinatore dello sviluppo M2.4, nell'ambito autorizzato dal titolare il 2026-10-26 («procediamo con Inc-5»; piano «approvabile con correzioni preliminari circoscritte a 5.0»). La ratifica esplicita del titolare è richiesta prima della chiusura di M2.4, insieme a quella di [ADR-0005](0005-m2-4-inc4-entry-decisions.md).
- **Gerarchia:** Specifica > DESIGN > AGENTS > ADR > piano operativo > ricerca. Questo ADR **attua** le raccomandazioni R2–R6 approvate il 2026-10-21 ([dossier sui metodi CACP](../research/M2_4_CACP_Evaluation_Methods.md) §6) e **precisa** D-M2-11 di ADR-0005. Non modifica:
  - le ABI pubbliche `MusicIntentEvent`, `ScheduledAudioEvent` e `AudioCommand`;
  - il budget di 8 voci;
  - il golden `canonicalPlanHash` v1;
  - la roadmap §32 e i gate M2.5;
  - la definizione normativa di AC-AO-02 (agent_control_observability §10).
- **Perimetro:** sono fissati solo i contratti di cui hanno bisogno le fasi 5a–5e. I dettagli implementativi (nomi, layout interni, scelte locali reversibili) si approvano nella review di ciascuna fase (regola di avanzamento in [progress.md](../progress.md#m24-inc-5--piano-a-fasi-e-revisione-del-piano-2026-10-26)).
- **Ricerca a supporto:** [M2_4_Inc5_Entry_Research.md](../research/M2_4_Inc5_Entry_Research.md) (2026-10-26).
- **Precisazioni della review di 5.0 (2026-10-26, verdetto NON OK):** DIF-5.0-01 cardinalità della catena O3 (D-M2-12); DIF-5.0-02 rilasci nel fingerprint (D-M2-12); DIF-5.0-03 modalità `blind` e `diagnostic` (D-M2-15); DIF-5.0-04 affermazioni tipizzate (D-M2-15); DIF-5.0-05 esiti della misura di diversità (D-M2-16). Review mirata (verdetto NON OK): DIF-5.0-R2-01 eventi cancellati prima dell'invio (D-M2-12); DIF-5.0-R2-02 presenza del segnale, attacco e udibilità (D-M2-15); codifica del fingerprint (D-M2-12). Terza review (verdetto NON OK): DIF-5.0-R3-01 NoteOff conservati dalla revoca (D-M2-12); DIF-5.0-R3-02 percorso nominale e percorso con guasto (D-M2-12); assenza dimostrata e soglia di rilevazione (D-M2-15). Registro in [progress.md](../progress.md#revisione-della-fase-50-dif-50-2026-10-26).

## 1. Contesto

Inc-4 ha consegnato fixture, pianificazione just-in-time, profiling e scenari `fixture_probe_v1`/`profile_probe_v1` (CI verde su `b0f466f`). Inc-5 deve:

- rendere AC-AO-02 misurabile per ruolo;
- rendere verificabili in automatico i giudizi degli agenti;
- produrre lo scenario full-length e il dossier per la review M2.5.

Il piano è registrato in [progress.md](../progress.md), piano operativo, riga Inc-5. Gli input sono:

- R2–R6 del dossier sui metodi CACP;
- la precisazione residua della revisione R3, cioè la verifica `hash(piano offline) == hash(piano JIT) == golden`;
- il profilo esplorativo `AUD-METER-EXPLORATORY` del [protocollo di valutazione](../research/M2_4_Evaluation_Protocol.md);
- la nota di ADR-0005 sulla valutazione multi-seed.

La revisione del piano da parte del titolare (2026-10-26) ha prodotto cinque rilievi, verificati sul codice prima di questo ADR:

- **INC5-REV-01:** l'hash di provenienza include il seed e non misura la diversità musicale.
- **INC5-REV-02:** «evento applicato» non è definito.
- **INC5-REV-03:** la ground truth era esposta al valutatore.
- **INC5-REV-04:** gli stem mono perdono lo stereo e il dominio di ricostruzione non è definito.
- **INC5-REV-05:** il test LUFS non era specificato.

Il registro completo è nel [progress](../progress.md#revisione-del-piano-inc-5-inc5-rev-2026-10-26).

## 2. Decisioni

### D-M2-12 — Identità del piano e catena degli eventi applicati (O3)

*Rilievi: INC5-REV-01, INC5-REV-02. Fase: 5a.*

**Due identità distinte del piano.**

1. **Hash canonico di provenienza:** `canonicalPlanHash`, formato v1, [plan_canonical_hash.h](../../engine/audio/plan_canonical_hash.h). Invariato.
   - *Contenuto:* serializza eventi, NoteOff del planner e trace, compresi seed, policy, candidati, punteggi e tie-break.
   - *Uso:* golden, replay e l'invariante `hash(offline) == hash(JIT) == golden`.
   - *Hash JIT:* `FixtureCoordinator::planNext` conserva, per frase, gli intent che emette (oggi il vettore `out` viene scartato). L'hash JIT si calcola con la stessa funzione sugli intent JIT, sui NoteOff e sulle trace registrati durante la riproduzione.
2. **Fingerprint del contenuto musicale:** `musicalContentFingerprint`, formato v1. Nuovo.
   - *Contenuto:* solo ciò che determina la musica, rappresentato come **note complete**, non come eventi separati (precisazione DIF-5.0-02 del 2026-10-26).
     - Ogni NoteOn produce un **record di nota** con: ruolo, `sampleFrame` d'attacco, altezza, velocity, `timbreParam`, `durationFrames` e il **rilascio esplicito** associato.
     - Il rilascio esplicito è il frame del NoteOff del planner che porta lo stesso `eventId` (oggi solo per i pad aperti con `durationFrames == 0`), oppure «nessuno». Se una nota ha più NoteOff espliciti, conta la lista ordinata dei loro frame.
     - Il rilascio derivato da `durationFrames > 0` resta nel campo `durationFrames`; il rilascio esplicito ha un campo distinto. Una nota con entrambi conserva entrambe le informazioni.
     - L'`eventId` serve solo come chiave di unione fra NoteOn e NoteOff nel momento della costruzione. Non entra nella serializzazione.
     - Un NoteOff esplicito senza NoteOn corrispondente nel piano produce un record separato di **rilascio orfano** (ruolo, frame).
     - Così lo scambio dei rilasci fra due note diverse (es. Corpo a 220 e 330 Hz rilasciate a frame diversi) cambia i record e quindi il fingerprint. Lo scambio fra due note identiche non lo cambia, perché le due musiche sono identiche.
   - *Quantizzazione* (prima della serializzazione, per non far dipendere l'identità da differenze float non udibili):
     - altezza → intero in centesimi di semitono relativi a 440 Hz, cioè `round(1200·log2(f/440))`;
     - velocity e `timbreParam` → `round(x·1000)`.
   - *Ordine canonico:* ordinamento lessicografico totale dei record di nota su (`sampleFrame`, ruolo, altezza quantizzata, velocity quantizzata, timbro quantizzato, `durationFrames`, rilasci espliciti), seguito dai rilasci orfani ordinati per (frame, ruolo). È indipendente dall'ordine di emissione e dagli `eventId`. Due note identiche in tutti i campi sono indistinguibili anche musicalmente, quindi il loro ordine relativo non conta.
   - *Esclusioni:* seed, `eventId`, `tick`, epoch, `decisionId`, candidati, punteggi, tie-break e ogni altro metadato che può variare senza cambiare la musica.
   - *Valori speciali:* un'altezza 0 o non applicabile usa una sentinella esplicita; lo zero negativo vale 0; un valore non finito è un errore e non produce fingerprint (come nella canonicalizzazione di RFC 8785). Lo stesso vale per un valore finito il cui arrotondamento non è rappresentabile nel campo intero (DIF-5a-03): non si converte mai fuori dominio.
   - *Serializzazione* (codifica deterministica non ambigua, precisata dopo la review del 2026-10-26). Tutti gli interi sono little-endian a larghezza fissa e ogni lista è preceduta dalla propria lunghezza:
     1. intestazione: versione del formato (`u32`), sample rate (`u32`), numero di note (`u64`), numero di rilasci orfani (`u64`);
     2. per ogni nota, in ordine canonico: ruolo (`u8`), `sampleFrame` (`u64`), altezza (`i32`), velocity (`i32`), timbro (`i32`), `durationFrames` (`u32`), numero di rilasci espliciti (`u32`), poi i frame dei rilasci espliciti in ordine crescente (`u64` ciascuno);
     3. per ogni rilascio orfano, in ordine canonico: ruolo (`u8`), frame (`u64`).

     La lunghezza esplicita delle liste rende la codifica iniettiva. L'hash è FNV-1a a 64 bit, come `canonicalPlanHash`. I numeri di note e di rilasci si confrontano insieme all'hash. Con le poche decine di seed previste la probabilità di collisione è trascurabile (dossier di ricerca §2.3).
   - *Varianti:* fingerprint per ruolo e per frase, usati dalle metriche di diversità.
   - *Uso:* rilevare duplicati e misurare la diversità (D-M2-16). È l'identità con cui 5c sceglie i seed di verifica.

**Registro degli eventi consumati dal motore (applied-event log).**

| Disposizione | Condizione | Punto nel codice (alla data) |
|---|---|---|
| `Applied` | evento consegnato al `VoiceAllocator`, che lo accetta: `onNoteOn`/`onNoteOff` restituisce uno slot ≥ 0 | `audio_engine.cpp`, `switch` sul tipo dopo la validazione |
| `Ignored` | evento consegnato ma rifiutato dal `VoiceAllocator` (slot −1, es. NoteOff senza nota corrispondente dopo uno steal) | idem |
| `Dropped` / `StaleFrameOrEpoch` | `sampleFrame` < inizio blocco oppure epoch < `minValidEpoch` | i due siti di validazione del ciclo eventi |
| `Dropped` / `FutureEpochUnmatchable` | epoch futura con frame già raggiunto e nessun cambio tempo pendente prima del frame | idem |
| `Dropped` / `StopDrain` | evento scartato dal drain bounded su `Stop` o a trasporto fermo | i due drain di Stop |

**Contenuto di ogni record:** `eventId`, ruolo, tipo, `sampleFrame` pianificato, frame logico in cui il motore ha consumato l'evento, epoch, disposizione, ragione, slot. Gli eventi `ParameterChange` del tono diagnostico sono registrati con il loro tipo e restano fuori dal controllo di catena musicale.

**Vincoli realtime e di ciclo di vita** (agent_control_observability §6; AGENTS §3):

- sink opzionale tramite puntatore, nullo per default e quindi nel percorso realtime del gioco;
- array preallocato dal chiamante, con capacità dichiarata. Profilo iniziale: 8.192 record, più del doppio dei 2.784 eventi consumati dalla fixture (1.392 NoteOn e 1.392 NoteOff, `engine_processed_events` di `fixture_test`). La dimensione esatta del record si registra in 5a;
- nella callback solo scritture bounded di un record. Niente allocazioni, lock, I/O o analisi;
- overflow contato. Un overflow > 0 rende il verdetto della catena FAIL: un registro incompleto non dimostra nulla;
- installazione, rimozione e lettura solo in quiescenza (motore offline, oppure device fermo e callback non in esecuzione), con un'asserzione di contratto. È lo stesso vincolo proposto per DIF-INC4-R3-05;
- dopo 5a, misura `perf` con la metodologia D-M2-10 (W e F, sink nullo): p99.9 ≤ 2,0 ms invariato rispetto alla baseline Inc-4.

**Controllo esatto di catena (O3).** Esito PASS/FAIL, vincolante per lo scenario:

*Cardinalità distinte* (precisazioni DIF-5.0-01 e DIF-5.0-R2-01 del 2026-10-26). Intent ed eventi audio non sono la stessa unità, e un evento cancellato prima dell'invio non arriva mai al motore.

| Simbolo | Grandezza | Origine |
|---|---|---|
| `I` | intent musicali generati dal Coordinator JIT | intent registrati in `planNext` |
| `E_exp` | eventi audio **attesi**, derivati dal piano: un NoteOn per intent, un NoteOff derivato per ogni intent con `durationFrames > 0`, un NoteOff per ogni rilascio esplicito del planner | funzione pura del piano JIT |
| `E_canceled` | eventi attesi rimossi dallo staging **prima** dell'invio alla SPSC | registro delle cancellazioni dello scheduler (sotto) |
| `E_sent` | eventi effettivamente inviati alla SPSC dallo scheduler | registro degli invii dello scheduler (sotto) |
| `E_cons` | record consumati dal motore (`Applied` + `Ignored` + `Dropped`) | applied-event log |

**Partizione per identità.** Ogni evento è identificato da (tipo, `eventId`, ruolo, `sampleFrame`, epoch). Valgono:

- `E_exp = E_sent ⊎ E_canceled`: ogni evento atteso è stato inviato oppure cancellato, mai entrambe le cose e mai nessuna delle due;
- dopo il drain completo della SPSC (coda vuota verificata in quiescenza), `E_sent = E_cons` per identità.

Le uguaglianze dei conteggi sono solo una conseguenza; non sostituiscono il confronto per identità.

**Registro delle cancellazioni.** Lo scheduler gira nel thread del Coordinator, non nella callback. Registra ogni evento che rimuove dallo staging, con identità e ragione:

| Ragione | Operazione nel codice (alla data) |
|---|---|
| `Revoked` | `AudioScheduler::revokeFromFrame`: NoteOn con frame ≥ confine, NoteOff delle note revocate, eventi non di nota oltre il confine |
| `ClearedAtStop` | `AudioScheduler::clearAtStop(stopFrame)` sullo `Stop` applicato del Coordinator |
| `Cleared` | `AudioScheduler::clear()` senza contesto di `Stop`: registrato, ma mai ammesso dalla riconciliazione |

**Validazione causale delle cancellazioni** (DIF-5a-01, review di 5a del 2026-10-26). Ogni record di cancellazione porta il proprio contesto (`contextFrame`): il confine della revoca, oppure il frame dello `Stop` applicato. Il Coordinator dichiara alla riconciliazione il confine della transizione fallita e il frame dello `Stop` che ha svuotato lo staging. Una ragione diversa da `None` non basta: la cancellazione deve essere causalmente possibile.

- **`Revoked`:** contesto uguale al confine dichiarato; frame dell'evento non anteriore al confine; un NoteOff solo insieme alla revoca del proprio NoteOn. `revokeFromFrame` conserva i NoteOff delle note già inviate: revocarne uno è un errore.
- **`ClearedAtStop`:** contesto uguale al frame dello `Stop` dichiarato, e frame dell'evento non anteriore allo `Stop`. Un evento già dovuto prima dello `Stop` doveva essere inviato, non cancellato.
- **`None`, `Cleared` o ragioni sconosciute:** FAIL. Lo stesso vale per una cancellazione senza contesto dichiarato.

Il sink è opzionale, come per gli eventi applicati, e l'overflow rende la catena FAIL. Lo stesso registro annota gli invii riusciti, quindi `E_sent` non dipende dal solo contatore `totalDispatched`.

Il contatore `revokedEvents` già esistente resta come diagnostica: da solo non basta per un controllo d'identità. Un tentativo respinto per backpressure non è una cancellazione, perché l'evento resta in staging e viene ritentato.

Per la fixture v1, nella quale ogni intent ha esattamente un NoteOff (derivato oppure esplicito), vale `|E_exp| = 2 × I`, cioè 2.784 = 2 × 1.392. Nel percorso nominale `E_canceled = ∅`. La formula generale resta quella della tabella.

**Due percorsi** (precisazione DIF-5.0-R3-02 del 2026-10-26):

| Controllo | Fixture nominale | Fixture con guasto iniettato |
|---|---|---|
| `hash(offline) == hash(JIT) == golden` del piano completo | obbligatorio | `NOT_APPLICABLE`: dopo un ack non valido il Coordinator smette di pianificare, quindi il piano JIT è parziale per costruzione |
| riconciliazione O3 per identità (punti 2–7) | obbligatoria, con `E_canceled = ∅` | obbligatoria sugli eventi effettivamente generati fino all'interruzione, comprese le cancellazioni con ragione ammessa |
| verdetto complessivo dello scenario | PASS solo se tutti i controlli passano | **sempre FAIL**, perché il guasto è iniettato deliberatamente; la riconciliazione O3 è un sottocontrollo d'integrità che può passare, ma non trasforma il guasto in un successo |

1. Solo per la fixture nominale: `hash(offline) == hash(JIT) == golden`.
2. **Confronto per identità, non per contatori.** L'insieme atteso `E_exp` si costruisce dal piano. Deve valere la partizione `E_exp = E_sent ⊎ E_canceled`, e ogni elemento di `E_sent` deve corrispondere a esattamente un record consumato. I contatori osservati si confrontano solo come controllo aggiuntivo. Un evento perso, duplicato, o cancellato senza ragione ammessa fa fallire il confronto con il piano atteso e non viene compensato dall'uguaglianza fra contatori osservati.
3. Ogni NoteOn atteso **inviato** ha esattamente un record `Applied`, con stesso `eventId`, ruolo e `sampleFrame`, e frame consumato uguale a quello pianificato.
4. Ogni NoteOff atteso, derivato o esplicito, è associato al proprio NoteOn tramite `eventId` e ruolo. Se è inviato, ha un record `Applied`, oppure un `Ignored` previsto e giustificato (es. la voce era già stata rubata). Un NoteOff `Ignored` non previsto è FAIL.
5. Nessun record consumato fuori da `E_sent`, salvo i `ParameterChange` del tono diagnostico, esclusi dal controllo.
6. Nessuno scarto e nessuna cancellazione sono contati come applicazione. Nella fixture nominale `Dropped` = 0 ed `E_canceled = ∅`.
7. **Arresto d'emergenza (D-INC4-R3-01).** Si tengono separate due categorie:
   - eventi **cancellati prima dell'invio** (`Revoked` dal confine, `ClearedAtStop`): stanno in `E_canceled` e non devono comparire nell'applied-event log;
   - eventi **già committed** nella SPSC e invalidati dallo `Stop`: stanno in `E_sent` e compaiono come `Dropped` con ragione `StopDrain` o `StaleFrameOrEpoch`, e corrispondono uno a uno a quelli che il probe classifica come invalidati.

   Ogni cancellazione deve avere una ragione ammessa nel percorso di guasto previsto.

   **NoteOff conservati dalla revoca** (precisazione DIF-5.0-R3-01 del 2026-10-26). `revokeFromFrame` conserva nello staging i NoteOff delle note già consegnate, ma solo `dispatchDueEvents` li trasferisce alla SPSC, e lo `Stop` successivo può cancellarli con `clear()`. La classificazione dipende quindi dallo stato effettivo dell'evento, registrato dallo scheduler:

   | Stato del NoteOff | Classificazione |
   |---|---|
   | già trasferito alla SPSC prima della revoca | `E_sent` |
   | ancora nello staging, poi trasferito | `E_sent` |
   | ancora nello staging, poi eliminato dallo `Stop` | `E_canceled`, ragione `ClearedAtStop` |

   Nessuna regola colloca un NoteOff in `E_sent` solo perché la revoca lo ha conservato.

   **Limite noto** (review di 5a, 2026-10-26): l'associazione fra un NoteOff revocato e il proprio NoteOn usa la coppia (`eventId`, ruolo) senza epoch. È adeguata finché gli `eventId` sono unici nella run, come nella fixture. Va rivalutata se il runtime ammetterà il riuso degli identificativi fra epoch.

**Precisazioni operative di 5a** (richieste dalla review di chiusura di 5.0, 2026-10-26):

- **Disposizioni ammesse per percorso.** I punti 3 e 4 si applicano per percorso, il punto 7 ne regola l'eccezione.
  - *Percorso nominale:* ogni NoteOn inviato è `Applied`. Ogni NoteOff inviato è `Applied`, oppure `Ignored` solo se appartiene a un insieme di ignorati attesi dichiarato in anticipo (vuoto per la fixture v1). Nessun `Dropped`.
  - *Percorso con guasto:* oltre alle disposizioni nominali è ammesso `Dropped`, ma solo per eventi di `E_sent` consumati a un frame non anteriore al **frame di taglio del guasto**, con ragione `StaleFrameOrEpoch`, `FutureEpochUnmatchable` o `StopDrain`. Un `Dropped` anteriore al taglio, o senza ragione ammessa, è una perdita arbitraria e rende la riconciliazione FAIL.
  - Il frame di taglio è il minimo fra l'inizio del blocco audio che contiene il confine della transizione fallita (se raggiunto) e l'inizio del blocco in cui il Coordinator rileva l'ack non valido. È la stessa regola del contatore `unexpectedDrops` di Inc-4, espressa per frame invece che per contatore.
- **Ambito della contabilizzazione.** Il registro degli eventi consumati si stacca dal motore, e la catena si chiude, **prima** della prova dell'evento d'epoch vecchia di `fixture_probe_v1`. Quella prova inietta un NoteOn direttamente nel motore, senza scheduler: è un controllo di fault injection separato, fuori da `E_exp`, e non produce record nel registro.
- **Drain della SPSC.** L'uguaglianza `E_sent = E_cons` si verifica dopo il drain completo della coda. In quiescenza un'API di sola lettura del motore conferma che la coda degli eventi programmati è vuota.

**Verifica (5a):**

- test di fault injection:
  - un evento perso fra scheduler e SPSC;
  - un evento in ritardo, che diventa `Dropped`;
  - un NoteOff non corrispondente, che diventa `Ignored`;
  - un evento duplicato.

  Ognuno deve produrre FAIL sul confronto con l'insieme atteso, anche quando i contatori osservati sono coerenti fra loro.
- test dell'arresto d'emergenza, con due casi separati (DIF-5.0-R2-01):
  - eventi revocati prima dell'invio, che finiscono in `E_canceled` e sono assenti dall'applied-event log;
  - eventi già committed e invalidati dallo `Stop`, che finiscono in `E_sent` come `Dropped`.

  - un NoteOff conservato dalla revoca e poi cancellato dallo `Stop` prima dell'invio, classificato `E_canceled` / `ClearedAtStop` (DIF-5.0-R3-01); se fosse atteso in `E_sent`, il controllo deve fallire.

  In tutti i casi la riconciliazione O3 passa quando cancellazioni e scarti hanno le ragioni ammesse, il golden del piano completo è `NOT_APPLICABLE` e il verdetto complessivo dello scenario resta FAIL (DIF-5.0-R3-02). Una cancellazione senza ragione ammessa fa fallire anche la riconciliazione;
- test dell'overflow con capacità ridotta, sia dell'applied-event log sia del registro delle cancellazioni;
- test della cardinalità: sulla fixture nominale `|E_exp| = 2 × I` e la catena passa (nessun falso FAIL);
- test del fingerprint:
  - due piani che differiscono solo per seed ed `eventId` producono lo stesso fingerprint e hash di provenienza diversi;
  - una sola nota diversa cambia il fingerprint;
  - **due test sui rilasci:** due piani identici salvo l'associazione dei NoteOff espliciti fra note diverse producono fingerprint diversi; lo scambio fra due note identiche non cambia il fingerprint;
  - una nota con rilascio derivato e una con lo stesso frame di rilascio ma esplicito producono fingerprint diversi. All'ascolto nominale sono uguali, ma il contratto è diverso: lease aperta con failsafe `audioMaxLeaseDeadline` contro durata fissata. Il fingerprint resta quindi conservativo;
- `fixture_probe_v1` con hash JIT = golden;
- test `perf`.

**Alternative scartate:**

- *Ricavare gli eventi applicati dallo scheduler:* prova l'invio, non l'applicazione. È il problema di R3-02.
- *Contatori aggregati per disposizione:* non permettono la corrispondenza uno a uno.
- *Fingerprint come hash di provenienza senza seed:* resterebbero `eventId`, candidati e punteggi, che possono variare a musica invariata.
- *Fingerprint sui float grezzi:* differenze sotto il centesimo di semitono non sono musicali e renderebbero fragile l'identità.

### D-M2-13 — Stem per ruolo offline (O1)

*Rilievo: INC5-REV-04. Fase: 5c.*

- **Punto di prelievo:** `RoleBusProcessor::processAndMix`, uscita stereo di ciascun ruolo dopo gain del bus, micro-ducking di Corpo e delay stereo della Trama, prima della somma nel mix dei bus.
- **Formato:** stereo float32 per ruolo. Il mono `(L+R)/2` si ricava solo nel worker d'analisi, quando serve (onset).
- **Sink:** puntatore opzionale con buffer preallocati dall'harness. Valgono gli stessi vincoli di quiescenza, assenza di allocazioni e scrittura bounded di D-M2-12. Il sub-blocking a 1.024 frame del `VoiceAllocator` resta invariato. Nessuno stem nel percorso realtime del gioco.
- **Memoria:** 8.825.200 frame × 2 canali × 4 B × 4 ruoli ≈ 282 MB, più il master stereo (≈ 71 MB). Solo offline. Il picco reale si misura in 5c e si registra.
- **Dominio della ricostruzione:**
  - la somma dei quattro stem coincide con il **mix dei bus** prelevato nello stesso punto, entro una tolleranza numerica dichiarata in 5c (somma float, ordine fisso);
  - non coincide con il PCM pre-limiter finale, perché il motore applica poi la rampa di trasporto e può sommare il tono diagnostico. Nessun test confronta stem e PCM finale.
- **`ChannelFrame.onset`:** campo dichiarato **riservato** nell'header e nella documentazione. La valorizzazione realtime (opzione O2) è rinviata alla ricerca d'ingresso R-M5-3. Il valore resta 0 e non va letto come «nessun onset».
- **Verifica (5c):** test di ricostruzione stem → mix dei bus; test di isolamento: con un solo ruolo attivo gli altri stem restano a zero; picco di memoria registrato.
- **Alternative scartate:**
  - *stem mono:* perdono lo stereo della Trama, cioè l'unico ruolo con delay L/R (E3 del dossier CACP);
  - *analisi in streaming senza conservare gli stem:* meno memoria, ma impedisce i riferimenti per ruolo di D-M2-15 e la ripetibilità dell'analisi; resta un'opzione per M5;
  - *onset nella callback (O2):* tocca il percorso realtime, rinviato a M5.

### D-M2-14 — Descrittori `descriptors_v1`

*Rilievo: INC5-REV-05. Fase: 5b; calibrazione in 5c.*

Tutti i descrittori stanno nel nuovo target offline `cymatica_cacp`, C++, mai linkato al percorso realtime (R5, opzione T3). Python resta strumento di ricerca. Ogni cambio di parametri produce una nuova versione (`descriptors_v2`, …), registrata nel manifest.

**Onset** (procedura del dossier sui metodi CACP §3.2, già fissata prima dei risultati di E1):

| Parametro | Valore iniziale |
|---|---|
| STFT | Hann 2.048 campioni, hop 256 (5,33 ms) a 48 kHz |
| Novelty | spectral flux di `log(1 + 100·|X|)`: differenza fra frame, rettificata a metà, somma sui bin, normalizzata al massimo |
| Soglia | media mobile ±100 ms + δ |
| Distanza minima fra onset | 30 ms |
| Tolleranza di corrispondenza | ±50 ms (default di mir_eval), estremo incluso; ±25 ms solo come diagnostica |
| Corrispondenza | uno a uno a cardinalità massima, non greedy: il greedy di E1 può sottostimare P e R (dossier di ricerca §4.3) |
| Ground truth per ruolo | frame dei NoteOn `Applied` del ruolo; NoteOn entro 30 ms fusi tenendo il primo; note senza nuovo attacco escluse dalla recall e riportate a parte |
| Tempo del frame | centro del frame, `(k·hop + N/2)/fs` |
| Correzione dello scarto temporale | calibrata per ruolo in 5c, non fissata a −8 ms |
| δ | calibrato per ruolo in 5c; punto di partenza 0,10 |

**Profilo per ruolo:** un solo rilevatore, con parametri versionati per ruolo:

- δ;
- scarto costante;
- larghezza del filtro massimo in frequenza (1 = flux classico, 3 bin = SuperFlux);
- lag.

Le varianti (SuperFlux per il vibrato di Vettore; lag maggiore o rilevatore d'inviluppo per gli attacchi di 15–20 ms di Corpo e Trama) entrano in una versione dichiarata solo se la calibrazione di 5c le giustifica (dossier di ricerca d'ingresso §4.3).

**Altri descrittori e formati:**

- **Side/Mid per finestra:** `10·log10(Σ side² / Σ mid²)`, con `side = (L−R)/2` e `mid = (L+R)/2`.
- **Spettrogramma mel:**
  - 128 bande, formula HTK, 30 Hz–16 kHz, filtri triangolari sulla STFT di cui sopra;
  - ampiezza in dB normalizzata sugli 80 dB superiori;
  - asse temporale aggregato a massimo sulla larghezza dell'immagine;
  - marcatori di battuta e di sezione.
- **PNG:** RGB 8 bit, filtro 0 per riga, zlib con blocchi deflate *stored* (non compressi, al massimo 65.535 byte per blocco), CRC-32 per chunk, Adler-32 nel trailer zlib.
- **Hash dei file:** SHA-256 (FIPS 180-4).
- **Loudness esplorativa** (ITU-R BS.1770-5; EBU Tech 3341 v4.0):
  - K-weighting a 48 kHz con i coefficienti delle Tabelle 1 e 2 di BS.1770-5;
  - pesi L/R = 1,0;
  - `L = −0,691 + 10·log10(Σ G·z)`;
  - Momentary su finestra rettangolare scorrevole di 0,4 s; Short-term su 3 s;
  - Integrated con blocchi da 400 ms sovrapposti al 75 %, gate assoluto −70 LUFS e gate relativo −10 LU; il blocco incompleto finale si scarta;
  - valori `MEASURED`, mai PASS.

**Casi di test** (fase 5b, tratti dalla tabella dei requisiti minimi di Tech 3341):

| Caso | Segnale | Atteso |
|---|---|---|
| 1 | sinusoide 1 kHz stereo in fase, picco −23,0 dBFS per canale, 20 s | M, S, I = −23,0 ± 0,1 LUFS |
| 2 | come #1 a −33,0 dBFS | −33,0 ± 0,1 LUFS |
| 3–5 | sequenze di toni con livelli diversi, che verificano il gate relativo e quello assoluto | I = −23,0 ± 0,1 LUFS |
| 9 | toni alternati −20/−30 dBFS di 1,34/1,66 s | S = −23,0 ± 0,1 LUFS, costante dopo 3 s |
| 12 | toni alternati −20/−30 dBFS di 0,18/0,22 s | M = −23,0 ± 0,1 LUFS, costante dopo 1 s |

Altri casi propri:

- silenzio: nessun blocco supera il gate;
- durata inferiore a un blocco di 400 ms;
- segnale con NaN in ingresso: il sanificatore dei probe lo azzera, e il test lo dichiara.

**Valori non definiti** (silenzio, durata insufficiente, risultato non finito): si serializzano come `null` con stato `INCONCLUSIVE`. Non si producono mai JSON non validi né PASS impliciti. La regola vale per tutti i descrittori.

**Contratto di validità delle primitive** (DIF-5b-01/02, review di 5b del 2026-10-28). Ogni primitiva restituisce uno stato esplicito accanto al valore: `OK`, `NON_FINITE_INPUT`, `EXACT_SILENCE`, `BELOW_NUMERICAL_FLOOR` o `INVALID_CONFIG`. Uno stato diverso da `OK` vale `null` / `INCONCLUSIVE`.

- **Ingressi non finiti** (NaN, ±∞) in STFT, novelty, peak picking, abbinamento, mel, Side/Mid e loudness danno `NON_FINITE_INPUT`, senza alcun valore. In particolare, una lista di onset vuota è un «nessun onset» valido solo con stato `OK`.
- **Mel:** il silenzio non si rappresenta mai come energia.
  - Matrice tutta nulla → `EXACT_SILENCE`; massimo non superiore al pavimento di ampiezza 10⁻¹⁰ → `BELOW_NUMERICAL_FLOOR`. In entrambi i casi tutte le celle normalizzate valgono 0.
  - Per i contenuti validi la normalizzazione resta relativa al massimo, sugli 80 dB superiori, e il livello assoluto del massimo si riporta in `maxDb`.
  - Il banco HTK e la normalizzazione non cambiano.
- Silenzio e segnale troppo corto restano `OK` per la loudness, con valori `null`, perché BS.1770 li lascia indefiniti.
- **Dominio dei parametri** (DIF-5b-R2-01, 2026-10-28): fuori dal dominio l'esito è `INVALID_CONFIG`, prima di qualunque conversione a intero:
  - semi-finestra della soglia e separazione minima in [0, 3600] s;
  - δ finita;
  - `logGamma` > 0 e finita;
  - tolleranza dell'abbinamento in [0, 3600] s.
- **Validazione autonoma dei dati intermedi** (DIF-5b-R2-02): novelty e mel validano lo spettrogramma ricevuto (stato, dimensioni, finitezza e non negatività di ogni valore) invece di fidarsi dello stato a monte.
- **Side/Mid strutturato:** restituisce stato, valore opzionale e i flag `sideIsZero` / `midIsZero`, così mono, controfase, silenzio esatto, ingresso non finito e dimensione non valida sono distinguibili.

**Verifica (5b):**

- FFT confrontata con una DFT diretta;
- vettori di riferimento per CRC-32, Adler-32 e SHA-256: `SHA-256("abc") = ba7816bf…f20015ad`, dall'esempio NIST;
- PNG riletto e controllato chunk per chunk;
- onset su impulsi sintetici in frame noti;
- Side/Mid su L = R e su L = −R;
- casi LUFS della tabella;
- parità con E1 del dossier CACP sul WAV della fixture, documentata ma non usata come gate.

**Esito dell'implementazione (5b, 2026-10-27):** nuovo target offline `cymatica_cacp` (`engine/cacp/`), mai linkato al percorso realtime. Lo scenario `--cacp-run onset_parity_v1` riproduce E1 sullo stesso ingresso: rilevati, precisione e richiamo coincidono con E1 a tutte e tre le δ (991/730/464), così come scarti e richiami per ruolo. Sulla fixture l'abbinamento a cardinalità massima e quello greedy di E1 danno gli stessi abbinamenti.

**Dopo DIF-5b-03** (2026-10-28) il confronto è automatico campo per campo, su tutti i valori pubblicati da E1:

- conteggi esatti;
- P, R, F, P/R/F per sezione e richiamo per ruolo entro 0,0005;
- scarto mediano, p05 e p95 entro 0,005 ms.

Le tolleranze valgono metà dell'ultima cifra stampata da E1. Lo scenario dà `MEASURED` solo se la fixture ha superato il proprio verdetto e il PCM è finito; altrimenti dà `INCONCLUSIVE`, con codice d'uscita diverso da zero. Dati in `docs/research/experiments/cacp_methods/onset_parity_v1_2026-10-28.json` (la prima esecuzione è in `…_2026-10-27.json`).

### D-M2-15 — `evaluation_pack_v1`, separazione della ground truth e `evaluation_result_v0`

*Rilievo: INC5-REV-03. Fase: 5d.*

Il pacchetto prodotto da `--cacp-run evaluation_pack_v1` ha **due modalità** con visibilità dei metadati diversa (precisazione DIF-5.0-03 del 2026-10-26). Le modalità condividono i descrittori, non i metadati visibili.

| | `blind` — valutazione alla cieca | `diagnostic` — valutazione esplicativa |
|---|---|---|
| Scopo | misurare che cosa il valutatore percepisce dall'audio o dalle immagini | spiegare e ispezionare la run, ad esempio per trovare difetti |
| Identificazione delle finestre | ID opachi, in ordine permutato | battute e frame assoluti, sezione, frase |
| Tempi | relativi all'inizio della finestra; nessun indice assoluto (battuta, frame, sezione, frase) che riveli la maschera dei ruoli | assoluti |
| Durata delle finestre | uguale per tutte le finestre: un estratto della frase con durata comune non superiore alla frase più breve (4 battute a 140 BPM ≈ 6,857 s), dichiarata nella review di 5d, con fade dichiarati. La durata non deve rivelare la sezione. | frase intera |
| Ground truth | assente; solo in `sealed/` | intent, eventi `Applied`, onset per ruolo e viste esplicative (es. la vista «a pentagramma») sono visibili |
| Seed di selezione e permutazione | stream RNG isolato, distinto dal seed musicale; il valore sta in `sealed/`, mentre `public/` contiene solo il suo hash (impegno) | dichiarato in chiaro |
| Qualifica del giudizio | può essere qualificato «alla cieca» solo se il valutatore dichiara di non aver avuto accesso a repository, `sealed/` o pacchetti `diagnostic` della stessa run | **mai** qualificato «alla cieca» |

Il tempo musicale e i ruoli udibili nell'audio restano percepibili per costruzione. Riconoscerli ascoltando è proprio la percezione che la modalità `blind` vuole misurare, non una fuga di metadati.

**Contenuto della modalità `blind`:**

| `public/` — consegnata al valutatore | `sealed/` — riservata al checker |
|---|---|
| WAV e PNG mel Mid/Side per ciascuna finestra di prova, con ID opachi | ruoli pianificati, eventi applicati e presenza misurata per finestra |
| metadati tecnici non rivelatori: sample rate, durata comune, versione dei descrittori, hash del seed | chiave fra ID opachi e finestre, con battute, frame, sezione ed epoch |
| riferimenti etichettati autorizzati: spettrogrammi degli stem per ruolo e sezione, tratti **da finestre diverse** da quelle di prova | seed di selezione e permutazione |
| `SHA256SUMS` dei file pubblici | regole di verifica, risultati attesi, `SHA256SUMS` dei file sigillati |

**Selezione delle finestre:**

- unità di una frase, cioè 4 battute, ritagliata nella modalità `blind` alla durata comune;
- per ogni sezione almeno una finestra di prova con un ruolo presente e una di controllo con lo stesso ruolo assente, quando la maschera lo consente. Esempio: Trama assente nelle frasi 0–3 di A e presente nelle frasi 4–7;
- i controlli aggiuntivi si fissano nella review di 5d, sulla base del dossier di ricerca d'ingresso §3.3 e §6:
  - negativi bilanciati, duplicati nascosti, catch trial, item a verità alterata, esito «non determinabile»;
  - registro degli accessi, pulizia dei metadati, confronto con una baseline banale;
  - controllo di discriminazione per ablazione (piano completo contro varianti ablate, come in MetaCompose);
  - vista «a pentagramma» con intent, eventi `Applied` e onset per ruolo (sul modello delle debug UI di Hi-Fi Rush), **solo** nella modalità `diagnostic`.

**Protocollo di separazione:**

- **Consegna:** il valutatore riceve soltanto `public/`, copiato in una directory separata o allegato al client.
- **Collocazione di `sealed/`:** viene scritto in un percorso scelto da chi lancia lo scenario. Per default sta fuori da `artifacts/cacp/`, così non si trova accanto ai file pubblici.
- **Hash degli input:** il risultato v0 dichiara la modalità del pacchetto e gli hash dei file effettivamente ricevuti. Il checker rifiuta un risultato i cui input includono file di `sealed/`, file sconosciuti o file `diagnostic` presentati come `blind`.
- **Limite dichiarato:** un agente con accesso al repository può rieseguire il planner e ricostruire la ground truth. La separazione è quindi procedurale, non crittografica. Ogni risultato v0 registra gli strumenti e gli accessi del valutatore (filesystem, esecuzione di codice); un valutatore con accesso al repository non produce evidenza percettiva «alla cieca».

**`evaluation_result_v0`** (JSON, sottoinsieme della proposta D-AO-04 di agent_control_observability §13.2):

- versione dello schema e versione dell'insieme di predicati (`claims_v0`);
- modalità del pacchetto (`blind` o `diagnostic`);
- valutatore: tipo, modello e versione;
- capacità dichiarate del canale: canali, sample rate o bitrate, token/s, risoluzione delle immagini;
- strumenti e accessi;
- hash degli input ricevuti;
- timestamp;
- lista di affermazioni **tipizzate** (precisazione DIF-5.0-04 del 2026-10-26).

**Affermazioni tipizzate.** Ogni affermazione ha questi campi:

- `window_id`;
- `method` ∈ {`listening`, `spectrogram`, `descriptor`, `code`};
- `predicate`, da un insieme chiuso e versionato;
- argomenti tipizzati;
- `claimed_value`;
- `text`, solo descrittivo e mai usato come input del controllo.

**Predicati di `claims_v0`** (precisazione DIF-5.0-R2-02 del 2026-10-26: presenza del segnale, attacco e udibilità sono predicati distinti):

| Predicato | Argomenti e valore | Base di verità nel checker |
|---|---|---|
| `role_signal_present` | `role`; `claimed_value`: booleano | **presenza energetica** dello stem nella finestra: energia dello stem sopra un livello assoluto e rapporto stem/mix sopra una soglia, entrambi calibrati in 5c. Vale anche senza nuovi attacchi, ad esempio per un pad sostenuto che entra nella finestra |
| `role_onset_detected` | `role`; `claimed_value`: booleano | almeno un nuovo attacco rilevato sullo stem nella finestra, con il rilevatore di D-M2-14 |
| `role_audible` | `role`; `claimed_value`: booleano | **giudizio percettivo**: nessuna misura lo rende vero. Il checker può solo contraddirlo, o dichiararlo compatibile con le misure (regola sotto) |
| `role_planned` | `role`; `claimed_value`: booleano | presenza pianificata (intent); ammesso solo con `method = code`, perché non è una percezione |
| `stereo_wider_than` | `window_id_other`; `claimed_value`: booleano | Side/Mid delle due finestre; `not_attributable` se il canale dichiarato è mono |
| `artifact_present` | `kind` ∈ {`click`, `clipping`, `dropout`}; `claimed_value`: booleano | descrittori di integrità del PCM della finestra; `unverifiable` se il descrittore per quel tipo non esiste |
| `observation` | nessun valore | nessuna; sempre `unverifiable` |

**Livelli di verità per un ruolo** nella finestra, tenuti distinti:

1. *pianificata:* esistono intent del ruolo che suonano nella finestra;
2. *applicata:* esistono NoteOn `Applied` del ruolo (D-M2-12);
3. *segnale presente:* `role_signal_present` misurato sullo stem;
4. *attacco rilevato:* `role_onset_detected` misurato sullo stem.

Un pad sostenuto che attraversa l'inizio della finestra ha il segnale presente senza alcun attacco rilevato.

**Regola per `role_audible`.** L'udibilità nel mix dipende dal mascheramento e dall'ascoltatore, e le misure sugli stem non la dimostrano.

| Dichiarazione | Misura | Esito |
|---|---|---|
| `role_audible = true` | **assenza dimostrata** del segnale: silenzio numerico dello stem nella finestra (sotto) | `contradicted`: un segnale assente non può essere udito |
| `role_audible = true` | segnale sotto la soglia di `role_signal_present`, ma non assente | `unverifiable`: una soglia di rilevazione non equivale al silenzio, e un segnale debole può essere percepibile |
| `role_audible = true` | segnale presente | `compatible`: non contraddetta dalle misure; l'udibilità non è verificata |
| `role_audible = false` | assenza dimostrata, oppure segnale sotto soglia | `compatible` |
| `role_audible = false` | segnale presente | `unverifiable`: il ruolo potrebbe essere mascherato nel mix |
| qualsiasi | misura non disponibile o `INCONCLUSIVE` | `unverifiable` |

**Assenza dimostrata** (precisazione del 2026-10-26, terza review): il picco dello stem del ruolo nella finestra è sotto un pavimento numerico dichiarato. Profilo iniziale: −120 dBFS, ben oltre la gamma dinamica di un convertitore a 20 bit; il valore si fissa nella review di 5d. Una misura sotto la soglia calibrata di `role_signal_present`, ma sopra questo pavimento, non prova l'assenza. Diventerebbe una contraddizione solo dopo una validazione percettiva specifica di quella soglia, che oggi non esiste. **Nota per 5d** (review di chiusura di 5.0): si distinguono `exact_silence` (tutti i campioni dello stem nella finestra sono nulli) e `below_numerical_floor` (picco sotto il pavimento ma non nullo). Solo `exact_silence` è assenza dimostrata in senso matematico. Per `below_numerical_floor` si evita una contraddizione categorica dell'udibilità senza una validazione percettiva; la scelta definitiva si fissa nella review di 5d.

L'esito di `role_audible` riporta sempre il limite: «verifica percettiva indipendente assente». La conferma dell'udibilità resta all'ascolto umano (M2.5).

**Esiti del checker** (`--cacp-check-evaluation`), per affermazione:

| Esito | Significato |
|---|---|
| `supported` | il valore dichiarato coincide con la base di verità misurata del predicato (`role_signal_present`, `role_onset_detected`, `stereo_wider_than`, `artifact_present`; `role_planned` con `method = code`) |
| `compatible` | solo per `role_audible`: non contraddetta dalle misure, ma senza verifica percettiva indipendente |
| `contradicted` | il valore dichiarato è opposto alla base di verità (es. `role_onset_detected = true` senza attacchi rilevati; `role_audible = true` con assenza dimostrata del segnale) |
| `not_attributable` | il metodo dichiarato non può produrla: stereo da un canale mono; fenomeno sotto la risoluzione temporale del canale (~31 ms a 32 token/s); `role_planned` o affermazioni da `code` presentate come percezione |
| `inadmissible` | violazione delle regole di D-M2-11 o della modalità: spettrogramma letto senza riferimenti, input da `sealed/`, pacchetto `diagnostic` dichiarato `blind`, finestra o predicato non validi |
| `unverifiable` | `observation`, base di verità non disponibile o `INCONCLUSIVE`, `role_audible = false` con segnale presente |

L'esito aggregato è riportato separatamente dai gate e non decide mai un verdetto (D-M2-11).

**Verifica (5d):**

- due run nella stessa build producono gli stessi hash dei file pubblici di ciascuna modalità;
- test del checker con affermazioni prefabbricate, almeno una per esito e per predicato;
- **nota sostenuta:** un pad che attraversa l'inizio della finestra, senza NoteOn al suo interno, dà `role_signal_present = true` → `supported`, `role_onset_detected = true` → `contradicted` e `role_audible = true` → `compatible`;
- `role_audible = true` con lo stem in silenzio numerico dà `contradicted`; con lo stem sotto la soglia di presenza ma sopra il pavimento dà `unverifiable`; `role_audible = true` non dà mai `supported`;
- test di leakage della modalità `blind`:
  - nessun file di `public/` contiene ruoli, intent, battute o frame assoluti, sezione, epoch, eventId, chiave o seed in chiaro (scansione dei campi, dei metadati WAV/PNG e dei nomi dei file);
  - tutte le finestre hanno la stessa durata;
- un risultato che dichiara un input sigillato, o un pacchetto `diagnostic` come `blind`, produce `inadmissible`.

**Precisazioni d'ingresso della fase 5d** (2026-10-29, `accepted for implementation`, da confermare nella review di 5d). Fissano i punti che questa decisione rimandava alla review di 5d. Implementazione in [evaluation_pack.h](../../engine/cacp/evaluation_pack.h) / [.cpp](../../engine/cacp/evaluation_pack.cpp); evidenze in [experiments/inc5_evaluation_pack/](../research/experiments/inc5_evaluation_pack/).

| ID | Punto | Scelta | Motivazione | Alternative scartate |
|---|---|---|---|---|
| 5d-01 | Durata comune `blind` | 6,0 s (288.000 frame) dall'inizio della frase, fade a coseno rialzato di 10 ms in ingresso e in uscita; `diagnostic`: frase intera | ≤ 6,857 s (frase più breve, 4 battute a 140 BPM), uguale per tutte le sezioni; il fade evita un click di taglio senza toccare gli attacchi interni | estratto centrato (perde il battere iniziale); durata pari alla frase (rivela la sezione dal tempo) |
| 5d-02 | Dominio dell'audio consegnato | somma degli stem inclusi (ordine del `RoleBusProcessor`), con guadagno e fade; la verità si misura sugli stessi stem consegnati | la somma riproduce il mix dei bus al bit (D-M2-13); il limiter della fixture ha riduzione massima 0 dB, quindi coincide con il PCM salvo rampa di trasporto. Permette l'item a verità alterata senza cambiare dominio | PCM finale (l'item alterato richiederebbe un secondo dominio) |
| 5d-03 | Profilo `presence_profile_v1` di `role_signal_present` | presente se RMS dello stem ≥ −60 dBFS **e** rapporto stem/mix ≥ −30 dB nella finestra consegnata | la fixture separa i casi senza ambiguità: ogni ruolo pianificato misura RMS ≥ −39,4 dBFS e rapporto ≥ −15,6 dB (minimi della Trama), ogni ruolo non pianificato è silenzio esatto. Le soglie stanno circa 20 dB e 14 dB sotto i minimi misurati. **Non validate:** la fixture non contiene livelli intermedi (code, ducking, pad deboli), quindi le soglie sono coerenti con i dati ma non messe alla prova. Si rivalutano in 5e o M2.5 con contenuti che li producono | soglie ricavate da standard di udibilità (misurano un'altra cosa, D-M2-15 separa presenza e udibilità) |
| 5d-04 | Assenza dimostrata | pavimento −120 dBFS di picco. `EXACT_SILENCE` (tutti i campioni nulli) è l'unica assenza dimostrata. `BELOW_NUMERICAL_FLOOR` e `BELOW_THRESHOLD` danno `unverifiable` per `role_audible = true` | nota della review di chiusura di 5.0: senza validazione percettiva non si contraddice categoricamente l'udibilità di un segnale non nullo | contraddizione anche sotto il pavimento |
| 5d-05 | Verità di `role_onset_detected` | vero se almeno un NoteOn `Applied` del ruolo che inizia nella finestra è abbinato a una rilevazione (profilo congelato `stem_onset_profile_v1`, ±50 ms, rilevatore sullo stem intero); un ruolo rimosso dall'audio non ha attacchi | le rilevazioni non abbinate sono rilasci (F6) o artefatti, non «nuovi attacchi»; il rilevatore sull'intero stem evita onset spuri al taglio | conteggio grezzo delle rilevazioni nella finestra (renderebbe vero l'attacco sul rilascio di un pad) |
| 5d-06 | `stereo_wider_than` | Side/Mid della finestra consegnata; differenza sotto 1 dB → `unverifiable` | sotto un dB la differenza non è una larghezza percepibile né stabile | confronto senza margine |
| 5d-07 | `artifact_present` | `clipping`: campioni con \|x\| ≥ 1 nella finestra consegnata. `click` e `dropout`: nessun descrittore in 5d → `unverifiable`. Un `click` dichiarato con `listening` su un canale con risoluzione > 10 ms (es. 32 token/s) → `not_attributable` | D-M2-11; F6 è un fenomeno di 2 ms | descrittore di click improvvisato senza validazione |
| 5d-08 | Esito «non determinabile» | `claimed_value: null` → esito `abstained`, contato a parte, mai successo | dossier d'ingresso §3.3 (S5: un'opzione esplicita di abbandono riduce l'imbroglio). Nuovo esito accanto ai sei di questa decisione | trattarlo come `unverifiable` (confonderebbe la scelta del valutatore con l'assenza di verità) |
| 5d-09 | Controlli del pacchetto `blind` | per ogni (sezione, maschera dei ruoli pianificati) fino a 2 frasi di prova estratte dallo stream di selezione: dove la maschera varia ci sono finestre con un ruolo presente e altre con lo stesso ruolo assente; **duplicato nascosto** di una finestra di prova con guadagno −0,1 dB **e inizio spostato di 7 ms** (336 frame, non multiplo dell'hop: WAV e PNG diversi, AG-5d-03); **item a verità alterata**: una frase non usata come prova con ≥ 2 ruoli presenti, un ruolo presente rimosso dall'audio ma non dal piano, scelto **preferibilmente in modo che i ruoli rimasti coincidano con la maschera pianificata di un'altra frase** (non riconoscibile contando i ruoli, AG-5d-03); in caso contrario la verità sigillata registra `altered_mask_novel`; **catch trial**: finestra di silenzio esatto | dossier d'ingresso §3.3 (S5, S7, S11, S12) | duplicati identici al bit (riconoscibili dall'hash); solo guadagno (si annulla nei PNG normalizzati e lascia il WAV come copia scalata, AG-5d-03) |
| 5d-10 | Regole dei controlli, fissate prima di ogni misura | catch trial fallito se il valutatore afferma presenza, attacco o udibilità di un ruolo nella finestra di silenzio: le statistiche di presenza diventano `EXCLUDED_BY_CATCH_RULE`. Item alterato: conteggio delle affermazioni vere sul ruolo rimosso (`altered_plan_following_claims`). Duplicati: concordanza delle affermazioni booleane uguali | analogo del post-screening di MUSHRA (S7), con soglie proprie | soglie copiate da MUSHRA |
| 5d-11 | Confronto con la baseline banale | sensibilità e specificità di `role_signal_present` con intervallo di Wilson al 95 %; supera la baseline banale solo se la somma dei due estremi inferiori è > 1 (un classificatore banale ha sensibilità + specificità = 1). Senza positivi e negativi → `INSUFFICIENT_DATA` | dossier §3.3; misurato: la baseline «sempre presente» ottiene l'82 % di accuratezza grezza con specificità 0 | accuratezza grezza (premia la baseline) |
| 5d-12 | Seed e riferimenti | stream di selezione SplitMix64 isolato dal seed musicale; ID opachi `w` + 8 cifre esadecimali; il pacchetto pubblico contiene solo l'impegno SHA-256 del seed. Il seed di default (`20261029`) è pubblico: un pacchetto generato con esso non è mai qualificato «alla cieca». Riferimenti: mel Mid dello stem solo, uno per ruolo e sezione, tratti da frasi non usate come prova né come item alterato | D-M2-15; dossier §3.3 | seed in chiaro; riferimenti dalle finestre di prova |
| 5d-13 | Qualifica «alla cieca» | pacchetto `blind`, risultato ammissibile, `access.repository`, `access.sealed` e `access.diagnostic_pack` dichiarati e falsi, seed non di default. Un accesso non dichiarato non qualifica | D-M2-15 | qualifica presunta in assenza di dichiarazione |
| 5d-14 | Separazione e integrità | ogni esecuzione genera `public_blind/`, `public_diagnostic/` (stessi descrittori, metadati visibili, `truth.json`, mel per stem) e `sealed/` (default `artifacts/sealed/evaluation_pack_v1`, fuori da `artifacts/cacp/`). Il checker verifica `sealed/SHA256SUMS` prima di leggere la verità e rifiuta, come `inadmissible`, un risultato che dichiara file sigillati, file del pacchetto `diagnostic` in un risultato `blind` o file sconosciuti. Lo scenario renderizza e costruisce due volte e richiede file identici al byte | D-M2-15 | un solo pacchetto per esecuzione (non si potrebbe riconoscere un file `diagnostic` presentato come `blind`) |
| 5d-15 | Irrobustimento del checker (secondo parere di Antigravity, 2026-10-29) | un'affermazione ripetuta per la stessa finestra, predicato e argomenti è `inadmissible` e non sovrascrive né conta (AG-5d-01); il file della finestra richiesto dal metodo (WAV per `listening` e `descriptor`, mel PNG per `spectrogram`, anche dell'altra finestra in `stereo_wider_than`) e almeno un riferimento etichettato per `spectrogram` devono essere fra gli input dichiarati (AG-5d-02); misure non disponibili serializzate come `null`, tassi e intervalli `null` quando la valutazione di presenza non è `MEASURED` (AG-5d-05); CLI fail-closed su seed non decimale, opzione senza valore e report non scrivibile, `SHA256SUMS` con CRLF accettato (AG-5d-06/07) | senza deduplicazione un `false` successivo annullava un catch trial fallito e le ripetizioni gonfiavano gli intervalli di Wilson; senza copertura degli input un'affermazione poteva riferirsi a file mai ricevuti | contare tutte le ripetizioni; considerare solo l'ultima |
| 5d-16 | Remediation della review di 5d (DIF-5d-01..03, OSS-5d-01) | **Directory:** `public_blind`, `public_diagnostic` e `sealed` disgiunte a coppie (link risolti), `sealed` né dentro né sopra la radice pubblica, verificato prima di ogni rendering; nessun `remove_all`; si riscrive solo una directory scritta dal generatore (ogni voce elencata nel suo `SHA256SUMS`), poi si verifica il disco. **Matrice metodo/predicato:** predicati acustici ed energetici mai da `code`; `role_audible` solo da `listening`; `role_planned` solo da `code`; campione di presenza solo `listening`/`spectrogram`, senza duplicato né catch trial. **Schema:** valutatore, `timestamp`, `channel.audio_channels` (0..64) e `channel.tokens_per_s` (≥ 0), i quattro flag di `access`, `spectrogram_references_used` e `claims` obbligatori; l'ascolto richiede almeno 1 canale e lo stereo da ascolto almeno 2 | fail-closed anche fuori dal percorso nominale: un percorso configurabile non deve poter cancellare dati né far finire la verità fra i file pubblici; l'ispezione del codice non produce punteggi percettivi; una capacità non dichiarata non è una capacità disponibile; un duplicato non è un'osservazione indipendente | conferma interattiva prima di cancellare (non adatta a uno scenario automatico); ammettere `code` con un'etichetta di provenienza |
| 5d-17 | Review mirata di 5d (DIF-5d-R2-01/02) | **Proprietà di una directory:** si sostituisce solo una directory integra del pacchetto. Le condizioni sono: `SHA256SUMS` ben formato e senza duplicati, insieme dei file uguale all'elenco, ogni digest uguale al contenuto, identificatore `pack.json` o `evaluation_truth.json` con `pack_version` `evaluation_pack_v1`. Tutto si verifica prima della prima rimozione; altrimenti la scrittura è rifiutata e la directory resta intatta. **Provenienza del piano:** in modalità `blind`, `role_planned` da `code` richiede l'accesso al repository dichiarato, che esclude la qualifica alla cieca; nella modalità `diagnostic` il piano è nel pacchetto | un elenco di nomi non prova la proprietà, il contenuto sì; il pacchetto `blind` non contiene il piano, quindi una verifica del piano senza repository non ha fonte | marcatore dedicato (aggiunge un file pubblico non necessario); vietare `role_planned` in modalità `blind` (perde un controllo utile, dichiarato con il suo accesso) |

**Rimandati, con motivazione:**

- *Controllo di discriminazione per ablazione* (proposta G17): richiede il rendering di varianti ablate del piano. Va a 5e, insieme a `procedural_v1`.
- *Vista «a pentagramma»* (G6, G8): il pacchetto `diagnostic` contiene già `truth.json` e i mel per stem di ogni finestra. La vista grafica è rimandata a 5e.

**Limiti dichiarati:**

- **Negativi scarsi.** La fixture ha un'unica assenza pianificata (Trama nelle frasi 0–3), più l'item alterato e il catch trial. I negativi bilanciati sono quindi possibili solo dove la maschera lo consente: è una conseguenza della diversità nulla misurata in 5c.
- **Separazione procedurale.** Resta procedurale, come da questa decisione.
- **`role_audible`:** non viene mai reso vero dalle misure.

### D-M2-16 — Calibrazione (R6), AC-AO-02 proposto, multi-seed, LUFS e CI

*Fasi: 5c e 5e.*

**Calibrazione e verifica dei parametri dei descrittori (R6):**

- *Latenza intrinseca:* prima della calibrazione si misura la latenza del rilevatore su segnali sintetici per ruolo con onset noti al campione, generati dal synth reale, e la si confronta con la convenzione temporale del frame.
- *Insieme di calibrazione:* fixture `fixture-m2-4-v1` con seed 1337. Su questo si fissano δ e correzione dello scarto per ruolo, ed eventuali varianti di D-M2-14. Il profilo si congela con versione e hash prima di eseguire la verifica. Il seed 1337 non è mai evidenza d'accettazione.
- *Insieme di verifica*, mai usato per scegliere i parametri:
  1. seed della fixture con fingerprint di contenuto (D-M2-12) diverso da quello del seed 1337, presi da una scansione dichiarata (insieme di seed e criterio fissati prima della misura);
  2. contenuto di `planner_probe_v1`;
  3. segnali sintetici per ruolo con tempi, densità e altezze diversi dalla fixture, generati in modo deterministico nei test.
- *Esiti* (precisazione DIF-5.0-05 del 2026-10-26):
  - se la scansione non trova seed con contenuto diverso, la **generalizzazione della calibrazione** all'insieme 1 è `INCONCLUSIVE`, mentre la **diversità osservata** è `MEASURED` e nulla, ed è registrata come risultato negativo (sotto, «Multi-seed»);
  - con fingerprint distinti ma meno di k, la generalizzazione si valuta sui soli seed distinti, con il flag «campione insufficiente»;
  - gli insiemi 2 e 3 restano validi in ogni caso. Se le metriche di verifica peggiorano oltre il margine dichiarato in 5c, i parametri non vengono ritoccati: si registra il risultato e si riapre la decisione (regola §6.9).

**AC-AO-02 in Inc-5:**

- **Catena O3:** controllo esatto di D-M2-12, PASS/FAIL, vincolante per gli scenari.
- **Onset per ruolo:** precisione, recall e F per ruolo sugli stem, con la ground truth dei NoteOn `Applied` del ruolo, misurati e riportati `MEASURED`. Le soglie per ruolo sono **proposte** in 5c, ciascuna con il margine osservato sull'insieme di verifica, e si ratificano al gate M2.5.
- Il master resta misurato per confronto con E1. Non è un criterio per ruolo, perché la Trama non è osservabile dal master.
- **Intent non è percezione:** questi controlli provano che un evento previsto è arrivato al segnale, non che sia udibile (dossier CACP §8).

**Esito della fase 5c** (2026-10-29, scenario `acao02_v1`, [dossier di calibrazione](../research/M2_4_Inc5_AC-AO-02_Calibration.md)):

- **Stem (D-M2-13):**
  - ricostruzione del mix dei bus esatta al bit;
  - memoria misurata 356 MB per la fixture intera;
  - presenza misurata sul PCM degli stem (OSS-5b-01);
  - i frame di silenzio «congelato» sono scritti come zeri, così gli stem restano allineati al rendering.
- **Profilo congelato `stem_onset_profile_v1`:** SHA-256 `1ad44100…b8b3cd`. Valori per ruolo:
  - Pulso: δ 0,30, scarto −11,67 ms;
  - Corpo: δ 0,30, scarto −10,67 ms;
  - Trama: δ 0,20, scarto +9,96 ms;
  - Vettore: δ 0,20, scarto −9,33 ms.
- **Verifica:**
  - altri seed: `INCONCLUSIVE`, perché i seed 1–32 producono tutti lo stesso contenuto (diversità osservata nulla);
  - `planner_probe_v1` e il set sintetico B sono misurati;
  - ogni falso positivo cade sul rilascio di 2 ms di una nota (finding F6).
- **Soglie AC-AO-02 proposte** (richiamo / precisione a ±50 ms): Pulso 0,90/0,95; Corpo 0,75/0,50; Trama 0,70/0,40; Vettore 0,90/0,45.
  - *Alternativa da decidere al gate M2.5:* precisione senza le rilevazioni entro ±50 ms da un NoteOff applicato, che vale 1,000 ovunque. Fino ad allora resta **diagnostica**.
- **Validità della misura (DIF-5c-01/02, review indipendente di `a6f360a`):** `MEASURED` significa «misurato su dati validi», non PASS di un gate.
  - *Golden per seed:* i golden fissi di `fixture_probe_v1` valgono solo per il seed 1337. Con un altro seed sono `NOT_APPLICABLE`; restano obbligatori determinismo, uguaglianza fra piano offline e JIT, riconciliazione O3 e controlli d'integrità. Il manifest riporta il seed effettivo.
  - *Cattura valida:* ogni rendering (sintetico, fixture, `planner_probe_v1`) deve avere verdetto del probe positivo, stem che coprono esattamente i frame renderizzati, nessun overflow degli stem né del registro degli eventi applicati. Un rendering non valido rende il suo insieme `INCONCLUSIVE`, senza punteggi.
  - *Analisi valida (DIF-5c-R2-01):* la validità di ogni analisi per ruolo si controlla separatamente da quella della cattura, per latenza intrinseca, punteggi di calibrazione e insiemi di verifica. `MEASURED` è valido con punteggi. `NO_REFERENCES` è valido ma senza evidenza: non vale mai come precisione o richiamo nulli e il ruolo non contribuisce alle soglie di quell'insieme. Ogni altro stato (`NON_FINITE_INPUT`, `INVALID_CONFIG`, `EXACT_SILENCE`, `BELOW_NUMERICAL_FLOOR`, `NOT_RUN`) è un fallimento dell'analisi: l'insieme diventa `INCONCLUSIVE`, il motivo resta registrato e l'esito complessivo non può essere `MEASURED`.
  - *Verdetto aggregato:* `MEASURED` solo se tutte le catture sono valide, la ricostruzione del mix è esatta e il profilo ricalibrato coincide con quello congelato. Altrimenti `INCONCLUSIVE` con le ragioni, la verifica non è conclusiva (le misure restano come diagnostica) e le soglie non sono proposte. La CLI esce con 0 solo se l'esito è `MEASURED` e il manifest è scritto.
  - *F6:* la vicinanza entro ±50 ms a un NoteOff non dimostra da sola il nesso causale. Prima di modificare l'ADSR in M2.5 serve un confronto controllato con diverse durate di release e gli altri parametri invariati.

**Multi-seed** (esplorativo, non è un gate):

- *Riproducibilità:* stesso seed, due pianificazioni → stesso hash di provenienza e stesso fingerprint.
- *Diversità:* fra seed diversi si confrontano il fingerprint, per i duplicati esatti, e le metriche M1–M4 sulla sequenza di eventi (dossier di ricerca d'ingresso §2.3). L'analisi segue l'*expressive range* della generazione procedurale: descrittori fissati prima, insieme di seed dichiarato, distribuzioni invece di un solo numero. N, k, griglia e pesi si fissano nella review di 5e, prima della misura.
- *Interpretazione degli esiti* (dossier di ricerca d'ingresso §2.3):
  - **fingerprint tutti uguali:** diversità osservata `MEASURED` e nulla. È un risultato negativo significativo sulla varietà del generatore, da registrare come finding, non un esito inconcludente;
  - **fingerprint distinti, ma meno di k:** distanze `MEASURED`, anche non nulle, con il flag «campione insufficiente per una conclusione robusta»;
  - **almeno k fingerprint distinti:** distribuzioni complete.
- La diversità non è un gate e non va massimizzata: nel gioco la variazione è meno percepita, e la ripetizione ha funzione di feedback (dossier di ricerca d'ingresso §5.2). Il criterio della ricerca approfondita resta prevalente: musica che ricorda, sviluppa, sorprende e ritorna.
- **Nessuna modifica del generatore in Inc-5 per forzare la diversità.** Eventuali sorgenti di variazione guidate dal seed richiedono una decisione successiva basata su queste misure (nota di ADR-0005).

**LUFS:** misure M, S e I sulla run `procedural_v1` secondo D-M2-14, solo `MEASURED`. Le soglie `−23 LUFS ± 2 LU` e `−1 dBTP` del protocollo restano soglie di prova, non PASS normativi (ADR-0004 §4). La fixture di circa 183 s non consente la misura rolling di 30 minuti.

**CI:**

- `full_m2_suite`, con profiling ripetuti e render lunghi, **non** diventa un gate obbligatorio sui runner condivisi, dove i tempi non sono rappresentativi;
- in 5e si valuta un controllo deterministico leggero su `procedural_v1` (catena O3, hash e integrità del PCM, senza perf);
- la suite integrale resta un gate locale documentato.

### Precisazioni d'ingresso della fase 5e (5e.0, 2026-11-02, `accepted for implementation`)

Fissano ciò che D-M2-15 e D-M2-16 rimandano a 5e e stabiliscono il perimetro della fase.

**Approvazione:** il titolare le ha approvate tutte il 2026-11-02, con precisazioni su 5e-08 e 5e-11 (più sotto). Nessun valore è `validated`: il nuovo release di produzione e la validazione delle soglie di presenza restano esclusi da Inc-5. La review del commit documentale verificherà la coerenza fra queste precisioni, il protocollo sperimentale e i criteri di accettazione di 5e. Valgono tre distinzioni, chieste dalla review di chiusura di 5d e da preservare in ogni manifest e documento di 5e:

- evidenza tecnica contro evidenza percettiva;
- misure valide contro profili validati;
- risultati su una sola fixture contro generalizzazione musicale.

| ID | Punto | Proposta | Motivazione | Alternative scartate |
|---|---|---|---|---|
| 5e-01 | Contenuto di `procedural_v1` | rendering di `fixture-m2-4-v1` (seed 1337, circa 183 s, tre sezioni con cambio di tempo) con l'intera catena in un solo manifest. **Vincolanti** (PASS/FAIL): catena O3, determinismo e golden (trace, piano e, dalla review di 5e.1, PCM; DIF-5e.1-01), integrità del PCM e invarianti del limiter già approvati (campioni finiti, frame persi nulli, \|x\| ≤ 0,89125, `clipped == 0`, `dspAnomalyCount == 0`; ADR-0005 D-M2-09). **`MEASURED`**, mai PASS: true peak confrontato con −1 dBTP (un superamento è un finding, non un FAIL; DIF-5e.0-01), onset per ruolo con il profilo congelato, LUFS, presenza per frase, diversità fra seed | M2.5 chiede tre minuti di musica continua con due sezioni distinguibili: la fixture li copre. Un brano nuovo richiederebbe di cambiare il generatore, escluso in Inc-5 (D-M2-16) | brano nuovo generato per 5e; riuso del solo `fixture_test` (non raccoglie le misure in un manifest unico) |
| 5e-02 | `full_m2_suite` | esecuzione in sequenza di tutti gli scenari M2: `tone_test`, `voice_test`, `timbre_test`, `planner_test`, `fixture_test`, `profile_test`, `onset_parity_v1`, `acao02_v1`, `evaluation_pack_v1` ed `evaluation_pack_v2` (seed passato dall'operatore), `evaluation_check_selftest_v1`, `release_ab_v1` e `procedural_v1`. Manifest aggregato con il verdetto di ognuno; uscita 0 solo se ogni verdetto vincolante è PASS e ogni misura è valida (`MEASURED`, `GENERATED`). Copertura richiesta (DIF-5e.0-03):<ul><li>i due pacchetti generati con ripetizione identica al byte;</li><li>`evaluation_check_selftest_v1`: risultati prefabbricati contro la verità sigillata di v1 e di v2, con esiti attesi fissati prima. Copre la compatibilità del checker con v1, i calcoli di `presence_stats_v1` (unità, `n_eff`, Wilson, `J_L`) e i casi `INSUFFICIENT_DATA` e `DESCRIPTIVE_ONLY`;</li><li>`release_ab_v1`: evidenze valide e riproducibili (stessi eventi fra varianti, confondenti registrati, ripetizione identica). H1, H2 e il giudizio artistico **non** sono gate.</li></ul> **Gate locale documentato**, Release, sull'hardware del titolare; mai in CI | i tempi di profiling dei runner condivisi non sono rappresentativi (D-M2-16) | gate di CI completo |
| 5e-03 | Controllo leggero in CI | nel job Release della CI Windows si aggiunge `fixture_test` (catena O3 esatta, hash PCM golden, integrità), con lo stesso schema del passo `tone_test` già presente: uscita e manifest verificati. Dalla review di 5e.1 (DIF-5e.1-01) il verdetto di `fixture_test` verifica anche il golden PCM `kFixtureProbeV1GoldenPcmHash` (15126969907236845529, seed 1337, percorso nominale; `NOT_APPLICABLE` per altri seed o con guasto iniettato), separato dai golden di trace e piano. Il valore è identico sulla macchina di sviluppo e sul runner windows-2025 (run 41501164874) e si versiona con la stessa regola degli altri golden. Il verdetto include anche il check ingegneristico del true peak già approvato in ADR-0005 D-M2-09: serve a intercettare regressioni del limiter su questa fixture deterministica, non è un gate M2.5 né un criterio di conformità (DIF-5e.0-01). Nessun nuovo gate true peak. Durata prevista: circa 10 s. Niente perf, stem o pacchetto | è il «controllo deterministico leggero» previsto da D-M2-16; `fixture_test` copre catena e PCM senza dipendere dai tempi | `procedural_v1` intero in CI (stem da 356 MB, LUFS, diversità: troppo pesante per i runner); nessun controllo |
| 5e-04 | Diversità fra seed | scansione dichiarata dei seed 1..64 (N = 64), k = 8 fingerprint distinti per le distribuzioni complete. Metriche M1–M4 del dossier d'ingresso §2.3: griglia di 1/16 di battuta per IOI e onset, Levenshtein con costo 1 per inserimento, cancellazione e sostituzione di token. Esito atteso dai dati di 5c: diversità `MEASURED` e nulla, registrata come risultato negativo; generalizzazione `INCONCLUSIVE` | il seed agisce solo sui pareggi del planner; ampliare la scansione non cambia l'esito senza modificare il generatore, escluso in Inc-5 | scansione molto più ampia (stesso esito, più costo); pesi tarati sui dati (vietato prima della misura) |
| 5e-05 | LUFS | sul PCM finale di `procedural_v1`: BS.1770-5 e EBU Tech 3341, cioè Momentary massimo, Short-term massimo, Integrated e true peak, per sezione e sul brano. Soglie di prova −23 LUFS ± 2 LU e −1 dBTP riportate come prova, non come PASS normativo. Nessun nuovo gate LUFS o true peak senza ratifica successiva (ADR-0004 §5, AUD-D-03); il limiter zero-latency stima ≤ −1 dBTP sui vettori testati e non lo garantisce per segnali arbitrari (ADR-0004 §4). Nessuna misura rolling di 30 minuti | D-M2-16 | soglie usate come gate |
| 5e-06 | Vista «a pentagramma» | PNG per frase, generato in modo deterministico, con una corsia per ruolo: intervalli pianificati, NoteOn `Applied`, onset rilevati (abbinati, non abbinati, sui rilasci). Prodotta in `procedural_v1` (5e.1) e aggiunta al pacchetto `diagnostic` a partire da `evaluation_pack_v2` (5e.2), così `evaluation_pack_v1` resta invariato e confrontabile; **mai** al `blind` | completa i manifest per l'ispezione multimodale (G6, G8) e per la revisione dell'esperto; rimandata da 5d | sola tabella JSON |
| 5e-07 | Controllo per ablazione | **rimandato a M2.5**, insieme a F5 («direttore musicale») | il confronto alla cieca fra piano completo e varianti ablate (G17) richiede varianti del generatore, escluse in Inc-5 | varianti ablate costruite a mano (non rappresentano il generatore) |
| 5e-08 | Strumento sperimentale del release (F6, R1, R2) | **richiede una decisione esplicita del titolare.** La proposta: <ul><li>un parametro di release dei NoteOff ordinari, attivo solo nei rendering offline; il default resta il declick di 2 ms, quindi audio, golden e callback invariati, e il declick di voice stealing e pausa resta di 2 ms;</li><li>uno scenario `release_ab_v1` che varia solo quel parametro (2, 15, 30, 50 e 80 ms) su set A, set B e `planner_probe_v1`;</li><li>misure: rilevazioni sui rilasci e richiamo per ruolo, e quali riferimenti della Trama restano non abbinati nel set B (verifica dell'ipotesi R2);</li><li>produce le clip A/B per la revisione dell'esperto.</li></ul> Nessun valore di release viene adottato in Inc-5: la scelta resta a M2.5 | è il confronto controllato chiesto dalla review di 5c prima di toccare l'ADSR; serve alla revisione dell'esperto a fine Inc-5 | rimandare tutto a M2.5 (revisione dell'esperto senza clip A/B); cambiare il default ora (tuning in Inc-5, escluso) |
| 5e-09 | Osservazioni aperte del checker | **OSS-5d-R3-01:** in modalità `diagnostic`, `role_planned` da `code` richiede `truth.json` fra gli input dichiarati. **OSS-5d-01:** procedura `presence_stats_v1` del [dossier R5](../research/M2_4_Inc5_Clustered_Claim_Statistics.md) §6.1. <ul><li>L'unità d'inferenza è la finestra indipendente (`test` e `altered`).</li><li>Sensibilità e specificità con Wilson al 95 % su n effettivo, assumendo ρ = 1 (risposte della stessa finestra trattate come una sola osservazione).</li><li>Le affermazioni si contano, ma senza intervallo per affermazione.</li><li>Baseline: `J_L` > 0 (somma degli estremi inferiori meno 1), con `Ĵ`, `J_L` e accuratezza bilanciata riportati.</li><li>`INSUFFICIENT_DATA` sotto la soglia minima di 5e-11.</li><li>Astensioni contate a parte, con l'analisi nel caso peggiore; negativi naturali e per rimozione riportati separatamente; concordanza dei duplicati per finestra, senza kappa con meno di 4 duplicati.</li><li>Portata dichiarata: «questa fixture, questo seed».</li></ul> | review di chiusura di 5d; dossier R5 (fonti FT: Donner & Klar 2004, Gabler et al. 1999, STARD 2015 e altre; calcoli esatti archiviati). L'enumerazione mostra che la regola attuale è valida e conservativa anche contando per affermazione, ma con intervalli troppo stretti: per la sensibilità il limite 0,904 diventa 0,701 su 9 finestre | intervallo per affermazione (precisione apparente); MOVER/Newcombe come gate (anticonservativo con 1–3 negativi); GEE o modelli misti (instabili con circa 10 cluster) |
| 5e-10 | Evidenze e handoff | <ul><li>`docs/m2_procedural_evidence.md`: manifest, misure, limiti, mappatura sui criteri M2.5;</li><li>evidenza percettiva supplementare: sessione di Antigravity qualificata «alla cieca» (seed nuovo non annotato, solo `public_blind/` fuori dal repository, checker eseguito dopo) e pacchetto per la revisione dell'esperto (WAV della fixture, viste a pentagramma, clip A/B se 5e-08 è approvata);</li><li>prompt per la review di Antigravity sulle decisioni di ADR-0006;</li><li>sezione di handoff M2.5 con le questioni F1–F6, le soglie da ratificare, i profili non validati e le ricerche aperte (R1, R3).</li></ul> | piano operativo di Inc-5 e decisioni del titolare del 2026-10-29 | — |
| 5e-11 | Soglia minima e composizione del pacchetto (**decisione del titolare**) | **Soglia:** `INSUFFICIENT_DATA` se l'n effettivo dei negativi o dei positivi è < 4 (Wilson: sotto 4 nemmeno un risultato perfetto esclude 0,5); alternativa con Clopper–Pearson: 6. **Composizione (modifica di 5d-09):** almeno 4 negativi indipendenti da frasi diverse, ottenuti anche rimuovendo un ruolo dall'audio di più finestre, bilanciati per ruolo; il pacchetto dichiara il numero di frasi e di gruppi (sezione, maschera) distinti. Per dimostrare con un risultato perfetto una specificità ≥ 0,7, 0,8 o 0,9 servono rispettivamente 9, 16 o 35 negativi indipendenti | la fixture ha negativi naturali solo nella Trama delle frasi 0–3; con 3 negativi la specificità non è riportabile (9/9 con 3/3 dà `J_L` 0,139 ma `INSUFFICIENT_DATA`) | aggiungere ruoli per finestra (non aumenta la precisione); restare a un solo item alterato |

**Precisazioni del titolare (2026-11-02), vincolanti per l'implementazione e la review di 5e:**

- **5e-08, `release_ab_v1`:**
  - *Stesse condizioni:* le varianti hanno identici seed, eventi e condizioni di rendering; cambia solo la durata del release dei NoteOff ordinari.
  - *Confondenti registrati:* sovrapposizioni di voci e voice stealing introdotti dai release più lunghi si contano e si riportano per ogni variante, ruolo e insieme, così da non confonderli con l'effetto del release.
  - *Ipotesi distinte:* la riduzione delle rilevazioni sui NoteOff (H1) e l'aumento del richiamo della Trama nel set B (H2, ipotesi R2) sono due ipotesi sperimentali separate. Ognuna ha la sua misura e il suo esito, e nessuna si dà per dimostrata prima della misura.
  - *Nessun release di produzione* viene adottato in Inc-5: la decisione resta a M2.5.
- **5e-11, soglia e composizione del pacchetto:**
  - *Riportabilità, non validazione:* 4 negativi indipendenti sono il minimo per riportare la specificità, non una sua validazione. Con 4/4 il limite inferiore di Wilson vale 0,510; con 9, 16 e 35 negativi tutti corretti vale 0,701, 0,806 e 0,901. Il report lo dichiara.
  - *Indipendenza:* frasi diverse non garantiscono l'indipendenza statistica. Finestre quasi identiche, varianti dello stesso materiale o più affermazioni sulla stessa frase non devono gonfiare l'`n_eff`. Il report dichiara le unità effettivamente indipendenti, cioè frasi e gruppi (sezione, maschera) distinti, e calcola `n_eff` su queste.
  - *Tipi di negativo:* i negativi naturali (ruolo non pianificato) e quelli per rimozione dall'audio restano distinguibili nella ground truth. Possono entrambi entrare nell'esperimento, ma si riportano anche separatamente, e il bilanciamento si controlla per ruolo, non solo nell'aggregato.
  - *Versionamento:* la modifica di 5d-09 introduce una nuova versione del protocollo del pacchetto (`evaluation_pack_v2`). `evaluation_pack_v1` resta generabile e leggibile dal checker, così le evidenze di 5d restano confrontabili.

**Precisazione del titolare (2026-11-03): livelli di CI** (precisa 5e-03 e la nota CI di D-M2-16, prima di 5e.3):

- **Motivo:** nella CI di `f649786` ([run 41599948296](https://github.com/naturewhisp/cymatica/actions/runs/41599948296)) il workflow dura 27 min 12 s. CTest impiega 77 s in Release e 1.444 s in Debug; sette test di generazione dei pacchetti di valutazione assorbono circa il 91 % del tempo Debug. Il costo è nella Debug non ottimizzata, non nell'analisi musicale, che la CI non esegue.
- **Scelta:** quattro livelli con frequenza diversa; nessun test eliminato né indebolito.
  - *CI ordinaria* (ogni push e PR): percorso Release invariato (tutti i test headless, `tone_test`, `fixture_test` con catena O3 e golden PCM). In Debug build pulita completa e test headless senza tag `[slow]` (preset `x64-debug-fast`, label escluse `device|perf|slow`).
  - *CI completa* (`windows-ci-full.yml`, ogni notte su `main` e manuale): tutti i test headless in Release e Debug, compresi gli `[slow]`.
  - *Gate locale M2* (chiusura di sottofasi e milestone): suite headless completa nelle due configurazioni e scenari CACP (`full_m2_suite`, profiling, pacchetti, `release_ab_v1`, riproducibilità), come da 5e-02.
  - *Validazione percettiva*: sessioni dedicate (5e-10).
- **Tag `[slow]`:** sui sette test individuati dalla misura (determinismo e leakage di v1, integrità della verità sigillata, gruppi e staff view di v2, compatibilità del checker con v1, rigenerazione delle directory v2, fingerprint non calcolabile). Si assegna solo a test lenti in Debug e già coperti dalla Release ordinaria; mai per escludere test instabili. Asserzioni invariate.
- **Invarianti:** `scripts/build.ps1 -Test -Headless` resta la validazione completa e non cambia comportamento; ogni test resta eseguito almeno dalla Release ordinaria; il gate di sottofase continua a richiedere la CI ordinaria verde e, alla chiusura di una milestone, anche la CI completa verde.
- **Alternative scartate:** eliminare o ridurre i test pesanti (perdita di copertura); cache del compilatore e CTest parallelo subito (seconda fase eventuale: i test condividono directory e artefatti, il parallelismo rischia interferenze); escludere la Debug dalla CI ordinaria (perde la build Debug e le asserzioni di debug ad ogni push).
- **Verifica:** conteggi dei preset (`ctest -N`), durata della CI ordinaria dopo il push, primo run verde della CI completa. Stima della CI ordinaria: 6–8 min, da misurare.

**Attuazione di 5e-08 in 5e.3 (`release_ab_v1`, 2026-11-03; regole fissate in `engine/cacp/release_ab.h` prima della misura):**

- **Variabile unica:** l'override cambia il release dei soli NoteOff ordinari.
  - 0 è il default e mantiene il declick di 2 ms: la variante 2 ms è il release di produzione, senza override.
  - Code di voice stealing, Pausa e Stop restano a 2 ms. Restano a 2 ms anche i rilasci di sicurezza di `checkLifetimes` (scadenza della durata e lease dell'open pad, DIF-5e.3-02).
  - Con un override il golden PCM della fixture è `NOT_APPLICABLE`; i golden di piano restano.
- **Isolamento offline** (DIF-5e.3-01): l'override si imposta solo con `AudioEngine::setNoteReleaseFrames`, accettato soltanto da un motore creato con `initOffline()` e senza device in esecuzione. Un motore con device fisico lo rifiuta, avviato o no. Il setter di `VoiceAllocator` è privato (`AudioEngine` è friend), quindi l'accessor modificabile non lo aggira. La regola (`noteReleaseOverrideAllowed`) è una funzione pura, verificata in CI senza hardware.
- **Stessi eventi:** per ogni insieme l'hash FNV-1a degli eventi applicati deve coincidere con quello della variante 2 ms; una variante senza riferimento a 2 ms è una violazione.
  - L'hash copre tipo, id, ruolo, frame pianificato, epoch, frame di consumo e disposizione. Lo slot è escluso perché può cambiare legittimamente con il release.
  - Con una violazione, una cattura non valida o un'analisi non valida l'esito è `INCONCLUSIVE`.
- **Confondenti:** per ogni insieme, variante e ruolo, con i totali (DIF-5e.3-03), si contano:
  - i voice drop (furto di una voce attiva);
  - i release tagliati (furto di una voce ancora in Release);
  - le sovrapposizioni (NoteOn mentre l'altra voce del ruolo è in Release).
- **Regole** (revisione DIF-5e.3-04..06, fissata prima della nuova misura):
  - **H1** è `SUPPORTED` se, in ciascuno fra set B, `planner_probe_v1` e fixture, le rilevazioni sui rilasci a 30 ms sono ≤ 20 % di quelle a 2 ms.
    - Contano solo i ruoli `MEASURED`; `NO_REFERENCES` non aggiunge nulla.
    - È `INCONCLUSIVE` con qualunque altro stato di analisi, una cattura non valida, una misura mancante o un riferimento a 2 ms nullo (fail-closed).
  - **H2** è `SUPPORTED` se il richiamo della Trama nel set B a 30 ms è ≥ 0,90 **e** maggiore di quello a 2 ms, entrambi `MEASURED` su catture valide. Senza entrambe le misure è `INCONCLUSIVE`.
  - **R2, sotto-ipotesi short-IOI** (diagnostica a 2 ms): `SUPPORTED` se i riferimenti della Trama non abbinati nel set B sono esattamente le 10 note precedute dagli intervalli di 0,18 e 0,22 s. Verifica solo questa sotto-ipotesi della spiegazione di 5c.
  - **R2, spiegazione causale:** resta `INCONCLUSIVE` nello scenario; l'indagine (velocity, timbro, distanze dai NoteOff, normalizzazione del flusso) è la micro-fase 5e.3b.
- **Esito:** `MEASURED`, mai un gate.
- **Ripetizione:** confronta il JSON delle misure, compreso l'hash del PCM finale della fixture per variante. Le clip derivano da quel PCM.
- **Clip A/B:** frasi 5 e 10 della fixture per ogni variante, dal PCM finale, con dissolvenze di 10 ms, per la revisione dell'esperto.
- **Nessun release di produzione è adottato in Inc-5:** la decisione resta a M2.5. Risultati in `progress.md` (5e.3).

**5e.3b — diagnostica causale dei 10 attacchi mancati della Trama (autorizzata il 2026-11-03; previsioni registrate prima di qualunque misura della fase):**

- **Perimetro:** analisi offline del set sintetico B, ruolo Trama, nei rendering a 2 ms (produzione) e 30 ms (controllo) di `release_ab_v1`. Restano invariati:
  - il DSP e il release di produzione;
  - `descriptors_v1` (STFT, flusso, soglia, picchi);
  - il profilo congelato `stem_onset_profile_v1` (Trama δ = 0,20, offset +9,96 ms).

  Nessun tuning. L'esito è `MEASURED` e non è un gate.
- **Modello del rivelatore** (D-M2-14):
  - n[k] = F[k] / max_j F[k], dove F è il flusso spettrale grezzo di log(1 + 100|X|);
  - soglia θ[k] = media mobile centrata di n su ±100 ms + δ;
  - un picco è rilevato se è un massimo locale con n[k] > θ[k].

  In unità grezze la soglia vale media(F) + δ·max F. Il termine δ·max F dipende da **tutto** il segnale, quindi un transitorio forte in un punto qualunque alza la soglia degli attacchi ovunque.
- **Ipotesi di meccanismo (M-NORM):** a 2 ms i transitori dei rilasci della Trama fissano il massimo globale del flusso. Gli attacchi più deboli restano così sotto δ·max F; a 30 ms i transitori spariscono, il massimo scende e gli stessi attacchi superano la soglia.
- **Provenienza dell'ipotesi (dichiarata):** nasce dall'osservazione a posteriori dei dati di 5e.3. Fra le 10 non abbinate, 9 hanno velocity ≤ 0,7 (atteso 20 su 40) e 8 timbro ≤ 0,3 (atteso 16 su 40); la prima nota, mancata, non ha un NoteOff precedente. Queste associazioni **non** sono una verifica. Le previsioni sotto riguardano grandezze nuove (flusso grezzo, normalizzatore, soglia, ricalcolo controfattuale), non ancora misurate.
- **Grandezze per riferimento** (finestra di abbinamento ±50 ms dopo la correzione dell'offset):
  - picco di n e di F nella finestra, soglia θ e margine n − θ al picco, condizione di massimo locale;
  - distanza dal NoteOff precedente, velocity, timbro e intervallo precedente;
  - per il rendering: normalizzatore max F e posizione del massimo (entro ±50 ms da un NoteOff della Trama, cioè su un rilascio, oppure no).
- **Previsioni e regole** (fissate ora):
  - **P1, normalizzatore dominato dai rilasci:** `SUPPORTED` se a 2 ms il massimo globale di F cade entro ±50 ms da un NoteOff della Trama **e** max F(2 ms) ≥ 1,2 × max F(30 ms).
  - **P2, controfattuale di normalizzazione:** a 2 ms si ricalcola n dividendo F per il suo massimo fuori dalle finestre di ±50 ms attorno ai NoteOff della Trama. Restano invariati δ, offset, finestra della media, distanza minima e abbinamento. `SUPPORTED` se il richiamo della Trama vale ≥ 0,95 (≥ 38 su 40). *Controllo:* lo stesso ricalcolo a 30 ms deve mantenere il richiamo a 1,000; altrimenti P2 è `INCONCLUSIVE`.
  - **P3, attacchi deboli:** `SUPPORTED` se almeno 8 dei 10 riferimenti mancati a 2 ms stanno fra i 12 con il picco grezzo di F più basso dei 40.
  - **P4, fusione per distanza minima** (meccanismo alternativo): `NOT_SUPPORTED` se nessun riferimento mancato ha nella sua finestra un massimo locale sopra soglia scartato dalla fusione a 30 ms; `SUPPORTED` se lo hanno almeno 5 dei 10; `INCONCLUSIVE` da 1 a 4.
  - *Validità:* la prova richiede catture e analisi valide, un riferimento per nota e un richiamo ricostruito passo per passo identico a quello di `evaluateRoleOnsets`. Con 2 ms e 30 ms rieseguiti, la ripetizione dev'essere identica. P3 è fissata per 10 mancati: un numero diverso la rende `INCONCLUSIVE`.
  - **Esito causale:**
    - `SUPPORTED` per M-NORM se P1, P2 e P3 sono `SUPPORTED` e P4 no;
    - `NOT_SUPPORTED` se P2 è `NOT_SUPPORTED` con controllo valido;
    - altrimenti `INCONCLUSIVE`, con le previsioni riportate singolarmente.
- **Conseguenze ammesse:** solo documentali (dossier di calibrazione, punti aperti per M2.5 e per il consolidamento del modello matematico). Un'eventuale modifica del rivelatore (per esempio una normalizzazione locale) è una decisione di M2.5 con una nuova versione di `descriptors_v1`.
- **Esito (2026-11-03, `trama_miss_diagnosis_v1`, `MEASURED`):** P1, P2 e P3 sono `SUPPORTED`, P4 è `NOT_SUPPORTED`; l'esito causale M-NORM è **`SUPPORTED`**. I dati sono in `progress.md` (5e.3b). Restano aperti per M2.5 e per il modello matematico:
  - la dipendenza delle misure di onset dal contenuto dell'intero stem;
  - il δ della Trama, calibrato in presenza dei transitori di rilascio;
  - la scelta fra normalizzazione globale e locale in un futuro `descriptors_v2`.

**Precisazioni della review di 5e.0 (`d08187d`, DIF-5e.0-02, vincolanti per 5e.2; fissate prima della misura):**

- **Regola deterministica di raggruppamento.** L'unità d'indipendenza è il **gruppo musicale**, non la finestra.
  - Due finestre appartengono allo stesso gruppo se provengono dalla stessa frase sorgente, incluse le varianti per rimozione di ruoli e il duplicato.
  - Vale lo stesso se le loro frasi sorgente hanno lo stesso fingerprint di contenuto per frase. È il «fingerprint parziale per frase» del dossier d'ingresso §2.3: i campi di D-M2-12, con `sampleFrame` relativo all'inizio della frase e tutti i ruoli. **Non esiste ancora nel codice** e si implementa in 5e.2, con un test positivo (frasi con lo stesso materiale in punti diversi del brano → stesso fingerprint) e uno negativo (un solo evento diverso → fingerprint diverso). Le ripetizioni identiche dello stesso materiale, frequenti nella fixture, contano quindi una volta.
  - La regola si calcola dal piano, prima di qualunque affermazione, ed è registrata nella verità sigillata come `group_id` per finestra.
- **`n_eff` per classe.** Per ogni classe (positivi e negativi) si calcola `n_eff = (Σ_c m_c)² / Σ_c m_c²` sui gruppi c, con m_c = numero di affermazioni deduplicate della classe nel gruppo. Si assume ρ = 1 dentro il gruppo e indipendenza fra gruppi.
  - Quattro finestre negative non garantiscono `n_eff ≥ 4`. Esempio della review: gruppi con 4, 1, 1, 1 affermazioni danno `n_eff` = 2,58.
- **Conteggi riportati, per classe:** affermazioni, finestre, frasi sorgente, gruppi musicali e `n_eff`, separando i negativi naturali da quelli per rimozione e riportando il bilanciamento per ruolo.
- **Dimensionamento di `evaluation_pack_v2`:**
  - il pacchetto si costruisce perché, con un'affermazione per finestra e ruolo, l'`n_eff` atteso dei negativi sia ≥ 4: almeno 4 gruppi musicali con negativi, e lo stesso numero di negativi per gruppo;
  - il pacchetto dichiara l'`n_eff` atteso nella verità sigillata;
  - se la fixture non offre abbastanza gruppi distinti, la costruzione lo dichiara (`expected_n_eff` < 4) invece di gonfiare il conteggio;
  - l'`n_eff` effettivo del risultato lo calcola il checker sulle affermazioni ricevute.
- **Interpretazione degli intervalli:**
  - gli intervalli di Wilson sono sempre etichettati `coverage: model_based_unverified`: sono condizionati al modello dichiarato e non sono una copertura frequentista verificata con dati dipendenti;
  - se l'indipendenza fra gruppi non è sostenibile (meno di 4 gruppi musicali distinti per classe), lo stato è `DESCRIPTIVE_ONLY`: si riportano solo conteggi e tassi descrittivi, senza intervalli né `above_trivial_baseline`;
  - `INSUFFICIENT_DATA` resta per `n_eff` < 4 con almeno 4 gruppi.

**Precisazioni della review di 5e.1 (`0e367a4`, DIF-5e.1-02/03):**

- **Stati delle misure.** Lo stato di ogni misura di `procedural_v1` (true peak, loudness del brano e per sezione, onset, presenza) deriva dal risultato effettivo:
  - `MEASURED` solo con valori validi;
  - `INCONCLUSIVE` con il motivo, e valori `null`, quando l'analisi è stata eseguita ma non ha prodotto valori;
  - `NOT_RUN` quando non è stata eseguita.

  Lo stato complessivo è `MEASURED` solo se lo sono tutte le misure. Il verdetto tecnico PASS/FAIL resta indipendente.
- **Coerenza fra stato e valori** (review mirata di 5e.1, DIF-5e.1-R2-01):
  - `MEASURED` richiede valori obbligatori finiti: Integrated, i massimi Momentary e Short-term quando presenti, true peak, precisione e richiamo degli onset;
  - i valori di una misura che non è `MEASURED` si serializzano `null`, mai come avanzi numerici;
  - il true peak di ogni sezione è un valore opzionale con un suo stato, indipendente dalla validità della loudness della sezione;
  - per gli onset, lo stato effettivo di ogni ruolo (`roleOnsetMeasureStatus`) governa sia l'aggregato sia la serializzazione del ruolo (DIF-5e.1-R3-01):
    - `MEASURED` solo con precisione e richiamo finiti;
    - `NO_REFERENCES` e `NOT_RUN` restano tali;
    - ogni altro caso è `INCONCLUSIVE`, con il motivo, `analysis_status` e metriche `null`;
    - gli altri ruoli conservano le loro misure valide.
- **Evidenza vincolante nel manifest.** Il blocco `binding_evidence` copia da `fixture_probe_v1`, senza rieseguirlo:
  - ogni check vincolante, distinguendo PASS, FAIL e `NOT_APPLICABLE`;
  - hash e golden;
  - riconciliazione O3 con i conteggi;
  - contabilità dello scheduling, ack di trasporto e conteggi del segnale;
  - il check ingegneristico del true peak, marcato come non vincolante in `procedural_v1`.

**Precisazioni d'implementazione di 5e.2** (2026-11-02, da confermare nella review di 5e.2):

- **Fingerprint per frase:** è il fingerprint di D-M2-12 con l'origine all'inizio della frase. Unisce solo i NoteOff legati ai NoteOn della frase.
- **Regola dei gruppi:** stessa frase sorgente oppure stesso fingerprint per frase. Con una sorgente senza piano, compresa la verità v1, il gruppo è la frase sorgente (`source_phrase_only`).
- **Composizione di `evaluation_pack_v2`:**
  - si riserva un gruppo per i riferimenti in ogni sezione che ha almeno due gruppi, scelto fra quelli con tutti i ruoli presenti, così ogni sezione conserva almeno una finestra di prova;
  - una finestra di prova per ogni gruppo restante;
  - una finestra di rimozione per gruppo, presa da un'altra frase del gruppo quando c'è, che rimuove il ruolo presente con meno rimozioni fino a quel momento; mai una finestra vuota;
  - un duplicato e un catch trial come in v1.
- **Sulla fixture:**
  - 14 gruppi, di cui 3 riservati, quindi 11 finestre di prova e 11 di rimozione;
  - rimozioni per ruolo 3/3/3/2;
  - `n_eff` atteso 10,9 per i positivi e 8,9 per i negativi.

  I negativi naturali stanno in un solo gruppo e restano `DESCRIPTIVE_ONLY`.
- **Stato complessivo della presenza:** `MEASURED` solo con entrambe le classi `MEASURED`. Altrimenti, in ordine:
  - `EXCLUDED_BY_CATCH_RULE`;
  - `NOT_RUN`, senza affermazioni;
  - `DESCRIPTIVE_ONLY`, se una classe è descrittiva o vuota;
  - `INSUFFICIENT_DATA`.

  Il tasso descrittivo resta sempre riportato, mentre intervalli, `J_L` e baseline compaiono solo con `MEASURED`. Le astensioni si contano a parte, con i tassi nel caso peggiore.
- **Compatibilità:** `evaluation_pack_v1` resta identico al byte. È verificato sul pacchetto reale con il seed 2718281828: `SHA256SUMS` identici per la parte pubblica blind, la diagnostic e la sigillata, prima e dopo 5e.2. Il checker legge le verità v1 e v2.
- **OSS-5d-R3-01:** nel pacchetto `diagnostic`, `role_planned` da `code` richiede `truth.json` fra gli input dichiarati; altrimenti è `not_attributable`.
- **Review di 5e.2 (`631258b`, DIF-5e.2-01..03, OSS-5e.2-01):**
  - **Directory per versione:** si sostituisce solo una directory integra della *stessa* versione del pacchetto, letta dal `pack.json` o dall'`evaluation_truth.json` da scrivere; una directory v1 non è mai sostituita da v2, né il contrario. Lo scenario controlla tutte e tre le directory prima di scriverne qualunque, così un rifiuto non lascia mai parte pubblica e verità sigillata di esecuzioni diverse.
  - **M3:** usa l'intersezione di multinsiemi (note simultanee alla stessa altezza contate con la loro molteplicità), sempre in [0, 1].
  - **Fingerprint non calcolabile:** con il piano disponibile la costruzione di v2 si interrompe con un errore (fail-closed); mai un gruppo a sé che conterebbe come indipendente. Il fallback `source_phrase_only` resta solo per le sorgenti senza piano (compatibilità con v1).
  - **Parametri statistici nel report:** z, ρ ipotizzato dentro il gruppo, soglie minime di gruppi e `n_eff`, regola dell'intervallo, finestre del campione e finestre con tutte le affermazioni corrette.
- **Diversità** (5e-04): sui seed 1..64 esiste un solo contenuto (`NULL_DIVERSITY`), e la generalizzazione della calibrazione è `INCONCLUSIVE`. M2–M4 si calcolano solo sulle coppie con contenuto diverso; le coppie identiche sono zero per identità. La misura è `MEASURED` e non rende rosso alcuno scenario.

**Sottofasi**, ognuna con build Release, Debug e Headless (0 warning, 100 % dei test), scenari interessati e stop per la review:

- **5e.1:** `procedural_v1`, LUFS, vista a pentagramma, controllo di CI;
- **5e.2:** diversità fra seed, `presence_stats_v1`, `evaluation_pack_v2` con la nuova composizione (5e-11) e osservazioni del checker;
- **5e.3:** `release_ab_v1` (5e-08, approvata);
- **5e.4:** `full_m2_suite`, dossier delle evidenze, materiali per le sessioni percettive e handoff M2.5.

## 3. Conseguenze

- **Fasi:** ogni decisione indica la fase che la implementa. Le fasi si eseguono una alla volta, con review obbligatoria del titolare prima del commit e dell'autorizzazione della fase successiva ([progress.md](../progress.md#m24-inc-5--piano-a-fasi-e-revisione-del-piano-2026-10-26)).
- **Interfacce interne nuove:** sink degli eventi applicati e degli stem, fingerprint del contenuto, target `cymatica_cacp`, scenari CLI. Nessuna ABI pubblica cambia.
- **Dipendenze:** il target `cymatica_cacp` usa nlohmann/json, già fissata in `docs/dependencies.md` per `cymatica_replay`. In 5b il target non ne ha ancora bisogno e non la linka: l'estensione d'uso si registra in quel file nella fase 5d, con il checker dei risultati v0. Nessuna nuova dipendenza.
- **Ricerca e documentazione:**
  - il dossier sui metodi CACP §9 e agent_control_observability §12 rimandano a questo ADR;
  - ADR-0005 (D-M2-11 e nota multi-seed) rimanda a D-M2-12, D-M2-15 e D-M2-16.
- **Criteri di verifica dell'ADR:** ogni decisione ha un test o un campo di manifest nella fase indicata. La review di ciascuna fase e la review indipendente di chiusura M2.4 verificano la corrispondenza.

## 4. Riferimenti

Fonti nuove, consultate il 2026-10-26. Le fonti già citate dal [dossier sui metodi CACP](../research/M2_4_CACP_Evaluation_Methods.md) §3.1 (Bello et al. 2005; mir_eval; librosa; MMAU; HALL; SPEC-FS; SPEC-NO; documentazione Gemini) valgono con il livello di verifica riportato lì. Le fonti sugli approfondimenti (diversità, separazione della ground truth, onset per ruolo) sono nel [dossier di ricerca d'ingresso](../research/M2_4_Inc5_Entry_Research.md).

- **ITU-R BS.1770-5:** «Algorithms to measure audio programme loudness and true-peak audio level», Rec. ITU-R BS.1770-5 (11/2023), in vigore. [Pagina ITU](https://www.itu.int/rec/R-REC-BS.1770).
  - *Verifica:* testo completo letto dal PDF ufficiale.
  - *Usati:* Tabelle 1 e 2 (coefficienti a 48 kHz), costante −0,691, blocchi da 400 ms con sovrapposizione del 75 %, gate assoluto −70 LKFS, gate relativo −10, scarto del blocco incompleto.
- **EBU Tech 3341:** «Loudness Metering: "EBU Mode" metering to supplement EBU R 128 loudness normalization», versione 4.0, novembre 2023. [Pagina EBU](https://tech.ebu.ch/publications/tech3341).
  - *Verifica:* testo completo letto.
  - *Usati:* finestre rettangolari scorrevoli di 0,4 s (Momentary) e 3 s (Short-term); tabella dei segnali di test, casi 1–5, 9 e 12.
- **EBU Tech 3342:** «Loudness Range: A measure to supplement EBU R 128 loudness normalization», versione 4, novembre 2023. [Pagina EBU](https://tech.ebu.ch/publications/tech3342).
  - *Verifica:* pagina e PDF letti.
  - *Uso:* solo riferimento. LRA non è misurato in Inc-5.
- **FIPS 180-4:** NIST, «Secure Hash Standard (SHS)», FIPS PUB 180-4, agosto 2015, [DOI 10.6028/NIST.FIPS.180-4](https://doi.org/10.6028/NIST.FIPS.180-4).
  - *Verifica:* testo completo letto (blocchi da 512 bit per SHA-256).
  - *Vettore «abc»:* letto da NIST, «SHA-256 example», [CSRC](https://csrc.nist.gov/projects/cryptographic-standards-and-guidelines/example-values).
- **RFC 1950:** P. Deutsch, J-L. Gailly, «ZLIB Compressed Data Format Specification version 3.3», maggio 1996. [RFC Editor](https://www.rfc-editor.org/rfc/rfc1950).
  - *Verifica:* testo completo letto.
  - *Usati:* CMF/FLG con `CMF·256 + FLG` multiplo di 31; Adler-32 modulo 65521; trailer big-endian.
- **RFC 1951:** P. Deutsch, «DEFLATE Compressed Data Format Specification version 1.3», maggio 1996. [RFC Editor](https://www.rfc-editor.org/rfc/rfc1951).
  - *Verifica:* testo completo letto.
  - *Usati:* `BTYPE = 00` «no compression»; LEN/NLEN; blocchi non compressi limitati a 65.535 byte.
- **PNG:** W3C, «Portable Network Graphics (PNG) Specification (Third Edition)», Recommendation, 24 giugno 2025. [W3C](https://www.w3.org/TR/png-3/).
  - *Verifica:* intestazione, stato e indice letti.
  - *Verificati sul testo HTML completo il 2026-10-27 (fase 5b):* firma `89 50 4E 47 0D 0A 1A 0A`; layout dei chunk con CRC su tipo e dati; CRC con polinomiale di ISO 3309 / ITU-T V.42, registro inizializzato a 1 e complemento finale; campi di IHDR e tipo colore 2 «Truecolor» con profondità 8 o 16; «Only filter method 0 is defined», filtro 0 «None», byte di filtro per riga; compressione 0 con datastream zlib e blocchi «raw (uncompressed)»; confini fra IDAT arbitrari; IEND con dati vuoti.
  - Lo stato del documento precisa che non è ancora uno standard internazionale; il rapporto con ISO/IEC 15948 non è verificato.
