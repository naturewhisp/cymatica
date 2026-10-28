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
