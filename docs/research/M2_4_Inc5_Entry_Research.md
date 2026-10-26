# M2.4 Inc-5 — ricerca d'ingresso: identità e diversità musicale, separazione della ground truth, onset per ruolo, stato dell'arte nei giochi

- **Data:** 2026-10-26
- **Stato:** documento di ricerca, non normativo. Sostiene le decisioni d'ingresso di Inc-5 in [ADR-0006](../adr/0006-m2-4-inc5-entry-decisions.md) (D-M2-12..16), che sono in review del titolare (fase 5.0).
- **Origine:** applicazione della regola document-driven §6.4: ricerca parallela allo sviluppo, su indicazione del titolare del 2026-10-26. Durante la stesura di ADR-0006 è emerso che quattro decisioni dipendevano da temi non ancora documentati nel repository. Su quei temi sono state svolte quattro ricerche parallele, con verifica diretta delle fonti.
- **Metodo e livelli di verifica:** ogni fonte riporta il riferimento completo, il DOI o un link permanente e la data di consultazione (2026-10-26). Il livello di verifica effettivo è uno di questi:
  - **FT:** testo completo letto (PDF estratto localmente o HTML completo);
  - **AM:** solo abstract e metadati sulla pagina primaria;
  - **SEC:** solo segnalazione secondaria. Una fonte SEC non sostiene alcuna affermazione di questo documento.

  Le citazioni testuali sono brevi. Le inferenze dell'analisi sono segnalate come tali.
- **Dati:** nessuna misura nuova. Le misure di riferimento restano quelle del [dossier sui metodi CACP](M2_4_CACP_Evaluation_Methods.md) §3.2 (E1–E4). Le misure di Inc-5 saranno prodotte nelle fasi 5a–5e.

## 0. Base già presente nel repository

Questo dossier **parte** dalla documentazione già raccolta nel repository, senza sostituirla, e approfondisce solo ciò che le decisioni di Inc-5 richiedono (indicazione del titolare del 2026-10-26).

| Documento | Che cosa fornisce già | Uso qui |
|---|---|---|
| [M2_4_Deep_Research_Reviewed.md](M2_4_Deep_Research_Reviewed.md), «Valutazione scientifica ed esperimenti musicali» | quattro piani di valutazione separati (correttezza, struttura, diversità/originalità, percezione); misure di diversità (*pairwise motif distance*, *n-gram duplication*, *archive coverage*, *self-similarity*, *pairwise run distance*); «decine di seed» e clip di 60–90 s; Yang e Lerch 2020 e Lerch et al. 2025 come base; **originalità non equivale a distanza statistica** | Q1 riusa le stesse famiglie di misure e le rende operative sul flusso di eventi; il principio limita l'interpretazione della diversità a dato esplorativo |
| idem, sezione sul ritmo | Danielsen et al. 2024: il tempo percepito di un onset dipende da attacco, durata e timbro; Frühauf et al. 2013 sul microtiming | Q3: l'onset fisico rilevato non coincide con l'onset percepito, e lo scarto per ruolo va calibrato |
| [M2_4_Evaluation_Protocol.md](M2_4_Evaluation_Protocol.md) §3–§5 | ascolto A/B con ordine randomizzato e controbilanciato, sigle anonime, file allineati in loudness; tabella delle «interpretazioni vietate» (es. «entropia maggiore ⇒ originalità artistica»); correlazione AC-AO-02 con finestra di 5 ms / 240 frame per Pulso come candidato; profilo `AUD-METER-EXPLORATORY` | Q2 estende agli agenti il cieco già previsto per gli umani; Q3 e D-M2-16 rispettano il candidato per Pulso e il carattere esplorativo |
| [M2_4_CACP_Evaluation_Methods.md](M2_4_CACP_Evaluation_Methods.md) | misure E1–E4 sulla fixture; fonti G-DOC, MMAU, HALL, SPEC-FS, SPEC-NO, ONSET (Bello et al. 2005), MIREVAL e LIBROSA; raccomandazioni R1–R6 | punto di partenza di Q2 e Q3; le fonti non vengono ripetute |
| [M2_4_Research_Review.md](M2_4_Research_Review.md), [Audio Renderer Research Review](M2_4_Audio_Renderer_Research_Review.md) | stato dell'arte già raccolto (Just Shapes & Beats, Audiosurf, Fournel GDC 2010, Talakat) | Q4 lo integra senza duplicarlo |

## 1. Domande

1. **Q1 — identità e diversità (D-M2-12, D-M2-16):** come si identifica una composizione indipendentemente da seed, ID e metadati? Come si misura la diversità fra composizioni generate con seed diversi? Quando un esito va dichiarato inconcludente?
2. **Q2 — separazione della ground truth (D-M2-15):** quali misure impediscono a un agente valutatore di ottenere la risposta dalla ground truth invece che dall'audio? Quali rischi restano?
3. **Q3 — onset per ruolo (D-M2-14, D-M2-16):** il rivelatore di onset del dossier CACP è adatto agli stem dei quattro ruoli? Come si definisce la ground truth per ruolo, e come si separano calibrazione e verifica?
4. **Q4 — stato dell'arte nei giochi:** come gestiscono varietà, sincronismo musica–gameplay e verifica i giochi pubblicati con musica procedurale o adattiva, inclusi i bullet hell e i giochi guidati dalla musica?

## 2. Q1 — Identità del contenuto musicale e diversità fra seed

### 2.1 Verifica sul codice

- `canonicalPlanHash` ([plan_canonical_hash.cpp](../../engine/audio/plan_canonical_hash.cpp)) serializza, per ogni trace, `runSeed`, `decisionId`, `tieBreakHash`, candidati e punteggi. Per ogni evento serializza `eventId`, `tick` e `transportEpoch`. Due seed che producono la stessa musica hanno quindi hash diversi (INC5-REV-01).
- `MusicIntentEvent` ([music_intent.h](../../engine/audio/music_intent.h)) contiene `eventId`, `sampleFrame`, `tick`, `transportEpoch`, `role`, `pitchHz`, `velocity`, `durationFrames` e `timbreParam`.

### 2.2 Fonti

| ID | Fonte | Verifica | Contenuto pertinente |
|---|---|---|---|
| F1 | L.-C. Yang, A. Lerch, «On the evaluation of generative models in music», *Neural Computing and Applications* 32, 4773–4784, online 2018, volume 2020. [DOI 10.1007/s00521-018-3849-7](https://doi.org/10.1007/s00521-018-3849-7); [postprint](https://musicinformatics.gatech.edu/wp-content_nondefault/uploads/2018/11/postprint.pdf) | FT (postprint) | Feature per campione (istogramma delle classi d'altezza, matrice di transizione, range, IOI, conteggio note); distanze intra-set e inter-set a coppie. La media intra-set «corresponds to the diversity of the samples». KLD «unbounded and asymmetric»; area di sovrapposizione (OA) limitata in [0, 1]. La robustezza della KDE dipende dal numero di campioni. |
| F2 | H.-W. Dong, K. Chen, J. McAuley, T. Berg-Kirkpatrick, «MusPy: A Toolkit for Symbolic Music Generation», ISMIR 2020. [DOI 10.48550/arXiv.2008.01951](https://doi.org/10.48550/arXiv.2008.01951); [documentazione delle metriche](https://hermandong.com/muspy/metrics.html) | FT (arXiv) e documentazione ufficiale | Metriche di altezza (entropia delle classi d'altezza, pitch-in-scale) e di ritmo (empty-beat rate, groove consistency), pensate per confrontare dati generati e di riferimento. |
| F3 | M. Mongeau, D. Sankoff, «Comparison of musical sequences», *Computers and the Humanities* 24, 161–175, 1990. [DOI 10.1007/BF00117340](https://doi.org/10.1007/BF00117340) | AM | Distanza d'edit fra sequenze musicali con operazioni di consolidamento e frammentazione, calcolata per programmazione dinamica. Il testo completo non è letto, quindi i pesi originali non si citano. |
| F4 | Z. Yin, F. Reuben, S. Stepney, T. Collins, «Measuring When a Music Generation Algorithm Copies Too Much: The Originality Report, Cardinality Score, and Symbolic Fingerprinting by Geometric Hashing», *SN Computer Science* 3:340, 2022. [DOI 10.1007/s42979-022-01220-y](https://doi.org/10.1007/s42979-022-01220-y); [copia aperta](https://eprints.whiterose.ac.uk/id/eprint/190005/) | FT | Cardinality score `cs(q,r) = max_t |(q+t) ∩ r| / max(|q|,|r|)` su insiemi di punti (onset, altezza). Con piccole differenze non rigide «cs will be near-zero, contrary to perception». Fingerprint da triple di note; i parametri vanno configurati. |
| F5 | A. Lerch, C. Arthur, N. Bryan-Kinns, C. Ford, Q. Sun, A. Vinay, «Survey on the Evaluation of Generative Models in Music», *ACM Computing Surveys*, 2025, [DOI 10.1145/3769106](https://doi.org/10.1145/3769106), già citato nella ricerca approfondita; letta la versione arXiv:2506.05104 v4 | FT (arXiv v4) | La diversità va misurata su più dimensioni. Generare solo varianti dello stesso brano «is generally not desirable». Le metriche quantitative di originalità e diversità sono poche, e mancano standard di «diversità sufficiente». |
| F6 | A. Rundgren, B. Jordan, S. Erdtman, «JSON Canonicalization Scheme (JCS)», RFC 8785, IETF, 2020. [DOI 10.17487/RFC8785](https://doi.org/10.17487/RFC8785) | FT | Analogia di metodo per l'hash di dati canonici: formato invariante; ordine deterministico; −0 serializzato come 0; NaN e Infinity non ammessi, con errore. |

**Fonti solo secondarie, non usate:** Arzt, Böck e Widmer, ISMIR 2012 (fingerprint simbolico; visto solo citato in F4); Banar e Colton, 2022 (visto solo citato in F5).

### 2.3 Analisi e scelta

**Due identità, non una sola.** Nessuna fonte tratta l'identità di un piano procedurale. La separazione fra provenienza e contenuto è un'inferenza ingegneristica: segue il principio di F6 (si fa l'hash di una forma canonica dei soli dati che definiscono l'oggetto) e il rilievo INC5-REV-01. L'hash di provenienza resta lo strumento per replay e golden. Il fingerprint del contenuto serve per duplicati e diversità.

**Definizione del fingerprint** (inferenza, registrata in D-M2-12 come profilo iniziale):

- **Campi:**
  - ruolo;
  - `sampleFrame` relativo all'origine del segmento analizzato (intero esatto);
  - `durationFrames` (intero esatto);
  - altezza quantizzata in centesimi di semitono rispetto a 440 Hz;
  - velocity e `timbreParam` quantizzati a 1/1.000.
- **NoteOff del planner:** ruolo e frame.
- **Ordine:** lessicografico totale su (frame, ruolo, altezza, durata, velocity, timbro).
- **Esclusioni:** seed, `eventId`, `tick`, epoch, `decisionId`, candidati, punteggi e tie-break.
- **Valori non finiti:** errore, nessun fingerprint (come in F6). Lo zero negativo vale 0. Un'altezza 0 o non applicabile usa una sentinella esplicita.
- **Hash:** FNV-1a a 64 bit, come `canonicalPlanHash`, più il numero di eventi.
  - Con le poche decine di seed previste, la probabilità di collisione è dell'ordine di N²/2⁶⁵, cioè trascurabile.
  - SHA-256 (D-M2-14) resta per i file del pacchetto.
  - Il passaggio a SHA-256 per il fingerprint è un'alternativa valida, rinviata: richiederebbe di anticipare a 5a una primitiva di 5b.
- **Fingerprint parziali:** per ruolo e per frase, utili alle metriche.
- **Controlli positivi e negativi (test di 5a):**
  - un solo campo musicale cambiato in un solo evento cambia il fingerprint;
  - cambiare `eventId`, `tick`, epoch o seed non lo cambia.

**Limite dichiarato.** Un valore vicino al confine di un intervallo di quantizzazione può cambiare il fingerprint per una differenza minima. L'uguaglianza dei fingerprint prova l'identità, non misura la somiglianza: per la somiglianza servono le metriche che seguono.

**Metriche di diversità esplorative** (D-M2-16, fase 5e). Rendono operative sul flusso di eventi le famiglie di misure già indicate nella ricerca approfondita: *pairwise run distance* (M2, M4), *n-gram duplication* (variante n-gram di M4) e *self-similarity*/duplicati (M1, M3).

| # | Metrica | Definizione | Base | Limiti |
|---|---|---|---|---|
| M1 | Identità esatta | quota di coppie di seed con fingerprint globale uguale; per coppia, quota di frasi con fingerprint uguale, per ruolo | inferenza; schema a finestre come in F4 | binaria; sensibile ai confini di quantizzazione |
| M2 | Distanze fra feature | per seed: istogramma delle classi d'altezza, istogramma IOI su griglia, numero di note, range, entropia delle classi d'altezza; distanze euclidee a coppie (intra-set) | F1, F2 | con pochi seed la KDE non è affidabile (F1): si riportano distanze grezze, media e mediana; feature uguali non provano un brano uguale |
| M3 | Cardinality score per ruolo | `1 − cs(q,r)` su punti (onset su griglia, semitono) | F4 | crolla con piccole differenze non rigide (F4); tratta trasposizioni e traslazioni come identità, il che qui è voluto |
| M4 | Distanza d'edit normalizzata per ruolo | Levenshtein su token (Δonset su griglia, intervallo, classe di durata), divisa per la lunghezza massima, con costi dichiarati prima della misura | F3 (AM) e inferenza | pesi arbitrari da dichiarare; F3 non letto per intero |

**Esiti della misura di diversità** (inferenza; fissati prima della misura; precisazione DIF-5.0-05 del 2026-10-26). Si tengono separate due conclusioni: la **diversità osservata** sul campione, che è sempre una misura, e la **generalizzazione** della calibrazione degli onset ad altri contenuti, che può essere inconcludente.

- **Precondizioni:**
  - il controllo positivo del fingerprint passa;
  - le pianificazioni sono valide;
  - il numero minimo di seed N e il numero minimo k di fingerprint distinti sono dichiarati prima della misura.
- **Tutti gli N seed producono lo stesso fingerprint:**
  - la diversità osservata è `MEASURED` e **nulla**: un **risultato negativo significativo**, registrato come finding sulla varietà del generatore («nelle condizioni X il seed non cambia il contenuto»), non nascosto dietro un esito inconcludente;
  - la generalizzazione della calibrazione a contenuti diversi (D-M2-16, insieme di verifica 1) è `INCONCLUSIVE`, perché il campione non offre contenuti diversi;
  - M2–M4 sono nulle per costruzione e si riportano come tali.
- **Almeno due fingerprint distinti, ma meno di k:**
  - le distanze M1–M4 sono `MEASURED` e possono essere diverse da zero; si riportano per intero, con il flag «campione insufficiente per una conclusione robusta»;
  - la generalizzazione della calibrazione si valuta sui soli seed distinti, con lo stesso flag.
- **Una quota di frasi identiche oltre la soglia dichiarata:** la diversità è `MEASURED` e bassa; la quota si riporta, anche come possibile effetto di confine della quantizzazione.
- **Almeno k fingerprint distinti:** si riportano le distribuzioni di M1–M4, senza soglie di qualità. Le fonti non danno soglie (F5).
- **N, k, griglia e pesi** si fissano nella review di 5e, prima di eseguire la misura.
- **Riproducibilità separata:** stesso seed → stesso hash di provenienza. Un fingerprint uguale fra seed diversi non è una prova di determinismo.

## 3. Q2 — Separazione della ground truth nella valutazione con agenti

### 3.1 Fonti

| ID | Fonte | Verifica | Contenuto pertinente |
|---|---|---|---|
| S1 | O. Sainz et al., «NLP Evaluation in trouble: On the Need to Measure LLM Data Contamination for each Benchmark», Findings of EMNLP 2023. [DOI 10.48550/arXiv.2310.18018](https://doi.org/10.48550/arXiv.2310.18018) | AM | La contaminazione peggiore è un modello «trained on the test split of a benchmark», che sovrastima le prestazioni. |
| S2 | S. Golchin, M. Surdeanu, «Time Travel in LLMs: Tracing Data Contamination in Large Language Models», ICLR 2024. [DOI 10.48550/arXiv.2308.08493](https://doi.org/10.48550/arXiv.2308.08493) | AM | Rilevazione della contaminazione per istanza e per partizione. |
| S3 | C. Xu, S. Guan, D. Greene, M-T. Kechadi, «Benchmark Data Contamination of Large Language Models: A Survey», arXiv:2406.04244, 2024 (preprint). [DOI 10.48550/arXiv.2406.04244](https://doi.org/10.48550/arXiv.2406.04244) | AM | Definizioni e mitigazioni della contaminazione dei benchmark. |
| S4 | S. Von Arx, L. Chan, B. Barnes (METR), «Recent Frontier Models Are Reward Hacking», 5 giugno 2025. [metr.org](https://metr.org/blog/2025-06-05-recent-reward-hacking) | FT (post) | Agenti con accesso all'ambiente ottengono «an existing implementation or answer» usata per controllarli. o3 sfrutta la funzione di scoring in 39 run RE-Bench su 128 (30,4 %). Le istruzioni contro l'imbroglio servono poco (80 % → 80 % con «do not cheat»). |
| S5 | Z. Zhong, A. Raghunathan, N. Carlini, «ImpossibleBench: Measuring LLMs' Propensity of Exploiting Test Cases», arXiv:2510.20270, ICLR 2026. [DOI 10.48550/arXiv.2510.20270](https://doi.org/10.48550/arXiv.2510.20270) | FT (circa 88 % dell'HTML) | Nascondere i test «reduces cheating success rate to near zero». Un'opzione esplicita di abbandono riduce l'imbroglio (GPT-5 dal 54 % al 9 %). I monitor basati su LLM rilevano solo in parte gli imbrogli nei compiti complessi. |
| S6 | SWE-bench, issue #465 «Repo State Loopholes During Agentic Evaluation», 3 settembre 2025. [GitHub](https://github.com/SWE-bench/SWE-bench/issues/465) | FT | Gli agenti leggevano la storia futura del repository e trovavano la soluzione; i maintainer hanno rimosso la storia futura. |
| S7 | ITU-R BS.1534-3 (10/2015), MUSHRA. [ITU](https://www.itu.int/rec/R-REC-BS.1534-3-201510-I/en) | FT | Metodo «double-blind multi-stimulus» con riferimento e ancore nascosti; ordine di presentazione casuale; esclusione dei valutatori che sbagliano il riferimento nascosto oltre soglie dichiarate (§4.1.2). |
| S8 | ITU-R BS.1116-3 (02/2015). [ITU](https://www.itu.int/rec/R-REC-BS.1116-3-201502-I/en) | AM | Metodo «double-blind triple-stimulus with hidden reference». |
| S9 | S. Hopewell et al., «CONSORT 2025 statement», *BMJ* 2025;389:e081123. [DOI 10.1136/bmj-2024-081123](https://doi.org/10.1136/bmj-2024-081123) | FT della checklist estesa ufficiale (la pagina BMJ ha risposto 403; il DOI viene da metadati di repository) | Occultamento dell'allocazione (item 18–19: contenitori opachi e sigillati, lista custodita separatamente); chi è in cieco (item 20); verifica e segnalazione delle compromissioni (item 21a). |
| S10 | L. Zheng et al., «Judging LLM-as-a-Judge with MT-Bench and Chatbot Arena», NeurIPS 2023 Datasets and Benchmarks. [DOI 10.48550/arXiv.2306.05685](https://doi.org/10.48550/arXiv.2306.05685) | AM | Bias di posizione, di verbosità e di auto-preferenza nei giudici LLM. |
| S11 | P. Wang et al., «Large Language Models are not Fair Evaluators», ACL 2024. [DOI 10.48550/arXiv.2305.17926](https://doi.org/10.48550/arXiv.2305.17926) | AM | La classifica cambia alterando l'ordine; propone una calibrazione a posizioni bilanciate. |
| S12 | L. Shi et al., «Judging the Judges: A Systematic Study of Position Bias in LLM-as-a-Judge», AACL-IJCNLP 2025. [DOI 10.48550/arXiv.2406.07791](https://doi.org/10.48550/arXiv.2406.07791) | AM | Il bias di posizione non è casuale. Metriche: stabilità alla ripetizione e coerenza di posizione. |
| S13 | C.-Y. Kuan, W.-P. Huang, H. Lee, «Understanding Sounds, Missing the Questions», Interspeech 2024 (già HALL nel dossier CACP). [DOI 10.48550/arXiv.2406.08402](https://doi.org/10.48550/arXiv.2406.08402) | AM | I modelli audio-linguistici sbagliano le domande discriminative sì/no, cioè lo stesso formato delle affermazioni «ruolo presente». |

**Non verificate e non usate:** il survey arXiv:2502.14425 (visto solo nei risultati di ricerca); le percentuali di leakage su SWE-bench riportate da blog.

### 3.2 Analisi

**Misure sostenute dalle fonti.** Il passaggio dalle fonti umane (S7–S9) agli agenti è un'analogia metodologica.

- **Separazione fisica `public/` / `sealed/`:**
  - analoghi: contenitori sigillati e lista custodita separatamente (S9); test nascosti, che portano l'imbroglio quasi a zero (S5); rimozione della storia futura (S6);
  - S4 mostra che gli agenti cercano e usano davvero la risposta del checker quando è raggiungibile.
- **ID opachi e ordine mescolato:** ordine casuale (S7); contromisura al bias di posizione (S10–S12). Inferenza: il seed di mescolamento sta in `sealed/`, oppure nel pacchetto pubblico solo come impegno crittografico (hash). In chiaro permetterebbe di ricostruire l'ordine.
- **Riferimenti tratti da finestre diverse:** coerente con la logica riferimento noto / stimoli nascosti di S7. È un'inferenza: nessuna fonte tratta il caso.

**Rischi residui** (inferenza, salvo dove indicato):

1. **Rigenerazione della ground truth:** un agente con accesso al repository o al binario può rieseguire il planner (analogo di S4 e S6). La separazione dei file non basta.
2. **Leakage dai metadati:** nomi e dimensioni dei file, durate, timestamp, chunk WAV e metadati PNG, legende con i nomi dei ruoli, tick ed epoch nel manifest pubblico.
3. **Dichiarazione degli input:** l'hash degli input dimostra solo ciò che il valutatore dichiara, non ciò che ha letto con altri strumenti.
4. **Errori del valutatore indipendenti dal leakage:** tendenza al «sì» e allucinazioni sulle domande binarie (S13); bias di posizione (S11, S12).
5. **Istruzioni:** le istruzioni contro l'imbroglio sono quasi inefficaci (S4), e i monitor basati su LLM sono deboli (S5).

### 3.3 Controlli aggiuntivi raccomandati

Da fissare nella review di 5d. Le misure di [ADR-0006](../adr/0006-m2-4-inc5-entry-decisions.md) D-M2-15 ne sono il nucleo.

- **Controlli negativi bilanciati:** finestre con ruolo assente, in numero comparabile alle positive, analoghe al riferimento e alle ancore nascoste di S7. Si riportano accuratezza bilanciata, specificità e falsi positivi, non solo il recall.
- **Duplicati nascosti:** la stessa finestra sotto due ID e in posizioni diverse, eventualmente con una perturbazione minima (gain, pochi ms) che ne cambia l'hash. Misurano coerenza e bias di posizione (S11, S12).
- **Catch trial con regola d'esclusione dichiarata prima della misura:** analogo del post-screening di S7. Le soglie CACP si decidono e si versionano; non si copiano da MUSHRA.
- **Item a verità alterata:** un ruolo rimosso dall'audio ma presente nel piano, o viceversa. Smascherano un valutatore che segue il piano invece dell'audio (analogo diretto di S5).
- **Esito esplicito «non determinabile»:** riduce l'imbroglio (S5); si conta a parte, mai come successo.
- **Isolamento e registro degli accessi:**
  - il valutatore lavora con il solo `public/` montato, senza repository, binari o rete, quando il client lo consente;
  - il risultato v0 dichiara strumenti e accessi;
  - senza isolamento la valutazione non è «alla cieca» e lo si registra (analogo dell'item 21a di S9).
- **Pacchetto rigenerato per sessione:** il seed di selezione e mescolamento viene da uno stream RNG isolato, distinto dal seed musicale. Ogni sessione di valutazione riceve ID e ordine nuovi.
- **Pulizia e audit dei metadati:**
  - WAV con soli chunk `fmt` e `data`;
  - PNG senza chunk testuali;
  - nomi normalizzati;
  - nessuna legenda con i nomi dei ruoli;
  - scansione automatica di `public/` per nomi dei ruoli, tick, epoch ed eventId.
- **Confronto con baseline:** l'agente si confronta con un classificatore banale (sempre «presente», oppure la frequenza a priori per ruolo) e con il descrittore deterministico (Side/Mid, onset degli stem). Un'affermazione aggregata vale solo se supera la baseline banale, con un intervallo di confidenza.

**Limite:** nessuna fonte verificata tratta agenti che valutano audio o spettrogrammi contro una ground truth sigillata. Il disegno è un'inferenza ragionata, da validare in 5d e 5e con catch trial e item a verità alterata.

## 4. Q3 — Onset per ruolo sugli stem

### 4.1 Verifica sul codice e base esistente

**Inviluppi per ruolo** (`VoiceAllocator`, [voice_allocator.cpp](../../engine/audio/voice_allocator.cpp), alla data):

| Ruolo | Attacco | Note |
|---|---|---|
| Pulso | 1 ms | sweep e click |
| Corpo | 15 ms | |
| Trama | 20 ms | FM, micro-delay 11/17 ms |
| Vettore | 5 ms | glide esponenziale con τ 15 ms e vibrato |

- Ogni NoteOn riattiva l'inviluppo, anche per Vettore, dove il glide parte dall'altezza precedente. **Oggi non esistono note legate senza nuovo attacco.**
- **La Trama ha l'attacco più lento**, non uno percussivo. Questo contribuisce plausibilmente alla recall di 0,31 sul master (E1), insieme al bus a −10 dB. È un'inferenza da verificare sugli stem in 5c.
- **Convenzione temporale** dello script di E1 ([cacp_methods_experiment.py](experiments/cacp_methods/cacp_methods_experiment.py)): il tempo di un frame è `(k·hop + N/2)/fs`, cioè il centro di una finestra che inizia a `k·hop`. È la convenzione «centre» di Dixon (O2 qui sotto). Con la convenzione d'inizio frame l'errore atteso sarebbe circa −21 ms, non −8.
- **Base esistente:**
  - Bello et al. 2005 (ONSET) e mir_eval (MIREVAL) sono già nel dossier CACP;
  - Danielsen et al. 2024 è nella ricerca approfondita: l'onset percepito dipende da attacco e timbro;
  - il protocollo §5 propone 5 ms / 240 frame per Pulso come candidato da profilare.

### 4.2 Fonti

| ID | Fonte | Verifica | Contenuto pertinente |
|---|---|---|---|
| O1 | J. P. Bello et al., «A Tutorial on Onset Detection in Music Signals», *IEEE TSAP* 13(5), 1035–1047, 2005. [DOI 10.1109/TSA.2005.851998](https://doi.org/10.1109/TSA.2005.851998) | FT (copia aperta segnalata da Semantic Scholar; nel dossier CACP era AM) | Onset come istante scelto per marcare un transiente esteso. I metodi a energia perdono robustezza con «amplitude modulations (i.e., vibrato, tremolo)». La scelta del metodo dipende dal tipo di segnale (intonato o percussivo). Finestra di 50 ms giustificata dall'imprecisione delle annotazioni. |
| O2 | S. Dixon, «Onset Detection Revisited», DAFx-06, 133–137, 2006. [Archivio DAFx](https://dafx.de/paper-archive/2006/papers/p_133.pdf) | FT | Frame centrato su `hn`; tolleranza di 50 ms; P, R e F. Il peak picking (media + δ) pesa molto sul rapporto fra falsi positivi e falsi negativi. Training e test vanno separati «in order to avoid overfitting»; i propri risultati sono «optimistic». |
| O3 | S. Böck, F. Krebs, M. Schedl, «Evaluating the Online Capabilities of Onset Detection Methods», ISMIR 2012, 49–54. [Archivio ISMIR](https://archives.ismir.net/ismir2012/paper/000049.pdf) | FT | Annotazioni entro 30 ms fuse in un onset; finestra di ±25 ms; abbinamento uno a uno «stretto». I rilevatori a flux anticipano: gli onset sono rilevati «one frame earlier» e riportati un frame dopo. |
| O4 | S. Böck, G. Widmer, «Maximum Filter Vibrato Suppression for Onset Detection» (SuperFlux), DAFx-13, 2013. [Archivio DAFx](https://dafx.de/paper-archive/2013/papers/09.dafx2013_submission_12.pdf) | FT | Un filtro massimo in frequenza riduce i falsi positivi da vibrato «by up to 60%». Gli onset soft hanno un attacco lungo: i metodi a energia e magnitudo «are not the best choice». Un lag maggiore avvicina i picchi all'onset reale. |
| O5 | G. C. Cawley, N. L. C. Talbot, «On Over-fitting in Model Selection and Subsequent Selection Bias in Performance Evaluation», *JMLR* 11, 2079–2107, 2010. [JMLR](https://www.jmlr.org/papers/v11/cawley10a.html) | AM | Ottimizzare la selezione su un campione finito produce overfitting e stime distorte, spesso grandi. |
| O6 | C. Raffel et al., «mir_eval», ISMIR 2014. [Archivio ISMIR](https://archives.ismir.net/ismir2014/paper/000320.pdf) | FT (nel dossier CACP era AM) | Abbinamento uno a uno. L'abbinamento greedy di MIREX è subottimo rispetto al matching bipartito massimo usato da mir_eval. |
| O7 | Codice di [mir_eval](https://github.com/mir-evaluation/mir_eval) (`onset.py`, `util.py`), [madmom](https://github.com/CPJKU/madmom) (`evaluation/onsets.py`, `audio/signal.py`) e [librosa](https://github.com/librosa/librosa) (`onset.py`) | codice letto sul ramo `main` il 2026-10-26; **commit non fissato**, da fissare se 5b ne riprende i dettagli | mir_eval: finestra di default 0,05 s, condizione `distance <= window`. madmom: finestra 0,025 s, fusione 0,03 s, parametro `delay` per calibrare uno scarto costante, origine del frame «center» o «left». librosa: `max_size` (filtro massimo, SuperFlux), `center`, `onset_backtrack`. |

**Non verificato direttamente:** il wiki MIREX sull'onset detection, perché il sito ha un certificato TLS scaduto. Le sue convenzioni sono riportate solo tramite O2 e O6.

### 4.3 Analisi e scelte

1. **Un solo rilevatore con profilo per ruolo, non quattro rilevatori diversi.**
   - *Nucleo comune:* quello di [ADR-0006](../adr/0006-m2-4-inc5-entry-decisions.md) D-M2-14.
   - *Profilo versionato per ruolo:* δ, scarto costante (parametro `delay`, come madmom, O7), larghezza del filtro massimo in frequenza (1 = flux classico; 3 bin = SuperFlux, O4) e lag.
   - *Varianti per ruolo:* entrano solo se la calibrazione le giustifica. Ogni grado di libertà in più aumenta il rischio di overfitting (O2, O5).
   - *Candidati attesi dall'analisi:* filtro massimo per Vettore (vibrato e glide, O4); per Trama e Corpo (attacco di 20 e 15 ms) eventualmente un lag maggiore o un rilevatore d'inviluppo con backtracking (O4, O7), da introdurre solo se la recall sugli stem lo richiede.
2. **Abbinamento a cardinalità massima, non greedy.** Il greedy può sottostimare P e R (O6). Si adotta il matching uno a uno a cardinalità massima con tolleranza inclusiva (`|Δ| ≤ finestra`, come O7). Su una retta con finestra simmetrica si calcola con due puntatori. Un test lo confronta con una ricerca esaustiva su casi piccoli.
3. **Ground truth per ruolo:**
   - frame dei NoteOn `Applied` del ruolo (D-M2-12);
   - NoteOn dello stesso ruolo entro 30 ms fusi in un onset di riferimento, convenzione di O3, O4 e O7. Con riferimenti sintetici esatti si tiene il primo NoteOn, non la media; la scelta è documentata.
   - note senza nuovo attacco (legato, pad sostenuti senza retrigger) da marcare come «nessun attacco atteso», escluse dal denominatore della recall e riportate a parte. Oggi l'insieme è vuoto (§4.1), ma il contratto lo prevede.
   - nessuno spostamento della ground truth: la latenza firmata (rilevato − NoteOn) si riporta per ruolo, come distribuzione.
4. **Tolleranze:** ±50 ms come valore primario, coerente con AC-AO-02, il dossier CACP e il default di mir_eval. ±25 ms come diagnostica più severa (O3, O4). Il candidato di 5 ms per Pulso del protocollo §5 si misura come latenza, non come finestra di abbinamento.
5. **Ordine di calibrazione e verifica** (O2, O5):
   - a. **Latenza intrinseca** del rilevatore su segnali sintetici per ruolo con onset noti al campione, generati dal synth reale con attacchi diversi. Si confronta con l'analisi della convenzione del frame.
   - b. **Calibrazione** di δ, scarto residuo ed eventuale variante sulla fixture con seed 1337. Il profilo si congela con versione e hash **prima** della verifica.
   - c. **Verifica senza ritocchi** su seed con fingerprint diverso, `planner_probe_v1` e segnali sintetici non usati in (a).
   - d. **Se la verifica fallisce:** si registra il fallimento. Una nuova calibrazione richiede un nuovo insieme di verifica, perché riusare quello vecchio è il bias descritto da O5. Il seed 1337 non è mai evidenza d'accettazione.
6. **Master e stem non sono confrontabili:** la recall della Trama sul master (0,31) misura il mascheramento nel mix, non la capacità del rilevatore. Si riporta separatamente.

## 5. Q4 — Stato dell'arte nei giochi con musica procedurale o adattiva

Integra lo stato dell'arte già raccolto (§0: Just Shapes & Beats, Audiosurf, Fournel GDC 2010, Talakat). I livelli **FT\*** indicano un testo completo letto tramite un estrattore automatico. Le citazioni sono quelle restituite dall'estrattore.

### 5.1 Fonti

| ID | Fonte | Verifica | Contenuto pertinente |
|---|---|---|---|
| G1 | A.-S. Mongeau, «Behind the Sound of No Man's Sky: A Q&A with Paul Weir on Procedural Audio», blog Audiokinetic, 11 aprile 2017. [Link](https://www.audiokinetic.com/en/blog/behind-the-sound-of-no-mans-sky-a-qa-with-paul-weir-on-procedural-audio) | FT | Pulse è «really just a glorified random file player»: strumenti e canvas con regole di frequenza, altezza, pan e volume. Il bilanciamento si ottiene conoscendo i limiti superiore e inferiore di ogni suono. |
| G2 | K. Jolly, «Usage of Pd in Spore and Darkspore», 4th Pure Data Convention, Weimar, 2011. [PDF](https://www.uni-weimar.de/kunst-und-gestaltung/wiki/images/Usage_of_Pd_in_Spore_and_Darkspore.pdf) | FT | Pd usato per la logica degli eventi. Selezione dei campioni «random, norepeat, or inorder». Il PDF raccomanda strumenti di visualizzazione dei dati e debugging. |
| G3 | R. Gould, «The Programmed Music of Mini Metro – Interview with Rich Vreeland», Designing Sound, 18 febbraio 2016. [Link](https://designingsound.org/2016/02/18/the-programmed-music-of-mini-metro-interview-with-rich-vreeland-disasterpeace) | FT\* | Il loop di gioco fa da sequencer; eventi quantizzati; limiti hard e failsafe sul volume di eventi. |
| G4 | J. Couture, «Turning road noise into music in Mini Motorways», Game Developer, 28 febbraio 2022. [Link](https://www.gamedeveloper.com/design/turning-road-noise-into-music-mini-motorways) | FT\* | Simulazione registrata e rigiocata in modo deterministico nell'editor; CI. Unico replay deterministico documentato in un sistema musicale procedurale fra quelli trovati. |
| G5 | M. Dealessandri, «When We Made… Ape Out», MCV/Develop, 2 ottobre 2019. [Link](https://mcvuk.com/?p=178981) | FT\* | Batteria simulata colpo per colpo. Pattern con rating di densità e di similarità: con l'intensità si scelgono pattern simili ma più densi. |
| G6 | R. Clark, «Game Design Deep Dive: Finding the beat in Crypt of the NecroDancer», Game Developer, 17 settembre 2014. [Link](https://www.gamedeveloper.com/audio/game-design-deep-dive-finding-the-beat-in-i-crypt-of-the-necrodancer-i-) | FT\* | Log dello scarto di ogni mossa dal beat. Autocalibrazione per giocatore, limitata a mezzo beat. |
| G7 | Awe Interactive, «Syncing Music and Gameplay in BPM: Bullets Per Minute», blog Audiokinetic, 13 gennaio 2022. [Link](https://www.audiokinetic.com/en/blog/syncing-music-and-gameplay-in-bpm) | FT | Bullet hell ritmico. Ricalibra il clock di gioco sul beat della musica quando lo scarto supera 10 ms. Ha scartato la musica procedurale per rischio di qualità, scegliendo «just the beat»; SFX con almeno 4 varianti contro la ripetizione. |
| G8 | B. Crecente, «Hi-Fi RUSH was inspired by Shaun of the Dead and Futurama», Unreal Engine, 16 marzo 2023. [Link](https://www.unrealengine.com/developer-interviews/hi-fi-rush-was-inspired-by-shaun-of-the-dead-and-futurama) | FT | Beat distribuiti a ogni oggetto. Debug UI a forma di pentagramma 4/4 per vedere azioni e beat. Cutscene scritte in tempo musicale. |
| G9 | KRAFTON Help Center, «Hi-Fi RUSH – Frame Rate and Music Sync Information», senza data. [Link](https://support.krafton.com/hc/en-us/articles/52370830105625) | FT | Un frame rate stabile è «essential for proper synchronization». |
| G10 | Audiokinetic, «Wwise SDK 2025.1.12 — Music Callbacks». [Link](https://www.audiokinetic.com/en/library/edge/?source=SDK&id=soundengine_music_callbacks.html) | FT | Callback di beat e battuta inviate quando il buffer passa all'hardware; «this delay is platform-dependent». |
| G11 | Firelight, «FMOD Studio API 2.03 — Studio::EventInstance». [Link](https://www.fmod.com/docs/2.03/api/studio-api-eventinstance.html) | FT | Callback `TIMELINE_BEAT` con battuta, beat, posizione in ms, tempo e metro. |
| G12 | Epic Games, «Quartz Overview» (UE 4.27). [Link](https://docs.unrealengine.com/4.27/en-US/WorkingWithAudio/Quartz/) | FT | Comandi quantizzati sul thread di render audio. Latenza dei buffer fino a 43 ms (2048 campioni a 48 kHz), 56 ms nel caso peggiore. Lo scheduling in anticipo compensa il ritardo. |
| G13 | Unity, «AudioSource.PlayScheduled». [Link](https://docs.unity3d.com/ScriptReference/AudioSource.PlayScheduled.html) | FT\* | Scheduling sul clock DSP, con 100–200 ms d'anticipo consigliati. |
| G14 | S. Parkin, «Oral history of Rez recounts a marriage of game and music», Game Developer, 17 marzo 2016, sul talk GDC 2016 di Mizuguchi. [Link](https://www.gamedeveloper.com/audio/oral-history-of-i-rez-i-recounts-a-marriage-of-game-and-music) | FT\* (secondaria rispetto al talk) | La quantizzazione dei suoni del giocatore è il nucleo dichiarato. Griglia e implementazione non sono descritte. |
| G15 | K. Collins, «An Introduction to Procedural Music in Video Games», *Contemporary Music Review* 28(1), 5–15, 2009. [DOI 10.1080/07494460802663983](https://doi.org/10.1080/07494460802663983) | AM | Distinzione fra algoritmi trasformazionali e generativi. |
| G16 | D. Plans, D. Morelli, «Experience-Driven Procedural Music Generation for Games», *IEEE TCIAIG* 4(3), 192–198, 2012. [DOI 10.1109/TCIAIG.2012.2212899](https://doi.org/10.1109/TCIAIG.2012.2212899) | AM | I loop a eventi possono produrre musica troppo ripetitiva; propone una generazione guidata da metriche di gioco. |
| G17 | M. Scirea, J. Togelius, P. Eklund, S. Risi, «Affective evolutionary music composition with MetaCompose», *Genetic Programming and Evolvable Machines*, 2017. [DOI 10.1007/s10710-017-9307-y](https://doi.org/10.1007/s10710-017-9307-y); [PDF aperto](https://researcher.itu.dk/ws/files/82428771/metacompose_journal.pdf) | FT | Ablazione: ogni componente sostituito da generazione «broken», con confronti a coppie su 298 partecipanti e test binomiale. Archivio con distanza armonica (TPSD) per gestire la diversità. |
| G18 | D. Herremans, C.-H. Chuan, E. Chew, «A Functional Taxonomy of Music Generation Systems», *ACM Computing Surveys* 50(5), 2017. [DOI 10.1145/3108242](https://doi.org/10.1145/3108242) | AM | Tassonomia per funzione. |
| G19 | P. Hutchings, J. McCormack, «Adaptive Music Composition for Games», *IEEE Transactions on Games* 12(3), 270–280, 2020. [DOI 10.1109/TG.2019.2921979](https://doi.org/10.1109/TG.2019.2921979) | AM | Sistema multi-agente; immersione riportata superiore alle colonne sonore originali in due giochi. |
| G20 | G. Smith, J. Whitehead, «Analyzing the Expressive Range of a Level Generator», PCG Workshop, FDG 2010. [DOI 10.1145/1814256.1814260](https://doi.org/10.1145/1814256.1814260); [PDF](https://www.pcgworkshop.com/archive/smith2010analyzing.pdf) | FT | 10.000 livelli, metriche fissate e istogrammi 2D, per «expose unexpected biases in the generation algorithm». |
| G21 | K. Worrall, «Expressive Performance Rendering for Reduced Music Repetition in Video Games», tesi PhD, University of York, 2025. [White Rose eTheses](https://etheses.whiterose.ac.uk/38684/) | AM | La variazione espressiva è meno percepibile durante il gioco che in ascolto isolato. |
| G22 | V. Dutta, «The Repetition of Video Game Music…», tesi dottorale, University of Huddersfield, 2020. [Link](https://pure.hud.ac.uk/en/studentTheses/the-repetition-of-video-game-music-its-impact-on-video-game-enjoy/) | AM | I giocatori tollerano la ripetizione, che ha anche funzione di feedback; l'eccessiva familiarità riduce il piacere. |
| G23 | B. Weck et al., «MuChoMusic: Evaluating Music Understanding in Multimodal Audio-Language Models», ISMIR 2024. [arXiv:2408.01337](https://arxiv.org/abs/2408.01337) | AM | I modelli audio-linguistici mostrano «an over-reliance on the language modality». |
| G24 | K. Kilgour et al., «Fréchet Audio Distance», Interspeech 2019. [DOI 10.21437/Interspeech.2019-2219](https://doi.org/10.21437/Interspeech.2019-2219) | AM | Distanza fra distribuzioni di embedding; correlazione con la percezione di 0,52. |

**Non verificati, quindi non usati:**

- contenuti dei talk GDC su No Man's Sky, Mini Metro, Thumper e Hi-Fi Rush (2024), perché GDC Vault era in manutenzione;
- un talk GDC 2023 su Hi-Fi Rush dedicato alla sincronizzazione, che non è stato trovato (esistono un talk CEDEC 2023 e uno GDC 2024 di game design);
- il dato «1/100 dei pattern per partita» di Ape Out;
- dettagli tecnici di Child of Eden, Tetris Effect, Thumper, Everyday Shooter, Beat Hazard, Soundodger, Sayonara Wild Hearts, FRACT OSC e Metal: Hellsinger;
- la tesi di Prechtl (2016), il cui repository ha risposto 403;
- il testo completo di G15, G16, G18 e G19.

### 5.2 Sintesi

- **Varietà.** La prassi documentata è **costruttiva**:
  - pool di varianti con regole anti-ripetizione (G1, G2);
  - rating di densità e similarità (G5);
  - mappatura stato di gioco → musica con quantizzazione e limiti hard (G3);
  - varianti multiple degli effetti (G7).

  Nessuna fonte industriale trovata **misura** la diversità fra seed o partite. Il metodo quantitativo più vicino viene dalla generazione procedurale di livelli: l'*expressive range* di G20.
- **Ripetizione.** Nel gioco la variazione è meno percepita che in ascolto isolato (G21). La ripetizione ha anche funzione di feedback (G22). Un bullet hell ritmico ha scelto una musica non procedurale proprio per qualità e leggibilità (G7). Questo coincide con il criterio già fissato nella ricerca approfondita: conta che la musica «ricorda, sviluppa, sorprende e ritorna», non l'infinità statistica dell'output.
- **Sincronismo.** La prassi è un clock audio autoritativo con scheduling in anticipo (G12, G13) e una riconciliazione con tolleranza (10 ms in G7).
  - Wwise distingue esplicitamente la notifica (buffer passato all'hardware) dall'ascolto, con un ritardo dipendente dalla piattaforma (G10). È la stessa distinzione di AGENTS §3 fra frame generati e audio udito.
  - In Hi-Fi Rush i cali di frame rate causano desync (G9). Il passo fisso e il clock musicale canonico di CYMATICA evitano questa classe di errore per costruzione.
- **Verifica.** Le verifiche documentate sono:
  - log degli scarti (G6);
  - debug UI a pentagramma (G8);
  - replay deterministico in CI (G4);
  - playtest.

  Nessuna fonte pubblica descrive un controllo automatico evento per evento fra piano e audio, né test di regressione dell'output musicale procedurale. È un'area poco documentata, non necessariamente inesplorata.
- **Valutazione con modelli.** L'eccessiva dipendenza dei modelli audio-linguistici dal testo (G23) conferma che la ground truth testuale va tenuta fuori dal valutatore (§3). L'ablazione di G17 offre un controllo di discriminazione.

## 6. Ricadute su ADR-0006

| Decisione | Ricaduta | Fonti |
|---|---|---|
| D-M2-12 | Due identità (provenienza e contenuto). Fingerprint su note complete, con il rilascio esplicito associato a ciascun NoteOn (DIF-5.0-02) e codifica con liste prefissate dalla lunghezza; partizione `E_exp = E_sent ⊎ E_canceled` per gli eventi cancellati prima dell'invio (DIF-5.0-R2-01); campi, quantizzazione, valori speciali e ordine definiti. Controlli positivi e negativi. Disposizioni `Applied`/`Ignored`/`Dropped`. | §2.3; F6; codice verificato |
| D-M2-12 | La catena O3 esatta non ha un equivalente pubblicato: la prassi è una riconciliazione con tolleranza. Si presenta come controllo più forte della prassi, non come prassi standard. | §5.2; G6, G7, G10 |
| D-M2-13 | Stem stereo; ricostruzione confrontata con il mix dei bus. | INC5-REV-04; E3 |
| D-M2-14 | Abbinamento a cardinalità massima; tolleranza ±50 ms primaria e ±25 ms diagnostica; fusione dei riferimenti entro 30 ms; profilo per ruolo con varianti solo se calibrate; latenza intrinseca misurata su segnali sintetici. | §4.3; O2–O7 |
| D-M2-15 | Modalità `blind` e `diagnostic` con visibilità dei metadati diversa (DIF-5.0-03); `public/` e `sealed/`; affermazioni tipizzate `claims_v0` (DIF-5.0-04), con predicati distinti per presenza del segnale, attacco rilevato e udibilità, quest'ultima mai resa vera da una misura (DIF-5.0-R2-02); limite procedurale dichiarato. Controlli aggiuntivi da fissare nella review di 5d: negativi bilanciati, duplicati nascosti, catch trial, item a verità alterata, esito «non determinabile», registro degli accessi, pulizia dei metadati, baseline banale. | §3.3; S4–S13, G23 |
| D-M2-15 (proposta per 5d) | Controllo di discriminazione per ablazione: confronto in cieco fra piano completo e varianti ablate. Se l'agente non le distingue, il suo giudizio di qualità non è informativo. | G17 |
| D-M2-15 (proposta per 5d) | Vista «a pentagramma» con intent, eventi `Applied` e onset rilevati per ruolo, da ispezionare in modo multimodale. Completa i manifest, non li sostituisce. | G6, G8 |
| D-M2-16 | Diversità esplorativa in stile *expressive range*: descrittori fissati prima, insieme di seed dichiarato, distribuzioni e istogrammi invece di un solo numero; diversità nulla registrata come risultato negativo, generalizzazione `INCONCLUSIVE` (DIF-5.0-05). La diversità non è un gate e non va massimizzata. | §2.3; G20, G21, G22; F5 |
| D-M2-16 | FAD escluso da Inc-5: richiede embedding e riferimenti, e la correlazione percettiva è moderata. | G24 |

## 7. Limiti

- **Fonti umane come analogie:** le fonti sui test d'ascolto umani (S7–S9) sono analogie di metodo, non norme applicabili agli agenti.
- **Fonti non lette per intero:** F3 è verificato solo su abstract e metadati; S1–S3 e S10–S12 pure.
- **Definizioni proposte:** fingerprint, metriche ed esiti della misura di diversità sono proposte, non decisioni validate. I parametri (N, k, griglia, pesi) si fissano prima della misura, nella review della fase che li usa.
