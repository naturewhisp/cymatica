# ADR-0005 — Decisioni d'ingresso M2.4 Inc-4 (fixture A–B–A′, orizzonte di pianificazione, profiling realtime)

- **Data:** 2026-10-20
- **Stato:** `accepted for implementation` (decisioni d'ingresso di Inc-4). Nessun valore qui riportato è `validated`: i parametri sono profili iniziali versionati da verificare durante Inc-4/M2.5.
- **Milestone:** M2 — sottofase M2.4, incremento Inc-4.
- **Owner:** coordinatore dello sviluppo M2.4, nell'ambito autorizzato dal titolare il 2026-10-20 («chiudi tutti i punti aperti per iniziare lo sviluppo di Inc-4»). Le scelte sono di misura e di struttura e non cambiano scope, ABI o gate. La ratifica esplicita del titolare è richiesta prima della chiusura di M2.4, in particolare per la statistica di PASS di D-M2-10.
- **Gerarchia:** Specifica > DESIGN > AGENTS > ADR > piano operativo > ricerca. Questo ADR **precisa** D-M2-05 e D-M2-07 di [ADR-0003](0003-m2-decisions-contracts.md) senza modificarne gli invarianti; non altera ABI pubbliche (`MusicIntentEvent`, `ScheduledAudioEvent`, `AudioCommand`), il budget di 8 voci, la roadmap §32 o i gate M2.5.
- **Origine:** revisione tecnico-scientifica indipendente post-Inc-3 del 2026-10-20 (registro rilievi e remediation in [progress.md](../progress.md), sezione «Revisione tecnico-scientifica post-Inc-3»).
- **Perimetro:** sono chiuse **solo** le scelte da cui dipende il codice di Inc-4. Restano esplicitamente differiti: tuning artistico e ascolto umano (M2.5/M2-F), LUFS (Inc-5, solo esplorativo), sessione rolling 30 min e H.872 (M5+), corsa su device con conteggio underrun (M2.5), semantica musica→geometria (M3+).

## 1. Contesto

Inc-3 ha consegnato pipeline gerarchica, candidate search bounded e probe `planner_probe_v1` su 8 battute. La revisione del 2026-10-20 ha rilevato che (a) l'espansione della forma di 96 battute in frasi, (b) l'orizzonte di pianificazione su 183 s con cambi di epoch, (c) la metodologia di misura del target `≤ 2,0 ms` e (d) i criteri di verdetto di `fixture_probe_v1` non erano definiti in alcun record normativo, mentre progress.md e la skill CACP citavano alias CLI e test non esistenti. Senza queste scelte Inc-4 dovrebbe inventare decisioni permanenti durante l'implementazione, contro `AGENTS.md` §1.1.

## 2. Decisioni

### D-M2-08 — Espansione della fixture `fixture-m2-4-v1` in frasi (precisa D-M2-05)

- **Scelta:** funzione pura del Coordinator `FormPlan × MusicClock → PhrasePlan[24]`: 3 sezioni × 8 frasi × 4 battute (`barsPerPhrase = 4`). Ogni `startFrame`/`endFrame` è `clock.frameAtBar(...)`; ogni onset interno è `clock.frameAtBeatStep(...)` (nessun frame derivato da divisioni locali). Confini attesi: sezione B a 3.072.000, sezione A′ a 5.705.142, fine contenuto a 8.777.142 frame (8.777.142 / 48.000 = 182,857125 s; durata musicale esatta 182,857143 s troncata al frame).
- **Armonia (tabella deterministica versionata, nessuna estrazione casuale aggiuntiva):** un accordo per battuta, ciclo di frase Stabilità → Preparazione → Tensione → Risoluzione.
  - A e A′ (Re dorico): gradi 0–3–4–0 (i–IV–v–i; il IV maggiore è il colore dorico).
  - B (Sol eolio): gradi 0–5–6–0 (i–VI–VII–i).
  - Ultima frase di A′: obiettivo `ModalResolution` con accordo finale di tonica sostenuto e rilascio esplicito del planner prima dello `Stop`.
- **Funzione delle sezioni (maschera di ruoli per frase):** A frasi 0–3 senza Trama, frasi 4–7 con Trama (contrappunto nella seconda metà); B tutti i ruoli, trasformazioni di Vettore limitate a `ModalTranspose` e `ContourInversion`; A′ ripresentazione di `Motif_0` con sole trasformazioni che preservano le note ancora.
- **Tensione obiettivo (profilo iniziale):** A 0,20; B 0,60; A′ 0,35. Il factory `makeFixtureM24V1Form()` usa oggi 0,20 / 0,50 / 0,70: i valori di B e A′ sono da allineare in Inc-4 (A′ più tesa di B contraddice la forma «contrasto → ritorno»).
- **Pad cross-sezione (unico uso di pad aperti nella fixture):** le frasi ordinarie realizzano un accordo per battuta con note di Corpo di una battuta (nessun pad aperto). Solo nell'ultima frase di A e di B:
  - battute 0–2 come le frasi ordinarie; nella battuta 3 (Risoluzione, tonica i) Corpo emette sul downbeat un pad aperto di 2 voci (fondamentale e quinta della tonica uscente, `durationFrames = 0`);
  - il `NoteOff` del planner cade a `frameAtBeat(S·beatsPerBar + 1)`, dove `S` è la battuta d'inizio della sezione successiva (32 o 64): un beat dentro la nuova sezione, con l'epoch della frase che lo emette (quella vecchia). È ammesso perché il motore onora gli eventi con epoch ≥ `minValidEpoch` (D-M2-04 §6);
  - la prima frase della sezione successiva inizia le note di Corpo al beat 1 della battuta 0, cioè allo stesso frame del `NoteOff` (ordinamento `NoteOff` prima di `NoteOn`, Spec §7.5). Per il beat di sovrapposizione: nel passaggio A→B il pad Re–La sta sotto Sol minore (Re è quinta, La è nona); nel passaggio B→A′ il pad Sol–Re sta sotto Re minore (Re è fondamentale, Sol è quarta sospesa). Entrambe le dissonanze durano un beat e si sciolgono al beat 1;
  - **emendamento al vincolo hard di Inc-3:** gli eventi restano in `[startFrame, endFrame)`; fa eccezione al più un `NoteOff` di pad per voce di Corpo, marcato come cross-sezione, entro `endFrame + 1 beat` del tempo della sezione successiva. Il tracker simulato della frase successiva parte con quelle due voci occupate fino a quel frame. Il lease è ampiamente sotto il bound fail-closed di D-M2-04 §4 (5 beat, meno di 2,5 s, contro 72 s).
- **Verifica:** test golden dei 25 confini di frase calcolati con oracolo razionale indipendente; pad attivo al frame di transizione; `orphanNoteRecoveredCount == 0`; nessun furto Regola 2 su Corpo ai confini; trace simbolico identico a parità di seed/policy.
- **Motivazione:** riusa il planner Inc-3 senza nuove fonti casuali e rende verificabile la continuità cross-epoch richiesta da D-M2-05 con il solo budget di 2 voci di Corpo.
- **Alternative scartate:** progressioni estratte dallo stream `Harmony` (rinviate: richiedono criteri di valutazione M2-F); forma calcolata in un solo passaggio di 96 battute (incompatibile con l'orizzonte mutabile di D-M2-04 §6); pad per tutta l'ultima frase (sopprimerebbe la progressione armonica della cadenza); pad che oltrepassa la prima battuta della nuova sezione (occuperebbe Corpo durante il nuovo accordo e forzerebbe furti).

### D-M2-09 — Orizzonte di pianificazione, epoch e invalidazione (precisa D-M2-04 §6)

- **Pianificazione just-in-time:** la frase N+1 è pianificata quando il cursore di pianificazione entra nella frase N (anticipo minimo 1 frase, ≈ 6,857 s a 140 BPM, molto superiore al lookahead). Lo staging di `AudioScheduler` è il *Mutable Planning Horizon* del Coordinator; solo il contenuto della SPSC è *Committed Horizon*. Lookahead nominale 7.200 frame (150 ms), dichiarato nel manifest.
- **Cambi di sezione:** il Coordinator invia `SetTempoMap` almeno una battuta prima, con `targetSampleFrame = clock.frameAtBar(barConfine)` esplicito (mai il default 0, che verrebbe quantizzato), e mantiene un clock di pianificazione con gli stessi segmenti. Gli eventi della nuova sezione portano l'epoch attesa (corrente + 1). L'ack deve risultare `Applied` con `sampleFrame` uguale al confine pianificato e l'epoch attesa; un ack `Rejected` o fuori frame è un errore fail-closed: il Coordinator revoca gli eventi in staging dal confine in poi e ripianifica. Nella probe offline Inc-4 lo stesso caso produce `FAIL`.
- **Revoca:** Inc-4 introduce in `AudioScheduler` una revoca dal frame F che rimuove i `NoteOn` in staging con frame ≥ F e conserva i `NoteOff` delle note già consegnate alla SPSC, così da non creare note orfane. Su `Stop` il Coordinator svuota lo staging e azzera il planner (tracker simulato incluso).
- **Verifica:** ack ed epoch controllati ai frame 3.072.000 e 5.705.142; test unitario della revoca (NoteOff conservati, nessuna nota appesa); `droppedScheduledEvents == 0` e `deferredScheduledEvents == 0` sulla fixture.
- **Motivazione:** il just-in-time mantiene la ripianificazione possibile (gameplay futuro, cambi di tempo) senza toccare eventi già garantiti; l'ack come conferma evita che un `SetTempoMap` tardivo ammutolisca un'intera sezione, perché gli eventi con epoch futura non raggiunta vengono scartati.
- **Alternative scartate:** pianificare tutta la forma in anticipo (nessuna revocabilità); inviare `SetTempoMap` nello stesso blocco del confine, come fa la probe Inc-3 (funziona solo offline e fallisce in modo silenzioso su device); epoch dedotte invece che confermate (si desincronizzano con Pause, Resume e Stop).

### D-M2-10 — Metodologia di profiling realtime (precisa D-M2-07)

- **Punto di misura:** wrapper unico di `AudioEngine::Impl::processBlock`, condiviso da callback device e rendering offline (`initOffline` esegue lo stesso codice di render). Il tempo misurato va dall'ingresso all'uscita e include comandi, eventi, voci, bus, limiter terminale, tap `AudioProbe` e pubblicazione della telemetria. Orologio monotono `std::chrono::steady_clock` (QPC su MSVC).
- **Raccolta nel thread audio:** istogramma preallocato posseduto dal thread audio (bin da 10 µs fino a 5 ms più bin di overflow), conteggio, somma, massimo, superamenti di `2,0 ms × frameCount / 512` e del periodo del blocco. Nessun I/O, nessuna allocazione, nessun lock. Lettura dopo quiescenza (offline) oppure tramite telemetria (`max`, `last`, superamenti). I percentili sono stimati dall'istogramma e riportati come limite superiore del bin.
- **Carichi del benchmark Inc-4 (Release, offline, blocchi da 512 frame):**
  - **W:** stress worst-case con 8 voci attive su tutti i ruoli (`timbreParam > 0`), un furto per blocco, delay di Trama attivo e limiter in riduzione di guadagno;
  - **W** dura almeno 10.000 blocchi misurati, così p99.9 poggia su almeno 10 campioni di coda;
  - **F:** fixture completa, 8.777.142 frame più coda (circa 17.150 blocchi).
  - Si scartano 64 blocchi di warm-up, si eseguono 3 ripetizioni, e ogni carico gira con CACP ON (probe collegata e drenata a ogni blocco) e OFF (probe scollegata). Il limiter è sempre attivo.
- **Criterio PASS/FAIL (solo Release, macchina e profilo documentati):** per W e F, CACP ON e OFF, in ogni ripetizione:
  - p99.9 ≤ 2,0 ms (normalizzato a 512 frame);
  - nessun blocco oltre il periodo del blocco (10,667 ms a 512/48 kHz), cioè nessuna violazione che su device sarebbe un underrun certo.
  
  Se una ripetizione fallisce **solo** sul secondo criterio, è ammessa una sola nuova esecuzione e si registrano entrambi i risultati. p50/p95/p99 e il massimo sono sempre riportati. In Debug i valori sono solo riportati.
- **Motivazione:** su un thread offline non realtime il massimo include la preemption dell'OS e non è un worst case garantito (piano M2.4 §5); un gate sul massimo renderebbe il verdetto instabile, mentre un percentile alto con un limite assoluto al periodo del blocco intercetta sia il costo sistematico sia gli stalli gravi. Questa interpretazione di «deve operare entro ≤ 2,0 ms» (D-M2-07 §1) è una scelta di misura e non un declassamento del target: la verifica su device resta a M2.5.
- **Come W raggiunge la riduzione di guadagno (precisazione DIF-INC4-R2-01, 2026-10-21, decisa dal titolare):**
  - **Problema:** con il gain staging dei bus di Inc-2 (−6/−8/−10/−6 dB, `kNominalVoiceScale = 0,25`, velocity ≤ 1) le sole 8 voci di W restano sotto la soglia del limiter (−1,80 dBFS): −7,13 dBFS di picco assoluto all'attacco simultaneo del blocco 0 (5,3 dB sotto soglia) e −8,54 dBFS a regime nei blocchi misurati (6,7 dB sotto soglia). Il limiter resta quindi a 0 dB di riduzione, anche con le voci all'unisono.
  - **Soluzione:** W avvia il trasporto con il tono diagnostico di M2.2 a ampiezza `kProfileWorkloadWToneVolume = 0,8` (`Start.paramF32`). Il tono è sommato alle voci prima del limiter, nello stesso percorso di render di produzione, e produce circa 3,1 dB di riduzione sostenuta.
  - **Condizione vincolante:** `max_gain_reduction_db > 0` fa parte di `stress_conditions_met` di W, insieme a 8 voci attive e a ≥ 1 furto per blocco misurato.
  - **Motivazione (misure in [M2_4_DIF-INC4-R2-01_Limiter_GR_Analysis.md](../research/M2_4_DIF-INC4-R2-01_Limiter_GR_Analysis.md)):** il criterio resta quello originale e non viene indebolito. W diventa più pesante (≈ +5 % di costo medio) ed esercita il ramo più costoso del limiter (riduzione sostenuta, +14 % rispetto a sotto soglia), come raccomandato per i carichi di stress misurati [Wilhelm et al. 2008]. La riduzione da sola costa ≈ 0,9 µs per blocco (0,045 % del budget), molto sotto il rumore delle code.
  - **Alternative scartate:** emendare D-M2-10 togliendo la riduzione (lascia un ramo RT non esercitato); aumentare il carico musicale (non fattibile: −7,13 dBFS assoluti, −8,54 dBFS a regime); un pre-gain diagnostico nel motore (nuova API e un ramo nel percorso RT).
  - **Vincolo:** se il tono diagnostico venisse rimosso o limitato, W va ridefinito prima della modifica.
  - **Conformità alle regole:** questo è l'unico carico in cui il tono può suonare insieme alle voci polifoniche, secondo l'eccezione di [`.agents/rules/realtime_and_serialization.md`](../../.agents/rules/realtime_and_serialization.md) §4, precisata il 2026-10-21 su decisione del titolare.
- **Limite dichiarato (precisazione R6-03, 2026-10-20):** il gate statistico offline di Inc-4 misura il costo del render su Windows; **non** equivale alla garanzia che ogni callback termini entro 2,0 ms né alla qualificazione realtime. Quest'ultima resta un gate distinto di M2.5 su device (periodo effettivo, MMCSS, underrun). I due esiti si registrano separatamente e il primo non si presenta come prova del secondo.
- **Alternative scartate:** gate sul massimo (instabile con la preemption); p99 (troppo permissivo sugli eventi rari, come i furti multipli); zero blocchi oltre 2,0 ms (equivale al gate sul massimo); misura solo su device in Inc-4 (non riproducibile in CI e senza controllo del carico).
- **Ambiente nel manifest:** commit, configurazione, CPU, versione dell'OS, versione MSVC, dimensione del blocco e sample rate.
- **Test:** `tests/unit/test_realtime_profile.cpp` con label CTest `perf`. Le asserzioni temporali valgono solo con `NDEBUG`. La label è esclusa dai preset CI (runner condivisi non rappresentativi) e il risultato è riportato a parte.
- **Fuori da Inc-4:** corsa su device WASAPI ≥ 180 s con periodo effettivo, MMCSS e underrun, assegnata a M2.5. FTZ/DAZ per thread resta un'opzione da valutare su evidenza (rischio di divergenza dei golden offline).

### D-M2-11 — Scenari CACP di Inc-4, verdetti e artefatti

- **Scenari:** `fixture_probe_v1` (alias `fixture_test`) per contenuto e segnale; `profile_probe_v1` (alias `profile_test`) per i tempi D-M2-10. I due verdetti restano separati, così un'anomalia temporale non maschera né sostituisce un difetto di contenuto. Un nome di scenario sconosciuto termina con errore.
- **Check vincolanti di `fixture_probe_v1`:**
  - frame catturati = contenuto + coda di rilascio, `droppedFrames == 0`, nessuna discontinuità di cattura (la probe va drenata fino a vuoto a ogni blocco);
  - ack `Applied` dei due `SetTempoMap` ai frame esatti, con sequenza di epoch 1 → 2 → 3;
  - `droppedScheduledEvents == 0`, `deferredScheduledEvents == 0`, `orphanNoteRecoveredCount == 0`, ogni `NoteOn` con il suo `NoteOff`; `voiceDropCount` riportato;
  - campioni finiti, |x| ≤ 0,89125, `clipped == 0`, `dspAnomalyCount == 0` (limiter incluso);
  - non-silenzio per sezione e per ruolo dalla telemetria di bus; silenzio numerico dopo lo `Stop`;
  - trace simbolico (eventi + decisioni) identico fra due esecuzioni e uguale all'hash golden di regressione versionato.
- **Riportati senza gate M2.5:** true peak stimato 4× (BS.1770 Annex 2) — resta check ingegneristico della probe, coerente con AUD-D-03 —, gain reduction massima, conteggio dei furti, statistiche di energia per ruolo. LUFS non fa parte di Inc-4.
- **Motivazione e alternative:** scenari separati, perché un unico verdetto misto farebbe dipendere la correttezza del contenuto dal rumore temporale della macchina (scartato lo scenario unico con alias multipli previsto in origine). Il true peak non è un gate M2.5, coerentemente con AUD-D-03, ma resta un check della probe per intercettare regressioni del limiter (scartata la rimozione completa).
- **Artefatti:** WAV stereo 16 bit di circa 35,1 MB in `artifacts/cacp/` (escluso da git), con hash del contenuto registrato nel manifest e nell'evidenza. L'eventuale giudizio percettivo dell'agente è supplementare, con modello, capacità, hash e timestamp, oppure `not_run`/`unsupported`; non sostituisce l'ascolto umano di M2.5.
- **Metodi di valutazione dell'agente (precisazione del 2026-10-21, decisione del titolare; analisi in [M2_4_CACP_Evaluation_Methods.md](../research/M2_4_CACP_Evaluation_Methods.md)):**
  - **Gerarchia:** i gate si basano solo su test, descrittori quantitativi e telemetria del motore. L'ingestione audio e la lettura di spettrogrammi da parte di un agente restano supplementari e non decidono mai un verdetto.
  - **Capacità dichiarate:** ogni giudizio dell'agente registra le capacità del canale usato (canali, risoluzione, token/s). Con l'API Gemini l'audio è mono a 16 Kbps, 32 token/s.
  - **Affermazioni non ammesse:** sullo stereo se il canale è mono; su fenomeni sotto la risoluzione temporale del canale (~31 ms con 32 token/s); da spettrogrammi letti senza un pacchetto di riferimenti etichettati.
  - **Riferimento obbligatorio:** ogni affermazione indica la finestra (frame o battute) e il metodo (ascolto, spettrogramma, descrittore, codice). Le affermazioni ricavate da codice o documenti non si presentano come percezione.
  - **Controlli di falsificazione:** un giudizio vale come evidenza supplementare solo se supera i controlli contro i dati dello scenario (es. ruoli effettivamente presenti nella finestra secondo gli intent). Da Inc-5 i controlli sono automatici, nel pacchetto `evaluation_pack_v1` e nel risultato strutturato v0.
  - **Riferimenti bibliografici:** le fonti sono nel §3.1 del dossier.

### Nota — riproducibilità e diversità (non è una decisione di Inc-4)

Nel planner Inc-3 i candidati derivano da una tabella fissa di ricette e il seed agisce solo sul tie-break fra punteggi pari. Quindi seed diversi non producono necessariamente composizioni diverse. È accettabile come baseline deterministica, ma non dimostra la varietà richiesta alla musica procedurale. Inc-4 non cambia il generatore. In Inc-5 (dossier M2.4-F) si aggiunge una valutazione multi-seed che misura separatamente la riproducibilità (stesso seed → stesso trace) e la diversità (seed diversi → distanza fra trace), secondo il [protocollo sperimentale](../research/M2_4_Evaluation_Protocol.md). I risultati sono esplorativi e non costituiscono un gate di M2.5. Eventuali sorgenti di variazione guidate dal seed (stream `Harmony`/`Rhythm`) richiedono una decisione successiva basata su quelle misure.

## 3. Conseguenze

- Inc-4 ha i contratti necessari per fixture, streaming, profiling e probe. Le API introdotte (espansore di frasi, revoca dello scheduler, wrapper di profiling, scenari CLI) sono interne e non toccano ABI pubbliche.
- Si aggiornano nella stessa modifica: [ADR-0003](0003-m2-decisions-contracts.md) (rimandi da D-M2-05/D-M2-07), [progress.md](../progress.md) (piano Inc-4), [piano M2.4](../research/M2_4_Implementation_Plan.md) e la [skill CACP](../../.agents/skills/cacp-testing/SKILL.md), che ora riportano gli scenari Inc-4 come *pianificati*.
- **Criteri di verifica dell'ADR:** ogni check di D-M2-08..11 ha un test o un campo di manifest corrispondente in Inc-4; la review indipendente di chiusura M2.4 verifica la corrispondenza.

## 4. Riferimenti

Fonti citate dalla precisazione DIF-INC4-R2-01 di D-M2-10, consultate il 2026-10-21. L'analisi completa, con le misure, i dati grezzi e il sorgente dell'esperimento, è in [M2_4_DIF-INC4-R2-01_Limiter_GR_Analysis.md](../research/M2_4_DIF-INC4-R2-01_Limiter_GR_Analysis.md).

- R. Wilhelm et al., «The Worst-Case Execution-Time Problem — Overview of Methods and Survey of Tools», *ACM Transactions on Embedded Computing Systems* 7(3), art. 36, 2008. [DOI 10.1145/1347375.1347389](https://doi.org/10.1145/1347375.1347389).
- D. Giannoulis, M. Massberg, J. D. Reiss, «Digital Dynamic Range Compressor Design — A Tutorial and Analysis», *Journal of the Audio Engineering Society* 60(6), 399–408, 2012. [AES E-Library](https://aes2.org/publications/elibrary-page/?id=16354).
- T. Hoefler, R. Belli, «Scientific Benchmarking of Parallel Computing Systems: Twelve Ways to Tell the Masses when Reporting Performance Results», *SC15*, 73:1–73:12, ACM, 2015. [Pagina dell'autore](https://www.unixer.de/publications/index.php?pub=222).
- J. Chen, J. Revels, «Robust benchmarking in noisy environments», arXiv:1608.04295, 2016. [arXiv](https://arxiv.org/abs/1608.04295).
- M. Wittmann, T. Zeiser, G. Hager, G. Wellein, «Short Note on Costs of Floating Point Operations on current x86-64 Architectures: Denormals, Overflow, Underflow, and Division by Zero», arXiv:1506.03997, 2015. [arXiv](https://arxiv.org/abs/1506.03997).
- R. Bencina, «Real-time audio programming 101: time waits for nothing», 2011, rossbencina.com. Segnalazione: [LWN](https://lwn.net/Articles/453005/).
