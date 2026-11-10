# CYMATICA — revisione d'insieme e consolidamento dell'analisi (2026-11-09)

- **Data:** 2026-11-09
- **Origine:** richiesta del titolare: rivedere e consolidare l'analisi di tutto il progetto, non solo di M2.5, correggere le parti errate e pianificare le ricerche mancanti.
- **Commit esaminato:** `72a1c8e` (ultimo su `main`), con M2.5.1 in corso.
- **Status:** documento di revisione, **non normativo**. Non modifica specifica, DESIGN, AGENTS, ADR, contratti né golden. Il codice cambia solo per CONS-01, su decisione del titolare del 2026-11-10 (§5.1). I rilievi aperti e le ricerche nuove sono **proposte** per il titolare.
- **Materiale:** script e output in [consolidation_2026-11-09](experiments/consolidation_2026-11-09/README.md).

## Punto di ripartenza

**Lo sviluppo di M2.5 riparte da questo documento.** Lo stato autorevole resta in [progress.md](../progress.md) e il piano in [ADR-0007](../adr/0007-m2-5-entry-decisions.md); qui c'è l'ordine dei lavori che ne deriva al 2026-11-10. Chi riprende il lavoro legge questa sezione, poi il §3.1 e il §5.

1. **Riprovare il loopback sul device.** `cymatica_game --cacp-run device_probe_v1 --device-usage games --device-capture-pcm --device-loopback`, a macchina silenziosa, poi `analyze_loopback.py`. Lo spostamento di CONS-01 non è stato riprovato dal vivo (§5.1).
2. **M2.5.1c**, ultima parte di M2.5.1 (ADR-0007 §3 e §8):
   - loudness per stem;
   - viste a pentagramma leggibili (P-M25-08);
   - strumenti offline d'insieme e di sviluppo proposti da R-M25-7: fixture avverse, nullo per permutazione, variogramma;
   - pulizie: «Â§» nel manifest, OSS-5e.3b-R2-01, OSS-M2.5.1d-R4-01, decisione su `/WX`, OSS-M2.4-CL-02.
3. **Preparare CP-1** con ciò che oggi manca (§3.1):
   - prova del volume sull'uscita USB, con previsione registrata prima (§6.2, R-M25-12);
   - parte anticipata di R-M25-3 su LFO, saturazione e supersaw (§6.3, CONS-08);
   - testo del protocollo d'ascolto P-M25-03;
   - monitor `IAudioClock` nel probe e un ascolto umano controllato sul device (R-M25-8);
   - se la scelta del default di MMCSS dipende da misure, il confronto controbilanciato di R-M25-10 (§6.1).
4. **CP-1**, decisioni del titolare: cambio di metro, fra le opzioni A e B del [dossier](M2_5_1b_Meter_Feasibility.md); criterio PASS del device; canale ETW; rapporto FM e proposte dell'esperto in M2.5.2; inserimento di R-M25-10..12 in ADR-0007.
5. Solo dopo CP-1: **M2.5.2**, mix e immagine stereo.

Il lavoro di questa revisione (documenti del 2026-11-09, CONS-01 e registrazione della review di M2.5.1b del 2026-11-10) è committato su indicazione del titolare **senza review indipendente**: va riesaminato alla prossima review, secondo ADR-0007 §6.

## 1. Esito in breve

L'analisi regge. Le misure archiviate si riproducono, i link interni sono integri e le citazioni controllate esistono. I problemi trovati sono di tre tipi:

1. **Documenti di stato rimasti indietro.** Il README, l'indice della ricerca e le righe di stato di `progress.md` descrivevano una situazione superata. Corretti in questa revisione (§4).
2. **Due divergenze da regole del progetto, mai registrate.** La cattura loopback apre un secondo device miniaudio, contro la formulazione dell'inventario delle dipendenze: risolta il 2026-11-10 con un target diagnostico separato (§5.1). M2.5.1b risultava committata senza una review registrata: la review esisteva ed è stata registrata il 2026-11-10 (§5).
3. **Due conclusioni di M2.5.1d più deboli di quanto sembrino.** La prova di carico non può, per costruzione, mettere in difficoltà il thread audio. L'attenuazione di −59,5 dB sull'uscita USB ha una spiegazione verificabile con una prova breve (§6).

Nessun rilievo tocca DSP, planner, golden o contratti realtime. La sola modifica al codice è lo spostamento di CONS-01.

## 2. Cosa è stato verificato, e come

| Verifica | Esito | Evidenza |
|---|---|---|
| Script di misura archiviati, rieseguiti e confrontati con gli output salvati | 11 su 11 identici: dieci di `m25_ensemble_baseline` e le statistiche di `inc5_claim_stats` | Comandi nel [README](experiments/consolidation_2026-11-09/README.md) |
| Controlli negativi degli strumenti su device | 15/15, 20/20 e 15/15, come dichiarato | `test_series_tools.py`, `test_load_series.py`, `test_analyze_loopback.py` |
| Riassunto della serie su device | Coincide con `summary.txt` | `summarize.py` |
| Link interni di 77 documenti | 1.058 link relativi, 0 rotti. 311 link assoluti `file:///` in due documenti di Antigravity (§5, CONS-06) | `linkcheck.output.txt` |
| Identificativi bibliografici | 122 DOI e 27 arXiv. Tutti risolti con autori e titolo coerenti con la citazione, tranne 11 DOI che Crossref non copre (9 DOI arXiv su DataCite, 2 Zenodo): questi non sono verificati | `doicheck.output.txt`, `doicheck_parens.output.txt` |
| Riferimenti al codice nei dossier di M2.5 | Confermati: `music_planner.cpp:617-618`, `audio_probe.cpp:1586`, `voice_allocator.cpp:597`, `music_planner.h:193`, `test_music_planner.cpp:820` e `:824` | lettura diretta a `72a1c8e` |
| Aritmetica dei dossier, ricalcolata a mano | Coerente: durate della fixture (182,857 s), frame della prova di metro, 87 stalli (47 con silenzio, 40 senza), regola del tre, potenza binomiale, scarti del rapporto FM (0,12·c), +1,75 LU | §2.1 |
| CI su `72a1c8e` | `success`, [run 42120050419](https://github.com/naturewhisp/cymatica/actions/runs/42120050419) e quattro corse notturne complete | `gh run list` |

**Cosa non è stato verificato.**
- Nella revisione del 2026-11-09 build e test non sono stati rieseguiti, perché non toccava il codice. Sono stati eseguiti il 2026-11-10 per CONS-01 (§5.1).
- Il controllo bibliografico prova che la fonte esiste e che autori e titolo corrispondono. Non prova che i valori citati siano nel testo: per quello vale il livello dichiarato in ogni dossier (FT, FT-parz, AM, SEC).
- `role_levels.py` ed `expert_analysis.py` non sono stati rieseguiti, perché leggono artefatti non versionati.
- I documenti di M2.4 sono stati controllati per coerenza incrociata e citazioni, non riletti riga per riga.
- Nessun ascolto: l'agente non ascolta.

### 2.1 Controlli incrociati fra documenti

| Grandezza | Dove compare | Esito |
|---|---|---|
| 2.784 eventi, 1.392 note | dossier delle evidenze, baseline su device | Coerenti: 416 + 96 × 2 + 400 + 384 = 1.392, perché Corpo suona diadi |
| Kick 7–9 dB sotto Corpo e Vettore | ascolto, proposta M2.5, ADR-0007 | Coerente con Pulso −10,6…−12,4 dB e Corpo, Vettore circa −3 dB rispetto al mix |
| `n_eff` 4,93 e 3,77; 14 e 8 gruppi | indice, dossier delle evidenze, progress | Coerenti |
| Golden PCM 15126969907236845529 | tutti i documenti di M2.5 | Coerente |
| `residual_db` −179,18 dB del loopback | dossier R-M25-8 e README della baseline | Il dossier lo riporta come misura; il README chiarisce che è il pavimento numerico della prima versione dello script e che il residuo è zero. **Fa fede il README** |

## 3. Stato consolidato del progetto

| Milestone | Stato | Fonte |
|---|---|---|
| M0, M1 | Complete | [progress.md](../progress.md) |
| M2.1–M2.3 | Complete | idem |
| M2.4 | Chiusa il 2026-11-03, review di chiusura OK in due cicli | idem |
| M2.5.0 | Chiusa (`f4c4723`) | [ADR-0007](../adr/0007-m2-5-entry-decisions.md) |
| M2.5.1a | Chiusa (`3a30d8b`) | progress.md |
| M2.5.1d | Chiusa (`38b9a93`), con OSS-M2.5.1d-R4-01 aperta | idem |
| M2.5.1b | Chiusa (`72a1c8e`), review OK registrata il 2026-11-10 | idem |
| M2.5.1c | Non iniziata: loudness per stem, viste leggibili, strumenti offline, pulizie | ADR-0007 §3 |
| CP-1 | Non tenuto | ADR-0007 §4 |
| M2.5.2–M2.5.6 | Non autorizzate | idem |
| M2 | **Non conclusa** | specifica §32 |

Nessun profilo sperimentale è `validated`. La diversità fra seed è nulla per costruzione (Teorema D). L'unico contenuto musicale esistente è la fixture del seed 1337.

### 3.1 Prontezza di CP-1

| Domanda di CP-1 | Evidenza disponibile | Cosa manca |
|---|---|---|
| Serve il cambio di metro? | [Fattibilità](M2_5_1b_Meter_Feasibility.md): tecnicamente alla portata, manca la gerarchia metrica | Scelta fra le opzioni A e B del dossier (OSS-M2.5.1b-01). Nessun ascolto del 3/4 |
| Criterio PASS del device | [Dossier R-M25-8](M2_5_1_Windows_Audio_Glitch_Detection.md), [baseline](experiments/m25_device_baseline/README.md) | Monitor `IAudioClock` non integrato nel probe. Ascolto umano controllato mai eseguito. Stress significativo (§6.1) |
| Canale ETW | Analisi dei 24 eventi | Decisione del titolare |
| Margine DSP per gli effetti | Callback p99,9 fra 0,12 e 0,28 ms su periodo di 10 ms | Costo degli effetti: R-M25-3, che oggi è dopo CP-1 |
| Protocollo d'ascolto | Proposte in R-M25-7 (controlli B0, costrutti separati, Q3 per sezione) | Testo del protocollo P-M25-03 |
| Rapporto FM in M2.5.2 | Modello di Sethares in R-M25-7 | Nessun ascolto; decisione |
| LFO, saturazione, supersaw, voci libere | Solo i suggerimenti dell'esperto | Nessuna analisi di costo, aliasing e contratto delle voci (§6.3) |
| Loudness per stem | — | M2.5.1c |

CP-1 è quindi raggiungibile, ma tre voci arrivano senza evidenza propria: ascolto controllato sul device, protocollo d'ascolto scritto, effetti proposti dall'esperto.

## 4. Correzioni apportate in questa revisione

| File | Correzione | Motivo |
|---|---|---|
| [progress.md](../progress.md) | Righe di stato allineate ai fatti: M2.5.1 autorizzata, 1a e 1d chiuse, 1b committata con CI verde. Riga contraddittoria della tabella di M2.5 riscritta. Errata sulle date | La riga in testa diceva ancora «M2.5.1 attende l'autorizzazione»; la tabella dava M2.5.1d insieme «in attesa» e «chiusa»; M2.5.1b risultava «non committata» |
| [README dell'indice di ricerca](README.md) | Titolo e stato estesi a M2.5; aggiunte le voci mancanti; ADR-0006 non più «in review» | Mancavano la fattibilità del metro, la baseline su device e questo documento |
| [README del progetto](../../README.md) | «Stato reale», tabella dei documenti e «Prossimo lavoro» aggiornati | Indicava Inc-4 come prossimo lavoro e ometteva ADR-0006 e ADR-0007 |
| [Inventario delle dipendenze](../dependencies.md) | Aggiunta la sezione sugli strumenti offline di ricerca | Python e numpy erano usati senza essere censiti (AGENTS §2) |
| [Dossier R-M25-7](M2_5_F5_Music_Director_Research.md) | Tre «DOI non verificato» sostituiti con l'esito del controllo | Povel e Essens 1985, Harrison e Pearce 2020 e Bharucha 1984 si risolvono su Crossref con metadati coerenti |

**Errata sulle date.** In `progress.md` sei voci di «Ultimo aggiornamento» portano la data 2026-11-07 o 2026-11-06, e il titolo del passo 1 di M2.5.1d dice 2026-11-06. Tutti i commit relativi sono del 2026-11-05 (`c545724` alle 19:44, `72a1c8e` alle 23:15). Le date del registro storico restano come scritte, con l'errata in testa.

## 5. Rilievi aperti

Gravità: **MED** richiede una decisione o una correzione prima della fase indicata; **LOW** è da sistemare quando si tocca il file.

| ID | Gravità | Rilievo | Proposta | Scadenza proposta |
|---|---|---|---|---|
| CONS-01 | MED, **risolto il 2026-11-10** (§5.1) | **Secondo device miniaudio.** `engine/audio/loopback_capture.cpp:48` apre un device di tipo loopback. L'[inventario](../dependencies.md) dice che solo `audio_engine.cpp` può aprire il device, e il criterio di M0 chiede «un solo device aperto nel binario finale». `MINIAUDIO_IMPLEMENTATION` resta unico e il file è nello stesso target, quindi la specifica §28.2 è rispettata alla lettera. La cattura è diagnostica e parte solo in `device_probe_v1`. La divergenza dall'inventario non è registrata in nessun documento | Scegliere: **(a)** registrare un'eccezione esplicita per un device di sola cattura, diagnostico, mai attivo nel runtime, con un test che lo dimostri; **(b)** spostare `LoopbackCapture` in un target diagnostico separato da `cymatica_audio`. La (b) è più pulita e costa un target CMake | CP-1 |
| CONS-02 | MED, **chiuso il 2026-11-10** | **M2.5.1b committata senza review registrata.** ADR-0007 §6 chiede review indipendente e approvazione prima del commit. `72a1c8e` contiene M2.5.1b; `progress.md` non riporta né review né approvazione | La review esisteva: verdetto OK su `72a1c8e`, consegnata e registrata in [progress.md](../progress.md) il 2026-11-10. La sua OSS-M2.5.1b-01 è integrata nel [dossier del metro](M2_5_1b_Meter_Feasibility.md) | Fatto |
| CONS-03 | MED | **La prova di carico non discrimina** (§6.1) | Ricerca R-M25-10 | Prima di M2.5.6 |
| CONS-04 | LOW | **Attenuazione USB non spiegata** (OSS-M2.5.1d-02, §6.2) | Prova breve con il volume registrato nel manifest | M2.5.1c o CP-1 |
| CONS-05 | LOW, **chiuso il 2026-11-10** | **AGENTS.md** dichiarava «Updated 2026-10-13», ma il §5.3 è del 2026-11-04 | Data aggiornata insieme al rimando al punto di ripartenza, su richiesta del titolare | Fatto |
| CONS-06 | LOW | **311 link assoluti `file:///c:/Users/…`** in `M2_4_Inc5_Math_Model_Antigravity.md` (289) e `m2_4_closure_review_r2.md` (22). Funzionano solo su questa macchina e mostrano un percorso locale. Sono testi consegnati da Antigravity, archiviati come ricevuti | Lasciarli invariati con una nota in testa, oppure convertirli in relativi dichiarando l'intervento | Decisione del titolare |
| CONS-07 | LOW | **Specifica:** l'intestazione dice 0.8.3 del 2026-10-07, il file è stato modificato fino al 2026-10-19; il changelog contiene una frase superata sul gate M1 | Riallineare alla prossima revisione della specifica, prevista entro CP-4 per il §32 (R-M25-7) | CP-4 |
| CONS-08 | LOW | **R-M25-3 è collocata dopo CP-1**, ma CP-1 deve decidere se LFO, saturazione e supersaw entrano in M2.5.2 | Anticipare a prima di CP-1 la sola parte di R-M25-3 su costo e aliasing di queste tre proposte (§6.3) | Prima di CP-1 |
| CONS-09 | LOW | Nota di ricerca `regole_insieme.md:523`: un articolo di *Music & Science* del 2026 è dato come «di argomento non determinato» | Crossref lo identifica: Gómez-Marín, Clark, Jordà, Herrera, «Tapping into Rhythm: How Polyphonic Drum Patterns Map to Monophonic Contours and Support Pattern Retrieval». Non letto. Pertinente per F1 e per R-MET | Con R-M25-11 |

### 5.1 CONS-01: risoluzione (2026-11-10)

Il titolare ha scelto l'opzione (b), target separato.

- `LoopbackCapture` è ora in `engine/audio_diag/`, nel target `cymatica_audio_diag`, che collega `cymatica_audio` e usa le sole dichiarazioni di miniaudio.
- `cymatica_audio` contiene solo l'interfaccia `DeviceLoopbackTap` (`engine/audio/device_loopback_tap.h`). `runDeviceProbeV1` riceve un puntatore al tap; `nullptr` significa nessun loopback.
- Solo `cymatica_game` collega il target diagnostico, e costruisce il device soltanto con `--device-loopback`. I test non lo collegano.
- Nuovo test CTest `single_audio_owner` ([script](../../tests/check_single_audio_owner.cmake)): fallisce se un file diverso da `audio_engine.cpp` e dalla cattura diagnostica include miniaudio o apre un device, se `MINIAUDIO_IMPLEMENTATION` non è definito esattamente una volta, o se `cymatica_audio` torna a citare la cattura. Controlli negativi eseguiti a mano: un `ma_device_init` aggiunto a `music_clock.cpp` e una radice sbagliata danno entrambi esito negativo.
- L'[inventario](../dependencies.md) registra l'eccezione nella scheda di miniaudio.
- Comportamento, DSP, planner, golden e formato del manifest sono invariati. Esiti di build e test in [progress.md](../progress.md).

**Limite.** Il percorso `--device-loopback` richiede un device e non è coperto dai test automatici: va rieseguito sul device prima di CP-1.

## 6. Integrazioni tecnico-scientifiche

Ogni punto separa ciò che è **verificato** sul repository da ciò che è **inferenza** di questa revisione. Le inferenze sono proposte da provare, non risultati.

### 6.1 Il carico al 100 % di CPU non può affamare il thread audio

**Verificato.**
- Il motore non imposta la priorità del thread audio. miniaudio usa `ma_thread_priority_default`, che vale `ma_thread_priority_highest` (`miniaudio.h:4452-4463`) e diventa `THREAD_PRIORITY_HIGHEST` (`miniaudio.h:17873`).
- `load_series.py` avvia un processo per core logico «at normal priority» (riga 4 dello script).

**Inferenza.** Lo scheduler di Windows è a priorità: un thread a priorità più alta sottrae la CPU a quelli a priorità normale. Con o senza MMCSS, i processi di carico non possono ritardare la callback. L'esito «nessun degrado» e «beneficio di MMCSS non riprodotto» è quindi quello atteso dal disegno della prova. Dice che il carico di calcolo ordinario non disturba, non che la catena regga le cause tipiche dei glitch. L'aumento del 43 % dei tempi medi è compatibile con la contesa di cache e memoria.

Le cause plausibili che questa prova non esercita: ritardi di DPC e ISR dei driver (il dossier R-M25-8 cita la soglia di 1–3 ms di Microsoft), thread a priorità pari o superiore, page fault sul percorso audio, carico della GPU, transizioni di alimentazione.

**Conseguenza.** La prima serie, con un gioco reale aperto, resta l'evidenza più informativa che abbiamo: lì una callback senza MMCSS ha superato il periodo (10,87 ms). La scelta fra Default e Games a CP-1 non dovrebbe poggiare sulla serie di carico.

**Proposta, R-M25-10.** Disegnare uno stress con **controllo positivo**, come per i rilevatori: uno stressor conta solo se dimostra di allungare gli intervalli in `glitchcal` o nel probe. Candidati da provare: carico di GPU, thread a priorità alta, pressione di memoria, I/O su disco e rete. Uno stressor che non produce effetto è `NON DISCRIMINANTE`, non un PASS.

### 6.2 L'attenuazione di −59,5 dB sull'uscita USB

**Verificato.**
- Lo scarto fra loopback e golden scalato è −148,89 dB in tutte e quattro le corse di carico.
- Una simulazione su segnale sintetico dà −151,8 dB per **una** moltiplicazione in float32 per un guadagno costante, a −59,5, −20 e −6 dB ([script](experiments/consolidation_2026-11-09/float32_gain_residual.py)).

**Inferenza.** Lo scarto misurato ha l'ordine di grandezza dell'arrotondamento di un solo stadio di guadagno in float32. È compatibile con un volume applicato dal motore audio di Windows prima del punto di cattura, non con un'elaborazione più complessa. I 3 dB di differenza non sono spiegati.

**Fonti, lette oggi tramite estrattore.** Tre pagine Microsoft dicono che per default il loopback è preso prima di volume e muto: [`AUDCLNT_STREAMOPTIONS`](https://learn.microsoft.com/en-us/windows/win32/api/audioclient/ne-audioclient-audclnt_streamoptions), [API per gli APO](https://learn.microsoft.com/en-us/windows-hardware/drivers/audio/windows-11-apis-for-audio-processing-objects) e [`KSPROPERTY_AUDIOLOOPBACK`](https://learn.microsoft.com/en-us/windows-hardware/drivers/audio/ksproperty-audioloopback). Quest'ultima scrive però anche: «The loopback stream also gets affected by endpoint volume and mute». Le fonti non sono quindi univoche, e nessuna distingue fra volume hardware e volume software.

**Prova proposta, breve.** Registrare nel manifest del probe il volume master dell'endpoint in dB (`IAudioEndpointVolume::GetMasterVolumeLevel`) e se il volume è hardware (`QueryHardwareSupport`). Ripetere la corsa USB a volume massimo. Previsione da registrare prima: guadagno del loopback uguale al volume master, e guadagno 1,0 a volume massimo. Se la previsione fallisce, la causa è altrove. Chiude OSS-M2.5.1d-02.

**Conseguenza pratica.** Se l'ipotesi regge, la procedura di CP-1 può chiedere il volume master al massimo durante le corse di qualifica, così il confronto torna a livello pieno anche su USB.

### 6.3 Effetti, saturazione e supersaw: tre prerequisiti

Nessuno di questi punti è un risultato. Sono domande che R-M25-3 dovrebbe contenere, perché toccano contratti esistenti.

1. **Denormali.** Riverbero e delay con feedback producono code che decadono verso zero. [ADR-0005](../adr/0005-m2-4-inc4-entry-decisions.md) cita già il costo dei denormali su x86-64, e il codice non imposta FTZ o DAZ (nessuna occorrenza in `engine/`). ADR-0007 §8 rinvia «FTZ/DAZ per thread» a «solo se la baseline su device lo motiva»: con strutture a feedback la motivazione arriva dagli effetti, non dalla baseline. Proposta: legare quella voce a R-M25-3.
2. **Aliasing della saturazione.** Una non linearità prima del limiter genera armoniche oltre Nyquist. Lo [studio sull'aliasing di Inc-1](M2_4_Voice_Distortion_Aliasing_Evidence.md) ha già misurato il problema per l'hard clipping. Da confrontare con fonti lette: sovracampionamento e metodi antiderivativi. Il limiter a latenza zero resta l'ultimo stadio (ADR-0004).
3. **Voci.** D-M2-03 fissa due voci per ruolo. Una supersaw a due voci su Vettore rientra nel contratto solo se le note del lead non si sovrappongono. «Riassegnare le voci libere» fra ruoli lo modifica e richiede un ADR, che ADR-0007 esclude dal proprio perimetro.

### 6.4 Rampa di tempo a gradini

**Verificato.** `MusicClock` ammette 16 segmenti di tempo (`music_clock.h:34`). La fixture ne usa tre.

**Inferenza.** Una rampa approssimata con segmenti a tempo costante resta razionale ed esatta e non richiede di estendere l'orologio. Con 13 segmenti liberi si ottengono circa sei gradini per transizione. Una rampa continua richiede invece una nuova legge di posizione, con regole di arrotondamento, e un ADR. R-M25-4 dovrebbe confrontare le due, insieme alla continuità dei pattern già prevista. L'udibilità dei gradini non è nota.

## 7. Piano delle ricerche

### 7.1 Ricerche già in ADR-0007

| ID | Tema | Stato | Osservazione di questa revisione |
|---|---|---|---|
| R-M25-7 | Direttore musicale | Eseguita | Tre DOI ora verificati. Nessuna correzione di merito |
| R-M25-8 | Glitch su Windows | Eseguita | Le misure non hanno avuto review (dichiarato). Resta da integrare il monitor `IAudioClock` |
| R-M25-1 | Bilanciamento, loudness per stem | M2.5.2 | Dipende da M2.5.1c |
| R-M25-2 | Immagine stereo | M2.5.2 | Aggiungere la compatibilità mono e il bilanciamento fra canali come misure, oltre al Side/Mid che l'esperto giudica insufficiente |
| R-M25-3 | Effetti e costo DSP | dopo CP-1 | Anticipare la parte su LFO, saturazione e supersaw (CONS-08); aggiungere denormali e aliasing (§6.3) |
| R-M25-4 | Transizioni | M2.5.3 | Aggiungere la rampa a gradini (§6.4) |
| R-M25-5 | Normalizzazione degli onset | prima di CP-4 | Invariata |
| R-M25-6 | Statistica per gruppi | prima di CP-4 | Invariata |
| R-M25-9 | Palette timbrica | CP-3 e prima di M7 | Invariata |

### 7.2 Ricerche nuove proposte

Non sono in ADR-0007: il titolare decide se inserirle nel §7 dell'ADR.

| ID proposto | Domanda | Perché serve | Metodo | Quando |
|---|---|---|---|---|
| R-M25-10 | Quale stress mette davvero alla prova il percorso audio? | La prova di carico attuale non discrimina (§6.1). «Senza crackle» è un criterio obbligatorio | Stressor con controllo positivo su `glitchcal`; confronto controbilanciato Default contro Games; fonti Microsoft su DPC e MMCSS lette per intero | Prima di M2.5.6; la parte MMCSS prima di CP-1 se la scelta del default dipende da essa |
| R-M25-11 | Come si rende udibile un metro diverso da 4/4? | La fattibilità mostra che il 3/4 si genera ma non ha accenti propri. Senza questa risposta P-M25-01 si decide solo sulla lettera del criterio | Letteratura su accento metrico e induzione del metro; modello a onset già usato (LHL, E0) esteso a 3/4 e 6/8; articolo CONS-09 | Solo se CP-1 sceglie il cambio di metro |
| R-M25-12 | Dove cade il punto di cattura del loopback rispetto al volume? | Chiude OSS-M2.5.1d-02 e stabilisce se la qualifica va fatta a volume massimo | Prova del §6.2 con previsione registrata prima | M2.5.1c o CP-1 |

### 7.3 Ricerche oltre M2.5, già note

Nessuna va anticipata. Sono elencate perché una decisione di M2.5 non le chiuda per errore.

| Milestone | Decisione o ricerca | Fonte |
|---|---|---|
| Ingresso M3 | Piano semantico condiviso o sidecar; quali campi del contratto d'insieme congelare | [addendum](../roadmap_music_research_addendum.md), R-M25-7 |
| M3, M4 | Durata del commit horizon ai BPM estremi; discretizzazione della reachability; regola del dash | specifica §36, punti 5–7 |
| M5, M7 | Validazione empirica di `PlayerProfile_v1` | [contratto M1](../player_contract_m1.md) |
| M6 | Attuazione di §16.3–§16.4, se CP-3 e CP-4 la collocano lì; metrica di pressione osservata | R-M25-7; specifica §36, punto 8 |
| M7 | Diversità misurabile fra run: oggi nulla. Ripetizione in gioco: nessun dato pubblicato | Teorema D; R-M25-7 |

## 8. Limiti di questa revisione

- Un solo revisore, lo stesso agente che ha scritto parte dei documenti esaminati: non è una review indipendente ai sensi di AGENTS §5.1.1 e non la sostituisce.
- Le inferenze del §6 non sono misurate, salvo la simulazione del §6.2, che usa un segnale sintetico e non la fixture.
- Le pagine Microsoft del §6.2 sono state lette tramite estrattore automatico; una era troncata.
- Il controllo dei riferimenti al codice copre i dossier di M2.5, non le centinaia di riferimenti dei documenti di M2.4.
- Committato il 2026-11-10 su indicazione del titolare, senza review indipendente: la review resta da fare (ADR-0007 §6).
