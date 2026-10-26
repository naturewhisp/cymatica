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
   - *Contenuto:* solo ciò che determina la musica. Per ogni NoteOn conta la tupla (ruolo, `sampleFrame`, `durationFrames`, altezza, velocity, `timbreParam`). Per ogni NoteOff del planner conta la coppia (ruolo, `sampleFrame`).
   - *Quantizzazione* (prima della serializzazione, per non far dipendere l'identità da differenze float non udibili):
     - altezza → intero in centesimi di semitono relativi a 440 Hz, cioè `round(1200·log2(f/440))`;
     - velocity e `timbreParam` → `round(x·1000)`.
   - *Ordine canonico:* (`sampleFrame`, ruolo, altezza quantizzata, `durationFrames`), indipendente dall'ordine di emissione e dagli `eventId`.
   - *Esclusioni:* seed, `eventId`, `tick`, epoch, `decisionId`, candidati, punteggi, tie-break e ogni altro metadato che può variare senza cambiare la musica.
   - *Valori speciali:* un'altezza 0 o non applicabile usa una sentinella esplicita; lo zero negativo vale 0; un valore non finito è un errore e non produce fingerprint (come nella canonicalizzazione di RFC 8785).
   - *Serializzazione:* intestazione con versione del formato, sample rate e numero di eventi; record a larghezza fissa little-endian; hash FNV-1a a 64 bit, come `canonicalPlanHash`. Il numero di eventi viene confrontato insieme all'hash. Con le poche decine di seed previste la probabilità di collisione è trascurabile (dossier di ricerca §2.3).
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

1. `hash(offline) == hash(JIT) == golden`;
2. eventi JIT generati = eventi inviati alla SPSC = eventi consumati dal motore = `Applied` + `Ignored` + `Dropped`;
3. ogni NoteOn degli intent JIT ha esattamente un record `Applied`, con stesso `eventId`, ruolo e `sampleFrame`, e frame consumato uguale al frame pianificato;
4. ogni NoteOff pianificato ha un record `Applied` o un `Ignored` atteso e giustificato;
5. nessun record `Applied` senza intent corrispondente;
6. nessuno scarto è contato come applicazione. Nella fixture nominale `Dropped` = 0;
7. nei casi di arresto d'emergenza (D-INC4-R3-01) gli scarti attesi sono quelli già classificati come invalidati dal probe, e il controllo li riconcilia uno a uno.

**Verifica (5a):**

- test di fault injection:
  - un evento perso fra scheduler e SPSC;
  - un evento in ritardo, che diventa `Dropped`;
  - un NoteOff non corrispondente, che diventa `Ignored`;
- test dell'overflow con capacità ridotta;
- test del fingerprint: due piani che differiscono solo per seed ed `eventId` producono lo stesso fingerprint e hash di provenienza diversi; una sola nota diversa cambia il fingerprint;
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

**Verifica (5b):**

- FFT confrontata con una DFT diretta;
- vettori di riferimento per CRC-32, Adler-32 e SHA-256: `SHA-256("abc") = ba7816bf…f20015ad`, dall'esempio NIST;
- PNG riletto e controllato chunk per chunk;
- onset su impulsi sintetici in frame noti;
- Side/Mid su L = R e su L = −R;
- casi LUFS della tabella;
- parità con E1 del dossier CACP sul WAV della fixture, documentata ma non usata come gate.

### D-M2-15 — `evaluation_pack_v1`, separazione della ground truth e `evaluation_result_v0`

*Rilievo: INC5-REV-03. Fase: 5d.*

Il pacchetto prodotto da `--cacp-run evaluation_pack_v1` ha due parti fisicamente separate.

| `public/` — consegnata al valutatore | `sealed/` — riservata al checker |
|---|---|
| WAV e PNG mel Mid/Side per ciascuna finestra di prova, con ID opachi e ordine mescolato da un seed dichiarato | ruoli presenti per finestra, ricavati dagli intent |
| intervalli temporali (battute e frame) e metadati tecnici non rivelatori (sample rate, durata, versione dei descrittori) | intent, epoch ed eventi applicati delle finestre |
| riferimenti etichettati autorizzati: spettrogrammi degli stem per ruolo e sezione, tratti **da finestre diverse** da quelle di prova | chiave fra ID opachi e finestre |
| `SHA256SUMS` dei file pubblici | regole di verifica, risultati attesi, `SHA256SUMS` dei file sigillati |

**Selezione delle finestre:**

- unità di una frase, cioè 4 battute;
- per ogni sezione almeno una finestra di prova con un ruolo presente e una di controllo con lo stesso ruolo assente, quando la maschera lo consente. Esempio: Trama assente nelle frasi 0–3 di A e presente nelle frasi 4–7;
- i controlli aggiuntivi si fissano nella review di 5d, sulla base del dossier di ricerca d'ingresso §3.3 e §6:
  - negativi bilanciati, duplicati nascosti, catch trial, item a verità alterata, esito «non determinabile»;
  - registro degli accessi, pulizia dei metadati, confronto con una baseline banale;
  - controllo di discriminazione per ablazione (piano completo contro varianti ablate, come in MetaCompose);
  - vista «a pentagramma» con intent, eventi `Applied` e onset per ruolo, sul modello delle debug UI di Hi-Fi Rush.

**Protocollo di separazione:**

- **Consegna:** il valutatore riceve soltanto `public/`, copiato in una directory separata o allegato al client.
- **Collocazione di `sealed/`:** viene scritto in un percorso scelto da chi lancia lo scenario. Per default sta fuori da `artifacts/cacp/`, così non si trova accanto ai file pubblici.
- **Hash degli input:** il risultato v0 dichiara gli hash dei file effettivamente ricevuti. Il checker rifiuta un risultato i cui input includono file di `sealed/` o file sconosciuti.
- **Limite dichiarato:** un agente con accesso al repository può rieseguire il planner e ricostruire la ground truth. La separazione è quindi procedurale, non crittografica. Ogni risultato v0 registra gli strumenti e gli accessi del valutatore (filesystem, esecuzione di codice); un valutatore con accesso al repository non produce evidenza percettiva «alla cieca».

**`evaluation_result_v0`** (JSON, sottoinsieme della proposta D-AO-04 di agent_control_observability §13.2):

- versione dello schema;
- valutatore: tipo, modello e versione;
- capacità dichiarate del canale: canali, sample rate o bitrate, token/s, risoluzione delle immagini;
- strumenti e accessi;
- hash degli input ricevuti;
- timestamp;
- lista di affermazioni `{window_id, method ∈ {listening, spectrogram, descriptor, code}, kind, role?, text}`.

**Esiti del checker** (`--cacp-check-evaluation`), per affermazione:

| Esito | Significato |
|---|---|
| `supported` | coerente con `sealed/` |
| `contradicted` | incoerente con `sealed/` (es. ruolo dichiarato presente ma assente negli intent della finestra) |
| `not_attributable` | il metodo dichiarato non può produrla: stereo da un canale mono; fenomeno sotto la risoluzione temporale del canale (~31 ms a 32 token/s); affermazione da `code` presentata come percezione |
| `inadmissible` | violazione delle regole di D-M2-11: spettrogramma letto senza riferimenti, input da `sealed/`, finestra non indicata |
| `unverifiable` | testo libero senza controparte verificabile |

L'esito aggregato è riportato separatamente dai gate e non decide mai un verdetto (D-M2-11).

**Verifica (5d):**

- due run nella stessa build producono gli stessi hash dei file pubblici;
- test del checker con affermazioni prefabbricate, almeno una per esito;
- test di leakage: nessun file di `public/` contiene ruoli attesi, intent, chiave o identificatori che permettano di risalire alla finestra (scansione dei campi e dei nomi dei file);
- un risultato che dichiara un input sigillato produce `inadmissible`.

### D-M2-16 — Calibrazione (R6), AC-AO-02 proposto, multi-seed, LUFS e CI

*Fasi: 5c e 5e.*

**Calibrazione e verifica dei parametri dei descrittori (R6):**

- *Latenza intrinseca:* prima della calibrazione si misura la latenza del rilevatore su segnali sintetici per ruolo con onset noti al campione, generati dal synth reale, e la si confronta con la convenzione temporale del frame.
- *Insieme di calibrazione:* fixture `fixture-m2-4-v1` con seed 1337. Su questo si fissano δ e correzione dello scarto per ruolo, ed eventuali varianti di D-M2-14. Il profilo si congela con versione e hash prima di eseguire la verifica. Il seed 1337 non è mai evidenza d'accettazione.
- *Insieme di verifica*, mai usato per scegliere i parametri:
  1. seed della fixture con fingerprint di contenuto (D-M2-12) diverso da quello del seed 1337, presi da una scansione dichiarata (insieme di seed e criterio fissati prima della misura);
  2. contenuto di `planner_probe_v1`;
  3. segnali sintetici per ruolo con tempi, densità e altezze diversi dalla fixture, generati in modo deterministico nei test.
- *Esiti:* se la scansione non trova seed con contenuto diverso, la generalizzazione multi-seed è `INCONCLUSIVE`, con la misura di diversità allegata. Gli insiemi 2 e 3 restano validi. Se le metriche di verifica peggiorano oltre il margine dichiarato in 5c, i parametri non vengono ritoccati: si registra il risultato e si riapre la decisione (regola §6.9).

**AC-AO-02 in Inc-5:**

- **Catena O3:** controllo esatto di D-M2-12, PASS/FAIL, vincolante per gli scenari.
- **Onset per ruolo:** precisione, recall e F per ruolo sugli stem, con la ground truth dei NoteOn `Applied` del ruolo, misurati e riportati `MEASURED`. Le soglie per ruolo sono **proposte** in 5c, ciascuna con il margine osservato sull'insieme di verifica, e si ratificano al gate M2.5.
- Il master resta misurato per confronto con E1. Non è un criterio per ruolo, perché la Trama non è osservabile dal master.
- **Intent non è percezione:** questi controlli provano che un evento previsto è arrivato al segnale, non che sia udibile (dossier CACP §8).

**Multi-seed** (esplorativo, non è un gate):

- *Riproducibilità:* stesso seed, due pianificazioni → stesso hash di provenienza e stesso fingerprint.
- *Diversità:* fra seed diversi si confrontano il fingerprint, per i duplicati esatti, e le metriche M1–M4 sulla sequenza di eventi (dossier di ricerca d'ingresso §2.3). L'analisi segue l'*expressive range* della generazione procedurale: descrittori fissati prima, insieme di seed dichiarato, distribuzioni invece di un solo numero. N, k, griglia e pesi si fissano nella review di 5e, prima della misura.
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
- **Dipendenze:** il target `cymatica_cacp` usa nlohmann/json, già fissata in `docs/dependencies.md` per `cymatica_replay`. L'estensione d'uso si registra in quel file nella fase 5b. Nessuna nuova dipendenza.
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
  - *Riportati da un estratto della pagina e da verificare sul testo in 5b:* metodo di compressione 0 = deflate; byte di filtro per riga; algoritmo CRC del §5.5.
  - Lo stato del documento precisa che non è ancora uno standard internazionale; il rapporto con ISO/IEC 15948 non è verificato.
