# CACP — metodi di valutazione audio dell'agente: decisioni per Inc-5 e ricerche d'ingresso M5/M6

- **Data:** 2026-10-21
- **Stato:** documento di ricerca (non normativo). **Decisione del titolare, 2026-10-21: raccomandazioni R1–R6 del §6 approvate** per M2.4 Inc-5 (`accepted for implementation`; nessun parametro `validated`). R1 è registrata come precisazione di [ADR-0005](../adr/0005-m2-4-inc4-entry-decisions.md) D-M2-11 e nella [skill CACP](../../.agents/skills/cacp-testing/SKILL.md); R2–R6 sono deliverable di Inc-5. Le ricerche del §7 restano condizioni d'ingresso proposte per M5 e M6, da confermare all'ingresso di ciascuna milestone.
- **Perimetro:** completo solo per quanto serve a M2.4 Inc-5 e AC-AO-02. Analisi live e video sono rinviate per scelta del titolare («procediamo passo passo»).
- **Riferimenti normativi:**
  - specifica §31.5 e §32 (AC-AO-01B, AC-AO-02);
  - [agent_control_observability.md](../agent_control_observability.md) §4, §6, §10, §12, §13.2;
  - [ADR-0005](../adr/0005-m2-4-inc4-entry-decisions.md) D-M2-11;
  - `AGENTS.md` §5.1.2;
  - [skill CACP](../../.agents/skills/cacp-testing/SKILL.md).
- **Dati e sorgenti:** [experiments/cacp_methods/](experiments/cacp_methods/).

## 1. Domande

1. **Q1 — metodi ammessi:** quali metodi di valutazione possono usare gli agenti nel CACP e con quale peso probatorio? I candidati sono ingestione audio, lettura di spettrogrammi, descrittori quantitativi, telemetria del motore e ascolto umano.
2. **Q2 — Inc-5:** che cosa deve consegnare Inc-5 perché quei metodi siano verificabili e riproducibili, e perché AC-AO-02 («onset correlati all'intent») sia misurabile?
3. **Q3 — rinvii:** che cosa va rinviato a M5 (osservazione live) e M6 (audio e video continui), e quali evidenze servono prima di entrarvi?

## 2. Contesto

Il CACP oggi esegue scenari offline deterministici (`--cacp-run`), produce WAV e manifest JSON e lascia all'agente un giudizio percettivo *supplementare* (AC-AO-01B, D-M2-11). Il 21 ottobre 2026 un agente esterno con ingestione audio (Antigravity, modello dichiarato Gemini) ha prodotto tre resoconti percettivi su Inc-4:

- **due respinti:** descrivevano la Trama dove il planner non la produce, un delay «ping-pong» inesistente, costanti copiate dal codice e presentate come ascolto, e un p99.9 superato;
- **uno accettato come supplementare:** [m2_4_inc4_perceptual_evidence.md](../m2_4_inc4_perceptual_evidence.md), dopo controlli incrociati fatti a mano.

L'episodio dimostra il problema da risolvere: oggi la verifica di un giudizio percettivo dipende da controlli manuali non standardizzati.

## 3. Evidenze

### 3.1 Fonti esterne (consultate il 2026-10-21)

| ID | Fonte | Contenuto pertinente | Verifica |
|---|---|---|---|
| G-DOC | Google, «Audio understanding», Gemini API docs ([link](https://ai.google.dev/gemini-api/docs/audio), «Last updated 2026-09-23 UTC») | Sezione «Technical details about audio»: 32 token al secondo; «Downsampled to 16 Kbps»; «Multi-channel audio combined to single channel». | testo letto direttamente nel browser |
| MMAU | S. Sakshi et al., «MMAU: A Massive Multi-Task Audio Understanding and Reasoning Benchmark», ICLR 2025, [arXiv:2410.19168](https://arxiv.org/abs/2410.19168) | 10k clip fra parlato, suoni e musica. Accuratezza migliore riportata ≈ 53 % (Gemini Pro 1.5, versione arXiv) e 59,93 % (Gemini 2.0 Flash, versione ICLR). | abstract e metadati |
| HALL | C.-Y. Kuan, W.-P. Huang, H. Lee, «Understanding Sounds, Missing the Questions: The Challenge of Object Hallucination in Large Audio-Language Models», [arXiv:2406.08402](https://arxiv.org/abs/2406.08402), 2024 | I modelli audio-linguistici descrivono l'audio in modo competitivo, ma sbagliano le domande discriminative («questo suono è presente?»). | abstract |
| SPEC-FS | S. Dixit, L. M. Heller, C. Donahue, «Vision Language Models Are Few-Shot Audio Spectrogram Classifiers», [arXiv:2411.12058](https://arxiv.org/abs/2411.12058), 2024 | Con esempi di riferimento, GPT-4o classifica spettrogrammi ESC-10 al 59,00 %, contro il 49,62 % di Gemini 1.5 che ascolta lo stesso audio. Leggermente meglio di esperti umani sulla classificazione visiva (73,75 % contro 72,50 % sul primo fold). | abstract |
| SPEC-NO | T. Loakman, J. James, C. Lin, «Seeing isn't Hearing: Benchmarking Vision Language Models at Interpreting Spectrograms», IJCNLP-AACL 2025, [arXiv:2511.13225](https://arxiv.org/abs/2511.13225) | Su trascrizione fonemica da spettrogrammi i modelli, zero-shot o finetuned, «rarely perform above chance». Serve una conoscenza parametrica di come leggere le figure. | abstract |
| ONSET | J. P. Bello et al., «A Tutorial on Onset Detection in Music Signals», *IEEE Trans. Speech Audio Process.* 13(5), 1035–1047, 2005, DOI [10.1109/TSA.2005.851998](https://doi.org/10.1109/TSA.2005.851998) | Rassegna dei metodi di rilevamento degli onset (funzioni di novità spettrali e d'energia, peak picking). È la base del metodo usato nel §3.2. | metadati e abstract |
| MIREVAL | C. Raffel et al., «mir_eval: A Transparent Implementation of Common MIR Metrics», ISMIR 2014, 367–372 | Implementazione di riferimento delle metriche; la tolleranza di default per gli onset è 50 ms. | metadati; il default è verificato tramite la firma `onset_tolerance=0.05` della documentazione, non sul testo del paper |
| LIBROSA | B. McFee et al., «librosa: Audio and Music Signal Analysis in Python», Proc. 14th Python in Science Conf., 18–25, 2015 ([PDF](https://proceedings.scipy.org/articles/Majora-7b98e3ed-003.pdf)) | Implementazione di riferimento di STFT, scala mel e onset. Qui è solo un riferimento metodologico, non una dipendenza. | metadati |

**Conseguenze dirette:**

1. **Stereo non valutabile con l'API Gemini.** Un modello che ascolta tramite l'API Gemini riceve un segnale **mono** a 16 Kbps (G-DOC), quindi non può giudicare immagine stereo, larghezza o delay fra i canali. Nel resoconto accettato di Antigravity l'affermazione «timbro allargato ai lati» è corretta nel fatto (§3.2, Side/Mid), ma non può derivare dall'ascolto se la pipeline è quella documentata dell'API. Va trattata come non attribuibile all'ascolto.
2. **Nessun giudizio sotto i ~31 ms.** Con 32 token al secondo un token copre circa 31 ms. Click, discontinuità sotto il millisecondo e allineamenti più fini della tolleranza onset di 50 ms non sono giudicabili dall'ingestione, a prescindere dal modello.
3. **Domande binarie inaffidabili.** Le accuratezze dei benchmark (≈ 53–60 %, MMAU) e le allucinazioni sulle domande binarie (HALL) rendono il giudizio d'ascolto inadatto come gate. È proprio il tipo di errore osservato nei due resoconti respinti.
4. **Spettrogrammi utili solo con riferimenti.** La lettura di spettrogrammi da parte di modelli visivi funziona solo con esempi di riferimento (SPEC-FS) e fallisce senza conoscenza specifica (SPEC-NO), come conferma il §3.2.

### 3.2 Misure proprie (fixture-m2-4-v1, `fixture_probe_v1.wav` build `9def52c`, seed 1337)

**Procedura:** [cacp_methods_experiment.py](experiments/cacp_methods/cacp_methods_experiment.py), solo numpy e libreria standard.

**Riferimento:** gli eventi pianificati esportati dal planner offline, [fixture_intents_seed1337.csv](experiments/cacp_methods/fixture_intents_seed1337.csv):

- 1.392 NoteOn, pari a 1.008 frame di onset distinti;
- planner con seed 1337, `candidateCount` 8 e orizzonte 0, come `planFixtureForm` nei test;
- export eseguito con un test temporaneo poi rimosso.

**Parametri fissati prima di guardare i risultati:** STFT Hann 2048, hop 256; novelty = spectral flux di `log(1 + 100|X|)` sul mono; soglia = media mobile ±100 ms + δ; distanza minima fra onset 30 ms; tolleranza ±50 ms (MIREVAL).

**E1 — Onset dal mix master contro Music Intent** ([report](experiments/cacp_methods/onset_report_2026-10-21.json)):

| δ | Rilevati | Precisione | Recall | F | Offset mediano [p5, p95] | Recall Pulso / Corpo / Trama / Vettore |
|---|---|---|---|---|---|---|
| 0,05 | 991 | 0,920 | 0,905 | 0,912 | −7,3 ms [−11,1; +10,3] | 1,00 / 0,99 / 0,76 / 1,00 |
| **0,10 (nominale)** | 730 | **1,000** | 0,724 | 0,840 | −8,3 ms [−11,2; −2,7] | 1,00 / 0,99 / **0,31** / 0,98 |
| 0,20 | 464 | 1,000 | 0,460 | 0,630 | −9,1 ms [−11,6; −6,4] | 1,00 / 0,42 / 0,02 / 0,29 |

- **Precisione perfetta, recall parziale:** alla soglia nominale tutto ciò che si rileva corrisponde a un intent, ma un quarto degli intent non è visibile nel mix.
- **La Trama non è osservabile dal master:** è il ruolo più debole (bus −10 dB, FM breve) e ha recall 0,31; la recall varia fortemente con δ. Un controllo AC-AO-02 «per ruolo» sul solo master non è affidabile.
- **Scarto sistematico di circa −8 ms:** dipende dalla convenzione temporale del frame STFT. È costante e va calibrato, non ignorato.
- **Costo:** STFT del mono di 182,9 s in 0,59 s e novelty in 0,24 s (numpy; macchina della §3 del dossier R2-01). L'analisi offline costa poco rispetto al render.

**E2 — Lettura alla cieca di spettrogrammi da parte di un agente** (Claude, solo immagini; [giudizi](experiments/cacp_methods/blind_judgements_2026-10-21.md), [hash dei PNG](experiments/cacp_methods/png_sha256_2026-10-21.txt)). Domanda: «la finestra di 4 battute contiene la Trama?».

- **Turno 1, senza riferimenti: 2/6, sotto il caso.** L'errore è sistematico: gli attacchi larghi di Pulso e Vettore sono stati scambiati per Trama. Conferma SPEC-NO.
- **Turno 2, con riferimenti etichettati e finestre nuove: 6/6.** Le frasi ripetute generano spettrogrammi quasi identici, quindi il turno misura il confronto con riferimenti, non la capacità di interpretare. Conferma SPEC-FS nel limite della ripetitività.

**E3 — Descrittore oggettivo Side/Mid** (energia della differenza L−R rispetto alla somma, per finestra di 4 battute):

| Trama | Side/Mid |
|---|---|
| presente | −20,05 … −20,36 dB |
| assente | −23,59 … −24,07 dB |

Le due classi sono separate da 3,2 dB senza sovrapposizioni su 12 finestre. Un descrittore semplice e deterministico distingue ciò che l'agente non distingueva al turno 1.

**E4 — Telemetria per ruolo:** `ChannelFrame` ha un campo `onset` per ruolo che non viene mai valorizzato e resta a 0. Lo si verifica in `engine/audio/role_bus_processor.cpp`, dove sono calcolati solo energia, densità e simili. Oggi quindi il motore non espone onset per ruolo.

## 4. Alternative

### 4.1 Metodi di valutazione

| Metodo | Cosa osserva | Punti di forza | Limiti misurati o documentati |
|---|---|---|---|
| M-A Ingestione audio dell'agente | mix master, in mono e a 16 Kbps per l'API Gemini | descrizione qualitativa globale; unico metodo «d'ascolto» di un agente | niente stereo (G-DOC); niente dettagli sotto ~31 ms; inaffidabile sulle domande binarie (HALL, MMAU); 2 resoconti su 3 respinti |
| M-B Spettrogramma letto dall'agente | immagine tempo-frequenza | accessibile agli agenti che leggono solo immagini; riproducibile (PNG con hash) | senza riferimenti 2/6 (E2); serve un pacchetto di riferimenti e annotazioni (SPEC-FS, SPEC-NO) |
| M-C Descrittori quantitativi sul PCM | onset, energia, Side/Mid, picchi, silenzio | deterministici, testabili in CTest, con soglie calibrate | dal master la Trama non si separa (E1); i descrittori vanno validati sulla fixture con riferimento noto |
| M-D Telemetria del motore e stem per ruolo | segnali per ruolo prima del master | separa i ruoli; correla direttamente con gli intent | gli onset per ruolo non esistono (E4); servono nuovo codice o un tap per ruolo |
| M-E Ascolto umano | percezione reale, device | unico criterio di qualità artistica | non automatizzabile; resta a M2.5 |

### 4.2 Collocazione degli strumenti di analisi

| Opzione | Descrizione | Pro | Contro |
|---|---|---|---|
| T1 C++ nel motore e nei probe | descrittori e PNG generati da `cymatica_audio`/`cymatica_game` nei worker offline | nessuna nuova dipendenza; CTest; stessi hash; coerente con «analisi nei worker» (§6 agent_control_observability) | va scritto un encoder PNG minimo (deflate *stored*, CRC32 e Adler-32, nessuna compressione: circa 0,7 MB per un'immagine 900×256 RGB) |
| T2 Tool Python con numpy | script come quello della §3.2 | rapido da evolvere | dipendenza Python e numpy da fissare in `docs/dependencies.md`; fuori da CTest; due implementazioni da tenere allineate |
| T3 Ibrido | descrittori normativi in C++, Python solo per la ricerca | gate deterministici; ricerca flessibile | va rispettato il confine: nessun gate dipende da Python |

## 5. Matrice

| Criterio | M-A | M-B senza riferimenti | M-B con riferimenti | M-C | M-D | M-E |
|---|---|---|---|---|---|---|
| Riproducibile | no (modello, prompt) | sì (PNG), il giudizio no | sì (PNG), il giudizio no | **sì** | **sì** | no |
| Verificabile in automatico | solo con controlli incrociati | solo con controlli incrociati | solo con controlli incrociati | **sì** | **sì** | no |
| Affidabilità misurata | 1/3 resoconti accettati | 2/6 | 6/6 (limite: ripetitività) | P 1,00, R 0,72; Side/Mid separazione 3,2 dB | da misurare | riferimento finale |
| Stereo | **no** (mono) | sì, se si genera lo spettrogramma Side | sì | sì | sì | sì |
| Ammissibile come gate | no | no | no | **sì** (dopo calibrazione) | **sì** (dopo implementazione) | sì (M2.5) |

## 6. Raccomandazioni per Inc-5 (approvate dal titolare il 2026-10-21)

**R1 — Gerarchia dei metodi** (da riportare nella skill CACP e in D-M2-11). Gate solo da M-C/M-D e dai test. M-A e M-B restano supplementari e valgono solo se passano i **controlli di falsificazione automatici** di R3. Regole minime:

- un giudizio d'ascolto ottenuto tramite ingestione mono non può contenere affermazioni sullo stereo;
- nessuna affermazione su fenomeni sotto ~31 ms;
- la lettura di spettrogrammi senza pacchetto di riferimenti non è ammessa come evidenza;
- ogni affermazione deve indicare la finestra (frame o battute) e il metodo da cui deriva.

**R2 — Pacchetto di evidenza riproducibile** (nuovo scenario, ad esempio `evaluation_pack_v1`, generato dal repo e non da script esterni):

- finestre scelte dal planner con i ruoli attivi annotati, inclusi i casi «ruolo assente» come controllo;
- per ogni finestra: WAV, spettrogrammi mel di Mid e Side con assi in battute e marcatori di sezione;
- spettrogrammi di riferimento etichettati, uno per ruolo e per sezione;
- descrittori JSON (onset rilevati, Side/Mid, RMS, picco, silenzio);
- hash di tutto.

Il pacchetto rende verificabili sia M-A sia M-B e sostituisce gli slice ad hoc che gli agenti esterni oggi generano da soli.

**R3 — Risultato di valutazione strutturato v0**, sottoinsieme di D-AO-04 (§13.2 di agent_control_observability). Campi: valutatore e modello, capacità dichiarate (canali, risoluzione, token/s), hash degli input, timestamp, lista di affermazioni {finestra, metodo, testo}. Il CACP verifica in automatico ogni affermazione contro i dati del pacchetto (es. «ruolo X presente nella finestra W» contro gli intent) e marca le affermazioni non attribuibili (es. stereo con ingestione mono). L'esito resta separato dal gate.

**R4 — AC-AO-02 misurabile per ruolo:** il master non basta (E1). Le opzioni da decidere sono tre, tutte fuori dal percorso critico della callback salvo O2:

- **O1:** tap AudioProbe esteso agli stem per ruolo prima del master, solo offline (memoria ×5, nessuna analisi in callback);
- **O2:** valorizzare `ChannelFrame.onset` con un rivelatore d'inviluppo limitato (bounded) per ruolo nel percorso dei bus, che già calcola energia e picchi; richiede una misura OFF/ON con D-M2-10;
- **O3:** correlare gli eventi applicati dal motore con gli intent (verifica la catena di scheduling, non l'audio) più gli onset del master per i ruoli osservabili.

**Raccomandata:** O1 per il gate offline, che è deterministico e non tocca la callback, più O3 come controllo di catena; O2 va valutata in M5 per il live. In ogni caso il campo `onset` oggi a 0 va valorizzato oppure documentato come riservato.

**R5 — Strumenti:** T3. Descrittori, finestre e PNG minimo in C++ (T1) per tutto ciò che diventa gate o pacchetto; lo script numpy resta uno strumento di ricerca archiviato, non una dipendenza del CACP.

**R6 — Calibrazione dichiarata:** soglie e parametri dei descrittori (δ, tolleranza, scarto di −8 ms) vanno fissati su un insieme di calibrazione (es. fixture con seed 1337) e verificati su un insieme separato (es. altri seed, quando il generatore li differenzierà; nota di ADR-0005). Ogni cambio di parametri si versiona.

## 7. Ricerche d'ingresso a M5 e M6 (proposte)

Le ricerche vanno svolte con il metodo del §6 della regola document-driven prima di congelare le rispettive decisioni (D-AO-05 in M5, spike AC-AO-06 in M6). Non sono lavoro di M2.

### 7.1 Ingresso M5 — osservazione live e monitor audio in tempo reale

| ID | Domanda di ricerca | Evidenza richiesta |
|---|---|---|
| R-M5-1 | Architettura del monitor live: worker che consuma l'AudioProbe e calcola STFT e descrittori scorrevoli (età, gap, frame persi) | costo OFF/ON con la metodologia D-M2-10, latenza del monitor, comportamento con consumer lento (la callback non deve risentirne) |
| R-M5-2 | Trasporto verso l'agente: tool MCP con contenuti immagine/audio, polling o push, dimensioni e frequenza | documentazione MCP e del client alla data, prove di ricezione, limiti di contenuto |
| R-M5-3 | Valorizzazione di `ChannelFrame.onset` (O2) per il live | rivelatore bounded validato sulla fixture (P/R per ruolo), costo in callback misurato |
| R-M5-4 | Affidabilità dei giudizi live (ingestione e spettrogramma) | protocollo con finestre alla cieca, riferimenti, accordo con valutatori umani; ripetere E2 con contenuto non ripetitivo |
| R-M5-5 | Capacità aggiornate dei modelli audio (canali, risoluzione, token/s, latenza delle API live) | documentazione del fornitore riletta alla data d'ingresso, perché G-DOC può cambiare |
| R-M5-6 | Device e loopback distinti dal tap (§6 agent_control_observability) | procedura, autorizzazioni, evidenze separate |

### 7.2 Ingresso M6 — audio e video continui (spike)

| ID | Domanda di ricerca | Evidenza richiesta |
|---|---|---|
| R-M6-1 | Streaming continuo audio e video verso modelli: formati, frequenze, latenze e limiti delle API live dei fornitori | documentazione alla data e prova sperimentale; nessuna promessa di «riflessi» dell'LLM |
| R-M6-2 | Sincronismo audio-video e misura di età e gap | metodo di misura con riferimento (es. evento noto nello scenario) |
| R-M6-3 | Codifica e dipendenze | FFmpeg e simili richiedono un'approvazione esplicita (AGENTS §2); costo di codifica misurato |
| R-M6-4 | Affidabilità dei modelli video su eventi con tempi critici (telegraph, hazard) | benchmark pubblicati e prova alla cieca su clip del gioco |
| R-M6-5 | Validazione del risultato strutturato del critic (D-AO-04) | calibrazione: accordo con assertion e umani, falsi positivi e negativi |

## 8. Limiti

- **E1 su una sola fixture e un solo seed:** il generatore di Inc-3/Inc-4 varia poco fra seed, quindi le metriche valgono per questo contenuto.
- **E2:** un solo valutatore (Claude), 12 finestre e contenuto ripetitivo. È un indizio coerente con la letteratura, non una misura di affidabilità generale.
- **E3:** Side/Mid separa la Trama perché è l'unico ruolo con delay stereo. Non è un rivelatore generale di ruoli.
- **Fonti:** abstract e metadati, salvo G-DOC (testo letto). Le accuratezze dipendono dalla versione del benchmark (MMAU è stato rivisto nel 2025).
- **Intent non è percezione** (revisione esterna R3, 2026-10-21): la corrispondenza fra onset rilevati e Music Intent prova che un evento era previsto ed è arrivato al segnale, non che sia udibile o musicalmente efficace. I controlli di falsificazione di R1/R3 respingono affermazioni incompatibili con il piano, ma non dimostrano la percezione: quella resta all'ascolto umano.
- **Pipeline di Antigravity non verificata:** se Antigravity non usa la pipeline documentata dell'API Gemini, la conseguenza 1 del §3.1 va riverificata sul suo canale reale.

## 9. Decisione (2026-10-21)

Il titolare ha approvato R1–R6 senza modifiche. Dove sono registrate:

| Raccomandazione | Record |
|---|---|
| R1 gerarchia dei metodi e regole di ammissibilità | [ADR-0005](../adr/0005-m2-4-inc4-entry-decisions.md) D-M2-11, precisazione del 2026-10-21; [skill CACP](../../.agents/skills/cacp-testing/SKILL.md) Passo 3b |
| R2 `evaluation_pack_v1`, R3 risultato strutturato v0, R4 AC-AO-02 per ruolo (O1 + O3), R5 strumenti (T3), R6 calibrazione | deliverable di Inc-5 nel piano di [progress.md](../progress.md) |

Parametri, formati e schemi di R2–R4 restano da definire nell'implementazione di Inc-5, con test e versione. Nessun valore è `validated`.

**Aggiornamento del 2026-10-26:** i contratti d'ingresso di Inc-5 che attuano R2–R6 sono in [ADR-0006](../adr/0006-m2-4-inc5-entry-decisions.md) (D-M2-12..16), con le correzioni della revisione del piano INC5-REV-01..05. Gli approfondimenti relativi sono nel [dossier di ricerca d'ingresso di Inc-5](M2_4_Inc5_Entry_Research.md).
