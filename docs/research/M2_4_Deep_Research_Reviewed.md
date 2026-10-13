# CYMATICA M2.4 — Composizione e sintesi procedurale di qualità artistica

> **Status:** ricerca specialistica revisionata, **non normativa**. Le proposte non modificano automaticamente [Spec §16/§32](../../CYMATICA_Specifica_Agentica_Sviluppo.md), [DESIGN](../../DESIGN.md) o [ADR-0003](../adr/0003-m2-decisions-contracts.md).
> **Baseline del repository:** commit [`460bc286378b473218bc07f751e3615964626a11`](https://github.com/naturewhisp/cymatica/commit/460bc286378b473218bc07f751e3615964626a11), 13 ottobre 2026.
> **Provenienza:** rapporto Deep Research fornito dal titolare; revisione editoriale e verifica a campione di fonti primarie/DOI; citazioni di sessione sostituite con collegamenti durevoli. Non è una meta-analisi e non certifica accesso al full text di ogni articolo.
> **Documenti di accompagnamento:** [revisione critica](M2_4_Research_Review.md), [piano operativo](M2_4_Implementation_Plan.md), [protocollo sperimentale](M2_4_Evaluation_Protocol.md), [roadmap proposta](../roadmap_music_research_addendum.md).


## Quadro di partenza, baseline verificata e lacune residue

La lettura del repository porta a una conclusione importante: **CYMATICA non ha bisogno di un altro “generatore procedurale di note”**, perché la specifica ha già preso molte delle decisioni architetturali corrette. Il modello temporale è gerarchico — sample → subdivision → beat → bar → phrase → section → run form — e musica, gameplay e visualizzazione devono derivare dallo stesso intento simbolico, non da un'analisi ex post del segnale audio. I quattro ruoli Pulso, Corpo, Trama e Vettore sono già definiti come funzioni musicali e ludiche complementari. ([Spec §16/§20/§32](../../CYMATICA_Specifica_Agentica_Sviluppo.md))

La specifica §16 è altrettanto esplicita sul problema artistico: la musica procedurale deve costruire **continuità, memoria e forma**; note casuali purché appartenenti a una scala non sono considerate sufficienti. Sono già previsti form planner, motif memory, trasformazioni, ritmi euclidei, progressioni armoniche finite, orchestrazione sui quattro ruoli e sintesi ibrida. La callback audio deve soltanto eseguire eventi pianificati anticipatamente, mentre adattamenti quali densità, timbro e probabilità di variazione riguardano esclusivamente il futuro ancora mutabile. ([Spec §16/§20/§32](../../CYMATICA_Specifica_Agentica_Sviluppo.md))

`DESIGN.md` impone inoltre un criterio più forte della semplice correttezza musicale: ogni processo deve produrre una **personalità giocabile**, le run devono essere irripetibili ma non arbitrarie e l'identità deve sopravvivere alla variazione. Fra i pilastri sono già presenti “forma, memoria, ritorno”; Pulso deve rendere il tempo prevedibile, Corpo dare struttura, Trama densità e Vettore direzione espressiva. Non tutti i ruoli devono diventare pericolosi simultaneamente. Sul piano audio, il documento chiede esplicitamente forma, motivi riconoscibili, micro-variazione, ritorni, mix stabile e funzione ludica, non un accompagnamento che sembri “MIDI casuale”. ([DESIGN](../../DESIGN.md))

ADR-0003 congela invece il contratto esecutivo. `MusicIntentEvent` contiene identificatore, `sampleFrame`, tick, ruolo, frequenza, velocity, durata e un parametro timbrico; Bjorklund opera su array statici fino a 32 step. Lo scheduler ha lookahead nominale di 150 ms, con intervallo previsto 100–250 ms, coda SPSC da 256 eventi, un solo producer e **otto voci preallocate, due per ruolo**, con voice stealing locale e micro-fade. ([ADR-0003](../adr/0003-m2-decisions-contracts.md)) Il record di avanzamento letto sul branch corrente colloca M2.1, M2.2 e M2.3 come completate e validate e identifica **M2.4 come prossimo passo**, precisamente per introdurre i generatori dei quattro ruoli, Bjorklund, synth a otto voci e due sezioni con transizione di tempo/metrica. ([Stato milestone](../progress.md))

Questa ricerca tratta pertanto come **baseline, non come temi da riscoprire**, gerarchia temporale, Music Intent condiviso, Euclidean rhythm, quattro ruoli, scheduler ahead-of-time, determinismo e callback bounded. Le vere lacune residue sono più specifiche:

| Lacuna residua | Perché è decisiva per la qualità artistica |
|---|---|
| Rappresentazione dell'identità tematica | Senza un oggetto “motivo” persistente, ripetizione e variazione rimangono effetti locali anziché memoria compositiva. |
| Grammatica di frase e funzione armonica | Una lista di accordi validi non produce automaticamente direzione, cadenza, aspettativa o risoluzione. |
| Voice-leading e coordinazione tra ruoli | Con due voci per ruolo, ogni nota deve avere una funzione; sovrapposizioni casuali consumano rapidamente il budget senza aumentare la qualità. |
| Modello esplicito di tensione e trasformazione | L'adattamento rischia altrimenti di equivalere a “più note quando è difficile”. |
| Dialetto timbrico CYMATICA | Una sintesi tecnicamente corretta ma genericamente “synth” non genera un'identità autoriale. |
| Corrispondenza strutturale musica–geometria | Il mapping beat→spawn è troppo debole rispetto alla promessa “attraversare la musica stessa”. |
| Protocollo di valutazione | Correttezza, varietà, preferenza, qualità artistica e originalità sono proprietà diverse e non possono condividere una singola metrica. |

La tesi centrale risultante è quindi questa: **per M2.4 la scelta più promettente non è un generatore più sofisticato statisticamente, ma un compositore gerarchico piccolo, esplicitamente autoriale e deterministico, basato su motivi persistenti, trasformazioni controllate, funzione armonica, voice-leading bounded e orchestrazione sparsa**. I modelli probabilistici diventano utili come generatori subordinati di variazione; i constraint solver come risolutori locali; i modelli neurali non sono necessari alla milestone. Questa conclusione è coerente sia con la letteratura sulla struttura musicale sia con l'architettura già congelata dal progetto. ([Conklin & Witten 1995](https://doi.org/10.1080/09298219508570672); [Anders & Miranda 2011](https://doi.org/10.1145/1978802.1978809); [Rohrmeier 2011](https://doi.org/10.1080/17459737.2011.573676); [Assayag & Dubnov 2004](https://doi.org/10.1007/s00500-004-0385-4))

La versione originale della Deep Research non disponeva delle fonti NotebookLM caricate nel progetto ChatGPT e lo dichiarava esplicitamente. Questa revisione editoriale ha successivamente esaminato alcuni materiali del corpus caricato (audio procedurale, process music, casi studio di giochi musicali e generazione bullet hell) per contestualizzare le conclusioni; non attribuisce risultati quantitativi a quel corpus. I contributi scientifici specifici rimangono legati alle pubblicazioni primarie citate. La selezione e le limitazioni dei materiali precedenti sono dettagliate in [M2_4_Research_Review.md](M2_4_Research_Review.md).

## Stato dell'arte e architettura compositiva raccomandata

La letteratura storica e moderna converge su un punto che per CYMATICA è particolarmente utile: **la coerenza emerge quando più scale temporali e più tipi di vincolo operano insieme**. Non esiste evidenza seria che una singola tecnica — Markov, grammatica, constraint programming, stochastic process, transformer o diffusion — sia universalmente superiore come “compositore”. Esistono invece metodi con punti di forza molto diversi.

Conklin e Witten mostrarono già nel 1995 che modelli a *multiple viewpoints* possono integrare rappresentazioni diverse del materiale musicale e combinare memoria a breve e lungo termine; il loro esperimento sui corali di Bach utilizzava la predittività/entropia come proprietà misurabile, pur riconoscendo che essa non equivale direttamente alla qualità artistica. ([Conklin & Witten 1995](https://doi.org/10.1080/09298219508570672)) Pearce e Wiggins hanno sviluppato questa linea in IDyOM: modelli n-gram a ordine variabile possono approssimare aspettative melodiche umane e produrre quantità come probabilità e information content, rendendo la sorpresa una variabile musicalmente interpretabile. ([Pearce & Wiggins 2012](https://doi.org/10.1111/j.1756-8765.2012.01214.x))

Il limite dei modelli puramente sequenziali è altrettanto ben documentato concettualmente. Rohrmeier mostra che la sintassi armonica tonale richiede relazioni gerarchiche di funzione, tonalità, grado e modulazione che una semplice tabella di transizioni locali non rappresenta bene. ([Rohrmeier 2011](https://doi.org/10.1080/17459737.2011.573676)) Anders e Miranda mostrano invece come il constraint programming sia adatto a formalizzare armonia, contrappunto, ritmo, forma e strumentazione perché permette di separare regole dichiarative e ricerca delle soluzioni; il rovescio della medaglia è che una ricerca generale non bounded è inappropriata per una callback realtime e va confinata al planner o sostituita da domini finiti/precalcolati. ([Anders & Miranda 2011](https://doi.org/10.1145/1978802.1978809))

Il paradigma del *factor oracle* costituisce una quarta famiglia interessante. Assayag e Dubnov dimostrarono l'uso realtime di una memoria sequenziale incrementale per apprendere e ricombinare materiale musicale; il lavoro successivo di Nika aggiunge “scenari temporali” proprio per risolvere la dialettica fra reattività locale e pianificazione globale. ([Assayag & Dubnov 2004](https://doi.org/10.1007/s00500-004-0385-4); [Nika 2016](https://jeromenika.com/research-designing-generative-agents/phd-thesis/)) Questa è una lezione importante per CYMATICA: una memoria generativa produce continuità, ma ha bisogno di una forma sovraordinata.

**Confronto delle architetture candidate.** I punteggi seguenti sono **stime qualitative editoriali non calibrate** per CYMATICA, non risultati sperimentali, benchmark né ranking universalmente valido. Non vanno utilizzati come pesi normativi per scegliere un algoritmo.

| Architettura | Forma lunga | Identità/memoria | Controllo armonico | Costo runtime | Determinismo | Rischio principale | Giudizio per CYMATICA |
|---|---:|---:|---:|---:|---:|---|---|
| **Grammatica gerarchica + motif memory + ricerca bounded** | 5/5 | 5/5 | 4/5 | basso | eccellente | regole troppo povere → meccanicità | **Core raccomandato** |
| **Multiple-viewpoint / variable-order Markov** | 2–3/5 | 4/5 | 2–3/5 | basso–medio | eccellente con modello/seed congelati | coerenza locale senza arco globale | Modulo di variazione futuro |
| **Constraint/weighted-CSP o beam search** | 4/5 | 3/5 | 5/5 | medio e potenzialmente variabile | eccellente | esplosione combinatoria | Ottimo risolutore armonico locale |
| **Factor oracle / memoria ricombinatoria** | 2–3/5 | 5/5 | 2/5 | basso | molto buono | ricombinazione senza teleologia | Ottimo per ritorni e improvvisazione futura |
| **Modello neurale end-to-end** | dipende dal modello | potenzialmente alta | difficile da garantire | alto/variabile | più oneroso da garantire | opacità, latency, dataset, validation | **Non giustificato per M2.4** |

La soluzione che massimizza il rapporto qualità/controllabilità è pertanto **un planner gerarchico a motivi con generazione locale vincolata**, non una scelta esclusiva fra grammatica, probabilità e constraints. La grammatica decide *che cosa sta facendo la frase*; la memoria di motivi decide *che cosa deve restare riconoscibile*; un piccolo generatore stocastico decide *quale variante usare*; il voice-leading/constraint layer decide *quali realizzazioni sono accettabili*.

Una pipeline concreta, compatibile con il repository corrente, sarebbe:

```text
FormPlan
  ↓
SectionPlan
  ↓
PhrasePlan
  ├── target tension / energy / density
  ├── harmonic function + cadence goal
  ├── motif IDs + allowed transformations
  └── dominant/support role policy
  ↓
Bounded Candidate Generator
  ├── rhythm realization
  ├── motif transformation
  ├── chord / voicing candidates
  └── orchestration candidates
  ↓
Hard constraints
  ↓
Deterministic aesthetic scoring
  ↓
Role realization
  ↓
MusicIntentEvent[]
  ↓
existing scheduler / audio engine
```

Questa stratificazione è particolarmente importante perché **non richiede di modificare subito `MusicIntentEvent`**: il contratto congelato può restare l'IR esecutivo “flattened”, mentre motivo, funzione armonica, tensione e struttura di frase vivono nel planner e vengono compilati in eventi correnti. È il modo meno rischioso di aumentare drasticamente la qualità compositiva senza trasformare M2.4 in una nuova milestone architetturale.

**Condizione di compatibilità da non trascurare:** M2.4 può mantenere i dati strutturali nel planner e generare l'IR audio esistente; ma il futuro generatore geometrico non può dedurre con certezza `motifId`, `phraseId` o `transformId` dal solo `MusicIntentEvent` flattenizzato. Prima dell'integrazione M3/M6 serve definire e versionare un **piano semantico condiviso**, oppure un sidecar immutabile con tali ID e coordinate temporali. Non è una modifica automatica a D-M2-02: è una decisione d'ingresso futura, soggetta ad ADR e replay/fairness test. ([ADR-0003](../adr/0003-m2-decisions-contracts.md))

**Identità tematica.** Il motivo non dovrebbe essere memorizzato come sequenza di frequenze assolute, ma come oggetto relativo: intervalli o gradi rispetto al centro corrente, durate rispetto alla griglia, accenti, profilo dinamico, eventuali note-obiettivo e insieme delle trasformazioni autorizzate. La memoria melodica dipende fortemente da relazioni di contorno e contesto, mentre i modelli probabilistici di aspettativa mostrano l'importanza del contesto sequenziale accumulato. ([Pearce & Wiggins 2012](https://doi.org/10.1111/j.1756-8765.2012.01214.x); [Sears et al. 2018](https://doi.org/10.1080/09298215.2017.1367010)) Per CYMATICA ciò permette a uno stesso “gesto” di sopravvivere a trasposizione, cambio di modo, orchestrazione o metrica.

Un motivo v1 può quindi essere concettualmente:

```text
Motif
    semantic_id
    interval_contour[]
    metric_positions[]
    duration_ratios[]
    accent_contour[]
    harmonic_anchors[]
    invariant_mask
    allowed_transforms
```

Le trasformazioni utili non devono essere tutte equivalenti. **Trasposizione, cambio di ottava, reorchestrazione e augmentation** preservano fortemente l'identità; inversione e mode-remapping la preservano moderatamente; frammentazione e sostituzione ritmica possono allontanarsene molto. Il sistema dovrebbe quindi distinguere trasformazioni *conservative*, *developmental* e *disruptive*, spendendo queste ultime soltanto quando la forma richiede vera novità. Questa classificazione è una proposta specifica per CYMATICA, da convalidare percettivamente.

**Tensione e risoluzione.** È sconsigliabile rappresentare la tensione come semplice “numero di note”. Farbood ha modellato giudizi continui di tensione tramite una combinazione temporale di armonia, altezza, aspettativa melodica, dinamica, densità degli onset, tempo, metro, regolarità e sincopazione; un modello puramente istantaneo era insufficiente rispetto a uno che considerava trend temporali. ([Farbood 2012](https://doi.org/10.1525/mp.2012.29.4.387)) Anche le cadenze producono aspettative di chiusura misurabili: in esperimenti con musicisti e non musicisti, differenti tipi cadenzali generavano profili distinti di aspettativa temporale. ([Sears et al. 2020](https://doi.org/10.1177/0305735618803676); [Sears et al. 2018](https://doi.org/10.1080/09298215.2017.1367010))

Per questo suggerisco un `CompositionalTensionState` interno multidimensionale — armonico, ritmico, registrale, densità, timbrico — da ridurre eventualmente a un target scalare solo per lo scoring. **Non va identificato con la Dissonanza/health del gameplay**: la specifica stessa impedisce di trasformare automaticamente la perdita di salute in note “sbagliate”. ([Spec §16/§20/§32](../../CYMATICA_Specifica_Agentica_Sviluppo.md)) Una run artisticamente interessante deve poter contenere una frase musicalmente tesa ma ludicamente recuperabile, o viceversa.

**Armonia.** Per M2.4 non serve un harmonizer generale. È più utile sostituire la nozione di “progressione casuale da una lista” con un piccolo **grafo di funzione armonica/modale**: stabilità/prolungamento → preparazione → tensione → risoluzione, con possibilità di elusione o sospensione. Rohrmeier fornisce la motivazione teorica per la struttura gerarchica; gli esperimenti sulle cadenze mostrano che funzione e chiusura influenzano effettivamente le aspettative degli ascoltatori. ([Rohrmeier 2011](https://doi.org/10.1080/17459737.2011.573676); [Sears et al. 2020](https://doi.org/10.1177/0305735618803676)) Questo non implica imitare il classicismo: il vocabolario può essere deliberatamente modale, quartale, pedale o simmetrico purché la grammatica di **stabilità e movimento** sia quella di CYMATICA.

**Voice-leading.** Qui la letteratura offre criteri particolarmente implementabili. Huron ha mostrato che buona parte delle regole tradizionali di condotta delle parti può essere ricondotta a principi percettivi di streaming e indipendenza delle voci, mentre Tymoczko rappresenta le successioni accordali come spostamenti geometrici fra configurazioni di pitch e osserva l'uso diffuso di trasformazioni a distanza relativamente breve. ([Huron 2001](https://doi.org/10.1525/mp.2001.19.1.1); [Tymoczko 2006](https://doi.org/10.1126/science.1126287)) CYMATICA può quindi evitare un corpus completo di regole scolastiche e usare un costo finito, per esempio:

\[
C =
w_m C_{\text{motion}}
+ w_r C_{\text{register}}
+ w_c C_{\text{crossing}}
+ w_d C_{\text{dissonance}}
+ w_f C_{\text{function}}
\]

con un numero piccolo e predefinito di voicing candidati. Questo rende il problema bounded, deterministico e interpretabile. Contrariamente a un vero CSP generale, il costo nel worst case può essere fissato a priori.

Il contrappunto dovrebbe essere usato come **principio percettivo**, non come esercizio di specie: movimento contrario/obliquo, separazione dei registri, gestione controllata delle collisioni e risoluzione delle dissonanze forti aiutano a distinguere Vettore e Trama; una proibizione dogmatica di parallele o dissonanze sarebbe invece artisticamente arbitraria per un'opera elettronica contemporanea. I risultati di Huron supportano proprio l'idea di distinguere principi percettivi fondamentali e opzioni stilistiche. ([Huron 2001](https://doi.org/10.1525/mp.2001.19.1.1))

**Ritmo.** L'algoritmo euclideo è un eccellente distributore di onset e Toussaint mostrò il rapporto fra distribuzioni “maximally even” e numerosi timeline rhythm tradizionali; ciò non significa che Bjorklund generi da solo groove, frase o forma. ([Toussaint 2005](https://www.archive.bridgesmathart.org/2005/bridges2005-47.html)) Per CYMATICA dovrebbe quindi essere un *operatore*, non il compositore ritmico completo:

\[
R =
\operatorname{MeterAccent}
\circ
\operatorname{EuclideanPattern}
\circ
\operatorname{PhraseMask}
\circ
\operatorname{MotifTransform}.
\]

Pulso può conservare un anchor fortemente metrico mentre Trama applica rotazioni, subdivisoni e poliritmie; Corpo può marcare cambi armonici meno frequenti; Vettore può attraversare la battuta con una frase che non replica semplicemente gli onset di Pulso.

È particolarmente importante **non equiparare “humanization” a jitter casuale**. In uno studio con 93 studenti di musica, Frühauf, Kopiez e Platz trovarono che il pattern quantizzato otteneva la valutazione di groove più alta e che deviazioni maggiori peggioravano il giudizio nel contesto testato. ([Frühauf et al. 2013](https://doi.org/10.1177/1029864913486793)) Una sintesi interdisciplinare del progetto TIME mostra inoltre che il tempo percepito dipende da attack, durata, timbro, centro spettrale, cultura e competenza: lo stesso onset fisico non implica lo stesso centro percettivo. ([Danielsen et al. 2024](https://doi.org/10.1525/mp.2024.41.3.176)) Perciò in M2.4 è preferibile **quantizzazione precisa + inviluppi/timbri ben progettati**; eventuali microtiming offsets devono diventare profili stilistici deterministici validati, non rumore gaussiano aggiunto a ogni nota.

**Process music e minimalismo.** Il saggio di Reich *Music as a Gradual Process* formalizza l'idea di processi compositivi percepibili, nei quali il mutamento è parte stessa dell'esperienza musicale. ([Reich 1968](https://doi.org/10.1093/acprof:oso/9780195151152.003.0004)) Per CYMATICA è una fonte estremamente fertile perché operazioni come rotazione, phase displacement, densificazione, sottrazione, augmentation e sostituzione progressiva sono simultaneamente musicali e geometriche. Ma il processo non dovrebbe sostituire la forma: usato da solo può produrre stasi o inevitabilità. La soluzione consigliata è **processo locale dentro una forma con esposizione, sviluppo, climax, recovery e ritorno**.

La stessa cautela vale per la composizione stocastica. La tradizione di Xenakis dimostra la potenza delle distribuzioni statistiche per masse, densità, texture e organizzazione temporale; il suo *Formalized Music* comprende stochastic music, Markov processes, sieves e dynamic stochastic synthesis. ([Xenakis, Formalized Music](https://www.worldcat.org/search?q=Xenakis+Formalized+Music)) Ma proprio per CYMATICA lo stocastico è più convincente come controllo della **texture e delle scelte subordinate** che come sostituto dell'identità tematica. La regola pratica è: maggiore è la scala temporale e il significato strutturale di una decisione, minore dovrebbe essere la sua arbitrarietà.

Il recente stato dell'arte non rovescia questa conclusione. Lavori 2026 come Calliphony mostrano che generazione simbolica realtime può essere controllata da segnali esterni su densità, pitch e attivazione dei layer, ma la pipeline utilizza un generatore e una DAW ed è una dimostrazione di interfaccia, non una prova che un modello neurale sia necessario per la qualità artistica sotto hard realtime. ([Wu, Yu & Xia 2026](https://nime.org/proc/nime2026_32/index.html)) Anche il dibattito 2026 sull'allineamento alle preferenze umane continua a indicare la coerenza lunga e l'affidabilità delle preferenze come problemi aperti, non risolti automaticamente dai modelli più recenti. ([Herremans & Roy 2026](https://doi.org/10.1609/aaai.v40i46.41323))

## Sintesi, orchestrazione e limite delle otto voci

La qualità timbrica di M2.4 dovrebbe essere affrontata con la stessa filosofia della composizione: **pochi meccanismi fortemente controllabili e combinabili sono preferibili a un synth universale molto complesso**. La letteratura storica sulla sintesi rimane direttamente rilevante. Chowning dimostrò che una FM semplice consente di controllare spettri complessi e soprattutto la loro evoluzione dinamica con pochissimi parametri. ([Chowning 1973](https://www.aes.org/e-lib/)) Välimäki e Huovilainen mostrano invece perché gli oscillatori sottrattivi digitali richiedono tecniche di antialiasing e individuano wavetable, BLIT/BLEP e altre famiglie di metodi band-limited o alias-reducing. ([Välimäki & Huovilainen 2007](https://doi.org/10.1109/MSP.2007.323276)) Karplus–Strong offre una struttura di feedback estremamente compatta per timbri percettivamente “fisici”, mentre una review AES del 2026 conferma che i digital waveguide rimangono significativamente più efficienti delle simulazioni PDE generiche e sono adatti a sistemi realtime. ([Karplus & Strong 1983](https://doi.org/10.2307/3680062); [Tablas de Paula et al. 2026](https://doi.org/10.17743/jaes.2026.0283))

Il confronto pratico è il seguente.

| Sintesi | Espressività | Costo relativo | Controllabilità procedurale | Criticità | Fit CYMATICA |
|---|---|---|---|---|---|
| **Sottrattiva band-limited** | alta per bassi, pad, lead | basso–medio | eccellente | aliasing se oscillatori ingenui; filtri nonlineari costano | **Molto alta** |
| **Wavetable band-limited** | alta, identità timbrica facilmente autoriale | basso | eccellente | memoria tabelle/interpolazione; morph da progettare | **Molto alta** |
| **FM a 2–3 operatori** | molto alta rispetto ai parametri | basso | eccellente | facilmente aspra/instabile se indici non limitati | **Molto alta** |
| **Karplus–Strong / waveguide semplice** | alta per pluck, corde, risonanze | basso | alta | tuning/decay e variazioni di pitch richiedono cura | **Alta, distintiva** |
| **Sampling ibrido** | altissima soprattutto sui transienti | CPU bassa, memoria maggiore | alta se asset piccolo e parametrizzato | licenze, asset management, pitch-shift | **Alta** |
| **Granulare** | eccellente per masse/Trama | medio–alto | altissima | numero di grain, envelope, memory traffic e scheduling | **Da differire o trattare come composite voice** |
| **Physical modeling PDE/modal complesso** | potenzialmente altissima | medio–molto alto | alta | stabilità e budget DSP | **Ricerca futura, non M2.4** |

Il valore della wavetable non è semplicemente “suona meglio”: offre una **superficie timbrica controllabile** in cui un singolo parametro morfologico può essere collegato a tensione, archetipo e gameplay senza cambiare algoritmo. Le tecniche di antialiasing sono mature e ben documentate. ([Välimäki & Huovilainen 2007](https://doi.org/10.1109/MSP.2007.323276); [Välimäki & Huovilainen 2006](https://doi.org/10.1162/comj.2006.30.2.19)) FM ha una proprietà complementare: poche modulazioni deterministiche possono produrre enormi cambiamenti di brightness e roughness, quindi `timbreParam` può diventare realmente significativo anziché un generico cutoff. ([Chowning 1973](https://www.aes.org/e-lib/))

Karplus–Strong è forse la tecnica più interessante da sperimentare come **firma CYMATICA**: il concetto stesso di risonanza, eccitazione, decadimento e feedback si sposa semanticamente con l'opera. L'algoritmo originale e le sue estensioni storiche dimostrano che un delay loop filtrato può generare corde e percussioni con costo modesto. ([Karplus & Strong 1983](https://doi.org/10.2307/3680062)) La review 2026 sui waveguide conferma che questa famiglia mantiene ancora oggi un rapporto particolarmente favorevole fra modellazione fisica e realtime. ([Tablas de Paula et al. 2026](https://doi.org/10.17743/jaes.2026.0283))

La granularità ha il problema opposto. Roads ha formalizzato la granular synthesis già nel 1978 come organizzazione di molti eventi microscopici. ([Roads 1978](https://doi.org/10.2307/3680222)) Artisticamente è perfetta per Trama, collasso, nubi e materia sonora; architetturalmente però “un grain = una voice” sarebbe incompatibile con un budget di otto voci. La via appropriata, se sperimentata, è un **grain-cloud processor interno a una singola voce DSP**, con massimo di grain attivi rigidamente bounded. Proprio perché ciò cambia il significato operativo della parola *voice*, non dovrebbe essere introdotto silenziosamente sotto D-M2-03.

Per M2.4 raccomando quindi una **sintesi differenziata per ruolo**, non quattro istanze identiche dello stesso synth:

| Ruolo | Motore consigliato M2.4 | Ragione artistica/tecnica |
|---|---|---|
| **Pulso** | transient sample opzionale + sine/noise/FM body | attacco preciso, leggibilità ritmica, bassissimo costo |
| **Corpo** | wavetable/subtractive; una voce bass + una voce harmonic shell | stabilità, massa, controllo del registro |
| **Trama** | wavetable/noise filtrato + delay; granular spike successivo | densità percepita senza consumare molte note |
| **Vettore** | wavetable/FM con glide, envelope e vibrato bounded | frase cantabile/direzionale e forte identità timbrica |

Delay e riverbero dovrebbero essere **bus globali**, non “voci musicali”, con parametri bounded. Saturazione, sidechain e limiter erano già indicati dalla specifica come parte della sintesi iniziale. ([Spec §16/§20/§32](../../CYMATICA_Specifica_Agentica_Sviluppo.md)) Questo è importante perché con otto voci la ricchezza dovrebbe venire anche dalla **trasformazione nel tempo di ogni voce**, non dalla semplice sovrapposizione di più oscillatori indipendenti.

Il limite 2+2+2+2 è musicalmente più severo di quanto sembri. Se “una voce” equivale a una nota indipendente, Corpo non può sostenere una triade e contemporaneamente un basso: due voci permettono al massimo una diade, oppure basso + una nota guida. Vettore non può fare accordi estesi senza sottrarre spazio alla frase; Trama deve evitare cluster realizzati nota per nota. Questo non rende impossibile musica ricca: **obbliga a orchestrare in modo distribuito**. Corpo può fornire fondamentale e guida, Trama aggiungere una nota armonica intermittente, Vettore suggerire estensioni melodiche e Pulso eventualmente contenere un transient accordato. L'armonia percepita è proprietà dell'insieme, non del solo ruolo Corpo.

Per M2.4 questo vincolo può persino essere un vantaggio estetico: il minimalismo orchestrale favorisce leggibilità, identità timbrica e spazio per i telegraph. `DESIGN.md` chiede esplicitamente una gerarchia sonora e che non tutti i canali dominino simultaneamente. ([DESIGN](../../DESIGN.md)) Il rischio vero è il **voice stealing di release lunghe**, soprattutto in Corpo e Vettore; quindi duration e release devono essere compositivamente consapevoli, non lasciate al synth.

Tre strategie meritano comunque test successivo:

| Strategia | Beneficio | Costo/rischio | ADR |
|---|---|---|---|
| 8 voci fisse, 2/ruolo | massimo controllo e determinismo; baseline attuale | armonia e texture molto sparse | nessuna modifica |
| 8 voci, 1 riservata/ruolo + 4 condivise | usa la polifonia dove musicalmente necessaria | stealing inter-ruolo e priorità più complessi | **modifica D-M2-03** |
| 12–16 voci dopo profiling | armonia/release più naturali | CPU, mix density, Android | **modifica D-M2-03** |

**Precisazione per la futura sintesi polifonica:** la relazione fra `NoteOn` e `NoteOff` non deve basarsi soltanto sull'epoch globale. L'identità logica richiede almeno `(role, eventId)` con un contratto di unicità di `eventId` nell'ambito della run e una generazione/ownership di slot che impedisca a un vecchio `NoteOff` di chiudere una voce rubata e riassegnata. Le note sostenute attraverso cambi di epoch non devono essere interrotte da un timeout globale arbitrario; devono comunque essere gestite tramite una policy esplicita di stop, perdita evento, overflow e durata massima autorizzata. Il contratto propedeutico è già in [ADR-0003](../adr/0003-m2-decisions-contracts.md); l'implementazione e i suoi casi limite si verificano in M2.4/M2.5.

La variante “composite voice”, per esempio una singola voce Corpo che calcola tre oscillatori accordali, è tecnicamente possibile ma semanticamente ambigua: rispetta “otto slot” e contemporaneamente supera la polifonia musicale prevista. Prima di adottarla sarebbe meglio distinguere formalmente **DSP voice** e **note voice** in un futuro riesame dell'ADR, invece di usare la tecnica per aggirare il contratto.

Sampling ibrido merita invece di restare pienamente sul tavolo. Un breve set di transienti proprietari/licenziati offre qualità elevata a costo DSP molto basso e la specifica lo contempla già. ([Spec §16/§20/§32](../../CYMATICA_Specifica_Agentica_Sviluppo.md)) Il vantaggio non consiste nel “campionare al posto di sintetizzare”, ma nel lasciare alla sintesi la componente controllabile e continua, usando il sample soltanto dove il transient acusticamente complesso costerebbe molto di più da modellare.

## Realtime, determinismo, adattamento e corrispondenza strutturale col gameplay

Sul piano realtime la ricerca non suggerisce di cambiare l'architettura corrente; al contrario, la rafforza. La documentazione ufficiale di miniaudio ricorda che la data callback è eseguita asincronamente e che determinate operazioni di device management non possono essere chiamate al suo interno, sottolineando inoltre che considerazioni ulteriori dipendono dai requisiti di realtime processing. ([miniaudio manual](https://miniaud.io/docs/manual/index.html)) CYMATICA impone già un regime più severo: niente ricerca, inferenza o lavoro unbounded nella callback; il planning worker opera su snapshot immutabili e il coordinatore resta l'unico producer della coda audio. ([Spec §16/§20/§32](../../CYMATICA_Specifica_Agentica_Sviluppo.md))

L'implicazione è che **la creatività algoritmica deve essere spostata temporalmente a sinistra**, cioè prima del rendering. Per M2.4 il form/phrase planner può generare l'intera struttura delle due sezioni all'avvio o almeno una frase completa con largo anticipo; il scheduler corrente continua poi a trasferire gli eventi verso l'audio nella finestra già congelata. L'ahead-of-time da 150 ms è una finestra di *delivery*, non deve diventare l'orizzonte intellettuale del compositore.

Un pattern robusto è:

```text
planning horizon     : phrase / section
commit horizon       : bar or phrase prefix
scheduler lookahead  : existing 100–250 ms
audio horizon        : current block
```

In M2.4, dove non esiste ancora la necessità di una forte adaptation live, è addirittura preferibile pianificare molto più del necessario e usare il runtime come esecutore deterministico. Nelle milestone successive il commitment può diventare più corto, mantenendo però invarianti i segmenti già schedulati. Questo è coerente con §16.10: l'adattamento deve influenzare il futuro, non riscrivere eventi passati. ([Spec §16/§20/§32](../../CYMATICA_Specifica_Agentica_Sviluppo.md))

**Determinismo.** Tutte le scelte del planner dovrebbero essere indicizzabili senza dipendere dall'ordine accidentale di chiamata: seed di run → section → phrase → role → motif/decision. Il repository ha già una policy di RNG versionata e stream isolation validata; M2.4 dovrebbe semplicemente estendere il domain separation, non inventare un secondo random engine. ([Stato milestone](../progress.md)) Un candidato musicale dovrebbe avere inoltre un tie-break stabile, così che variazioni di container order o del numero di candidati non cambino silenziosamente l'esito.

È utile distinguere due obiettivi spesso confusi. **Determinismo simbolico** significa ottenere gli stessi `MusicIntentEvent`, decision IDs e piani dalla stessa run configuration; questo è essenziale. **PCM bit-identico su qualunque CPU/backend** è un requisito molto più forte e non è necessario per affermare che la composizione sia riproducibile. Per M2.4 suggerisco di rendere canonico il primo e trattare l'eventuale bit-exact DSP multipiattaforma come requisito separato da profilare, non come implicazione automatica del replay.

La portabilità Windows→Android favorisce anche le tecniche suggerite: wavetable, FM semplice, filtri IIR moderati e Karplus–Strong hanno stato finito e workload prevedibile; un motore granulare ad alta densità, convolution reverb o modelli neurali introducono invece budget più difficili da garantire. La review AES 2026 sui digital waveguide evidenzia proprio l'efficienza che storicamente ne ha permesso l'impiego realtime. ([Tablas de Paula et al. 2026](https://doi.org/10.17743/jaes.2026.0283))

**Adattamento senza perdita d'identità.** La strategia più promettente è definire per ogni frase un insieme di **invarianti** e un insieme di **gradi di libertà**. Questa è una proposta originale per CYMATICA, derivata dai risultati su memoria, expectation e structured improvisation ma non direttamente testata nella letteratura sotto questa forma. ([Assayag & Dubnov 2004](https://doi.org/10.1007/s00500-004-0385-4); [Nika 2016](https://jeromenika.com/research-designing-generative-agents/phd-thesis/); [Pearce & Wiggins 2012](https://doi.org/10.1111/j.1756-8765.2012.01214.x))

| Preservare normalmente | Permettere di adattare |
|---|---|
| motif semantic ID | ottava/registro |
| contour fondamentale | densità di ornamenti |
| cadence/goal della frase | orchestrazione |
| metro/pulse anchor | brightness/filter |
| ruolo dominante della frase | subdivision secondarie |
| principali note armoniche | probabilità di variante |
| direzione formale verso climax/recovery | intensità e durata del recovery |

Se il giocatore è sotto pressione, quindi, il sistema non dovrebbe generare “un'altra musica”: dovrebbe **realizzare la stessa frase con meno carico**, per esempio sottraendo Trama, rallentando il tasso di eventi secondari, lasciando intatto il motivo Vettore e preservando la destinazione armonica. Questo crea adattamento percepibile ma non dissolve l'autorialità.

La relazione col gameplay offre la maggiore opportunità di ricerca originale. Molti giochi musicali noti associano eventi ludici a caratteristiche temporali di musica preesistente: Audiosurf, per esempio, analizza la musica e la converte in una pista, e il suo autore descrisse già nel 2008 di aver esplorato senza proseguirla la direzione opposta, cioè la generazione musicale dalle azioni del giocatore. ([Audiosurf creator interview 2008](https://arstechnica.com/gaming/2008/03/catching-waveforms-audiosurf-creator-dylan-speaks/)) Anche la ricerca su musica adattiva nei giochi dimostra benefici possibili da sistemi molto più semplici: uno studio del 2025 su DareFightingICE ha trovato vantaggi di player experience usando gruppi strumentali il cui mix reagiva al gioco. ([Fighting to the beat 2025](https://doi.org/10.1016/j.entcom.2025.100985)) Questi precedenti non dimostrano però che un mapping strutturale condiviso sia superiore; identificano piuttosto lo spazio sperimentale.

Per CYMATICA propongo una **grammatica audiovisiva equivarante alle trasformazioni**: musica e gameplay non reagiscono l'una all'altro, ma sono due proiezioni dello stesso oggetto compositivo.

```text
Shared structural state
        |
        +--> musical realization
        |
        +--> geometric/gameplay realization
```

La differenza rispetto al semplice “bullet sul beat” è radicale. Un motivo non produce soltanto quattro spawn sincronizzati: la sua **identità e le sue trasformazioni** diventano trasformazioni dello spazio.

| Struttura musicale | Possibile omologo geometrico | Vincolo |
|---|---|---|
| contour ascendente/discendente | traiettoria/direzione coerente | non deve compromettere fairness |
| inversione del motivo | riflessione della geometria | preservare telegraph |
| trasposizione | spostamento/riallocazione coerente | non aumentare difficoltà implicitamente |
| augmentation | espansione temporale/spaziale | mantenere finestre percettive |
| diminution | contrazione temporale/spaziale | soggetta al validator |
| density increase | maggiore numero di elementi o attività | non significa automaticamente più elementi letali |
| voice-leading breve | morph geometrico continuo | evitare “teleport” strutturali |
| cadenza/risoluzione | apertura, semplificazione o recupero del campo | leggibile prima dell'esito |
| frammentazione | pattern spezzato/incompleto | segnalare la nuova regola |
| ritorno del leitmotiv | ritorno di una firma geometrica | favorire apprendimento |

Qui l'analogia con Tymoczko è particolarmente fertile: se un cambio armonico può essere pensato come uno spostamento fra configurazioni vicine nello spazio degli accordi, CYMATICA può sperimentare la correlazione fra **distanza di voice-leading e distanza di morph geometrico**. ([Tymoczko 2006](https://doi.org/10.1126/science.1126287)) Non è una conseguenza provata del paper: è una nuova ipotesi audiovisiva che il progetto può testare.

Una seconda proposta originale è il **budget condiviso di sorpresa**. IDyOM e la letteratura sull'aspettativa mostrano che la prevedibilità/sorpresa di un evento può essere formalizzata probabilisticamente. ([Pearce & Wiggins 2012](https://doi.org/10.1111/j.1756-8765.2012.01214.x); [Sears et al. 2018](https://doi.org/10.1080/09298215.2017.1367010)) CYMATICA potrebbe usare questa idea non per massimizzare la sorpresa, bensì per coordinarla: se la frase introduce una nuova trasformazione musicale, il gameplay conserva una grammatica familiare; se introduce una meccanica geometrica nuova, la musica può riaffermare un motivo noto. Ciò eviterebbe il comune fallimento di “climax = tutto nuovo, tutto più forte, tutto più difficile”.

Una terza opportunità è la **memoria cross-modale del leitmotiv**: testare se i giocatori imparano a prevedere una famiglia di pattern geometrici attraverso la trasformazione riconoscibile del relativo motivo musicale. Se funzionasse, la musica diventerebbe informazione ludica a livello di frase e non soltanto metronomo. Questo sarebbe molto più vicino alla promessa di `DESIGN.md` secondo cui il giocatore attraversa la musica anziché limitarsi a seguire un soundtrack. ([DESIGN](../../DESIGN.md))

Queste innovazioni vanno però protette da un principio: **equivalenza strutturale non significa equivalenza di pericolosità**. DESIGN impone una gerarchia tra canale dominante e supporto e vieta che tutti i ruoli diventino simultaneamente letali. ([DESIGN](../../DESIGN.md)) Una densificazione musicale può quindi manifestarsi come particelle non letali, modulazione della piastra, armonia più ricca o texture: il validator di gameplay rimane autorità della fairness.

## Valutazione scientifica ed esperimenti musicali

La valutazione è probabilmente la lacuna metodologica più importante. Yang e Lerch hanno mostrato che i sistemi generativi musicali vengono valutati con una pluralità di metriche oggettive e soggettive e che nessuna singola misura cattura adeguatamente la qualità musicale. ([Yang & Lerch 2020](https://doi.org/10.1007/s00521-018-3849-7)) Il survey interdisciplinare di Lerch e colleghi pubblicato nel 2025 in *ACM Computing Surveys* amplia questa conclusione distinguendo valutazione dell'output e del sistema, metodologie qualitative/quantitative e prospettive musicologiche, ingegneristiche e HCI. ([Lerch et al. 2025](https://doi.org/10.1145/3769106))

Per CYMATICA conviene quindi separare **quattro piani** che non devono essere aggregati prematuramente in uno “score di qualità”.

| Piano | Domanda | Esempi di misure |
|---|---|---|
| Correttezza/engineering | funziona sempre e in tempo? | deadline miss, max callback time, queue high-water, voice steals, underrun, determinism |
| Struttura musicale | costruisce relazioni intenzionali? | motif recurrence, trasformazioni, cadenze, voice-leading, tension curve, section contrast |
| Diversità/originalità | collassa sempre sulle stesse soluzioni? | pairwise motif distance, n-gram duplication, archive coverage, self-similarity |
| Percezione/arte | una persona trova il risultato coerente, espressivo e interessante? | forced choice, ratings multidimensionali, interviste, preference ranking |

Una metrica automatica che aumenta non deve essere interpretata come “musica migliore”. L'entropia, per esempio, è utile per misurare predittività; troppa predittività può essere monotona e troppa entropia incoerente. Conklin e Witten esplicitavano già la difficoltà di quantificare oggettivamente l'accettabilità delle composizioni. ([Conklin & Witten 1995](https://doi.org/10.1080/09298219508570672)) Il recente survey di valutazione conferma che valutazioni oggettive e umane rispondono a domande differenti. ([Lerch et al. 2025](https://doi.org/10.1145/3769106))

**Esperimento compositivo offline principale.** La prova più informativa consiste nel generare gli stessi seed/form skeleton attraverso quattro condizioni, mantenendo identici synth, BPM, durata e mix:

| Condizione | Generatore |
|---|---|
| A — baseline | scala/modo + Euclidean + progressione finita + scelta casuale bounded |
| B — gerarchia | A + Form/Phrase plan + motif memory |
| C — gerarchia/armonia | B + funzione armonica + voice-leading score |
| D — variazione probabilistica | C + multiple-viewpoint/variable-order model o factor-oracle come variation engine |

Il confronto A→B misura il valore di memoria e struttura; B→C misura il contributo del voice-leading; C→D verifica se un modello probabilistico aggiunge espressività o soltanto complessità. L'ordine è cruciale: introdurre subito D impedirebbe di capire quale componente abbia prodotto l'eventuale miglioramento. Multiple-viewpoint e factor oracle hanno solide basi come modelli di memoria/aspettativa, ma non devono ricevere credito per la struttura che in realtà proviene dallo scaffold gerarchico. ([Conklin & Witten 1995](https://doi.org/10.1080/09298219508570672); [Assayag & Dubnov 2004](https://doi.org/10.1007/s00500-004-0385-4); [Pearce & Wiggins 2012](https://doi.org/10.1111/j.1756-8765.2012.01214.x))

Per l'esplorazione offline suggerisco decine di seed per condizione e durate di almeno 60–90 s, più un sottoinsieme di run di diversi minuti per evidenziare affaticamento e ripetizione. Il test percettivo pilota dovrebbe servire a stimare varianze ed effect size; la numerosità del successivo studio confermativo va stabilita tramite power analysis, non scelta arbitrariamente in anticipo.

Le feature automatiche utili sono:

| Feature | Interpretazione |
|---|---|
| motif return rate e distanza trasformazionale | memoria senza copia letterale |
| pitch/rhythm n-gram entropy | prevedibilità locale |
| median/max voice-leading motion | continuità armonica |
| cadence-goal success rate | costruzione della frase |
| tension target error | capacità di seguire l'arco richiesto |
| onset-density profile | forma dinamica |
| role occupancy e overlap | economia orchestrale |
| section distance + return similarity | contrasto e identità formale |
| pairwise run distance | diversità tra seed |
| voice-steal/drop count | costo musicale reale del budget DSP |

Le prime non devono diventare gate artistici automatici. Devono diagnosticare **perché** due condizioni vengono giudicate diversamente dagli ascoltatori.

**Test percettivo.** Gli ascoltatori non dovrebbero ricevere una domanda generica “quanto è bella?”, ma dimensioni distinte: coerenza della forma, riconoscibilità del materiale, tensione/risoluzione, espressività, qualità timbrica, monotonia, naturalezza delle transizioni, distintività e interesse complessivo. La letteratura sull'evaluation generativa raccomanda precisamente di non confondere target valutativi diversi. ([Lerch et al. 2025](https://doi.org/10.1145/3769106); [Yang & Lerch 2020](https://doi.org/10.1007/s00521-018-3849-7))

Un protocollo efficace sarebbe *within-subject* con clip loudness-matched e ordine randomizzato, evitando di rivelare quale algoritmo le ha prodotte. La stessa persona ascolta versioni dello stesso high-level seed, riducendo il confondente dovuto al materiale. Musicisti/compositori e giocatori non specialisti dovrebbero essere analizzati come gruppi distinti: gli studi sul microritmo mostrano che enculturazione ed expertise possono cambiare anche la localizzazione temporale percepita degli eventi. ([Danielsen et al. 2024](https://doi.org/10.1525/mp.2024.41.3.176))

Per la preferenza globale è preferibile una scelta pairwise A/B seguita da rating diagnostici. Il dato “B preferito ad A” risponde alla domanda artistica; i rating spiegano *perché*. Per le analisi confermative, un modello mixed-effects con partecipante e materiale come effetti casuali è più appropriato di considerare ogni giudizio indipendente; lo studio sulle aspettative cadenzali costituisce un esempio del ricorso a modelli mixed per misure ripetute di questo tipo. ([Sears et al. 2020](https://doi.org/10.1177/0305735618803676))

**Esperimento di sintesi.** Qui l'intento simbolico deve essere identico, cambiando soltanto renderer:

| Condizione | Renderer |
|---|---|
| S1 | sottrattivo/wavetable |
| S2 | FM 2-op/3-op |
| S3 | ibrido sample + synthesis |
| S4 | physical/Karplus variant |

Le misure percettive sono qualità timbrica, espressività, appropriatezza al ruolo, distintività CYMATICA e affaticamento; quelle tecniche sono costo medio, percentile alto e worst-case del processing, footprint, stabilità e voce massima. La letteratura non permette di dichiarare in anticipo quale renderer avrà “qualità artistica” maggiore nel contesto di CYMATICA: Chowning, Välimäki e Karplus–Strong dimostrano capacità e proprietà tecniche dei metodi, non una graduatoria estetica universale. ([Chowning 1973](https://www.aes.org/e-lib/); [Välimäki & Huovilainen 2007](https://doi.org/10.1109/MSP.2007.323276); [Karplus & Strong 1983](https://doi.org/10.2307/3680062))

**Esperimento sulle otto voci.** Lo stesso materiale dovrebbe essere reso in tre assetti offline: 8 voci rigide 2/ruolo; 8 con pool dinamico simulato; 12–16 voci. Il test deve misurare non soltanto preferenza, ma quante volte la maggiore polifonia produce un beneficio udibile. Se 12 voci aumentano CPU e densità senza un miglioramento percettivo consistente, il limite attuale è probabilmente un buon vincolo estetico anziché un difetto.

**Esperimento musica–gameplay**, probabilmente il più originale, dovrebbe confrontare:

| Condizione | Mapping |
|---|---|
| G1 | beat-only: eventi giocabili sincronizzati al Pulso |
| G2 | event-symbolic: ruoli MusicIntent→tipi di pattern |
| G3 | structural: motivo, trasformazione, tensione e cadenza condivisi con geometria |

A parità di difficoltà validata, si misurano riconoscimento di pattern, anticipazione, performance, perceived fairness, coerenza audiovisiva e preferenza. G3 è la vera ipotesi di ricerca di CYMATICA: che una relazione **strutturale** consenta al giocatore di imparare la musica come grammatica del livello, invece di limitarsi a reagire agli onset.

**Test della memoria tematica.** Dopo aver esposto A e A′ — stesso motivo trasformato — si chiede se l'utente percepisce il ritorno e se prevede più rapidamente il corrispondente comportamento del campo rispetto a un motivo B non correlato. Questo traduce direttamente “leitmotiv” in un'ipotesi misurabile di apprendimento cross-modale.

**Test di microtiming.** Soltanto dopo una baseline quantizzata riuscita: grid esatto, profilo deterministico “laid-back/pushed” specifico del ruolo e jitter casuale devono essere confrontati separatamente. Alla luce dei risultati empirici esistenti, il jitter non dovrebbe essere assunto come controllo positivo. ([Frühauf et al. 2013](https://doi.org/10.1177/1029864913486793); [Danielsen et al. 2024](https://doi.org/10.1525/mp.2024.41.3.176))

Infine, **originalità non equivale a distanza statistica**. Un sistema può produrre materiale molto distante da tutto ciò che precede e risultare privo di identità; oppure materiale ricorrente e fortemente originale come opera. Le metriche di similarità servono a individuare collapse o copie accidentali, mentre “distintività artistica” richiede valutazione umana. Il survey 2025 sull'evaluation rafforza precisamente la necessità di separare metrica computazionale e giudizio umano. ([Lerch et al. 2025](https://doi.org/10.1145/3769106))

## Raccomandazioni per M2.4, sviluppo successivo e ADR

La raccomandazione più forte è **resistere allo scope creep**. M2.4 può ottenere un salto qualitativo significativo senza cambiare ADR-0003 e senza introdurre ML, solver generali o una grande infrastruttura musicale.

| Proposta | Beneficio atteso | Costo | Rischio | Validazione | Collocazione |
|---|---|---|---|---|---|
| **Form/Phrase planner gerarchico minimo** | sezioni con arco e ritorno percepibili | basso | schemi troppo rigidi | A/B baseline vs gerarchia | **M2.4** |
| **Motif object relativo con 2–3 motivi per run/section** | identità riconoscibile attraverso trasformazioni | basso | motivi banali o troppo persistenti | recognition + return test | **M2.4** |
| **Trasformazioni classificate conservative/developmental/disruptive** | variazione senza perdita d'identità | basso | tassonomia errata | test percettivo A/A′ | **M2.4** |
| **Grafo armonico funzionale/modale finito** | tensione e risoluzione intenzionali | basso | suono troppo “scolastico” | cadence/tension ratings | **M2.4** |
| **Voice-leading bounded su pochi candidati** | meno salti arbitrari e parti più leggibili | basso | eccesso di smoothness | B/C architectural A/B | **M2.4** |
| **Euclidean + metric/phrase masks** | ritmi riconoscibili ma non meccanici | basso | ripetitività | entropy + groove listening | **M2.4** |
| **Synth role-specific wavetable/FM/subtractive** | firme sonore distinte e controllabili | medio | sound design insufficiente | blind synth A/B | **M2.4** |
| **Karplus/waveguide spike** | possibile firma “risonante” unica | basso–medio | tuning/decay | renderer comparison | M2.4 spike o subito dopo |
| **Adattamento identity-preserving** | reattività senza cambiare brano | medio | adaptation troppo percepibile o inefficace | same-seed player states | milestone successive |
| **Grammatica audiovisiva strutturale** | realizza la tesi artistica centrale | medio–alto | mapping artificioso/fairness | G1/G2/G3 | M3–M7 |
| **Dynamic shared voice pool** | usa la polifonia dove serve | medio | priorità/stealing più complessi | 8-fixed vs 8-shared | post-M2, ADR |
| **Factor oracle / MVS variation engine** | memoria statistica e maggiore varietà | medio | perdita di forma, corpus bias | C/D A/B | ricerca successiva |
| **Granular composite Trama** | texture molto ricca | medio–alto | DSP e nuova semantica voice | stress + blind test | esplorativa |
| **Modelli neurali** | potenziale corpus/style modeling | alto | latency, opacity, dataset/IP, determinismo | soltanto contro baseline forte | esplorativa, non necessaria |

Per **M2.4 immediata** proporrei quindi il seguente profilo compositivo, senza elevarne i numeri a requisiti normativi: due sezioni richieste dal milestone, ciascuna organizzata in frasi; pochi motivi persistenti; Pulso come anchor; Corpo con funzione armonica sparsa; Trama come variazione/densità; Vettore come portatore principale del materiale tematico. La seconda sezione dovrebbe dimostrare almeno un vero *ritorno trasformato*, non limitarsi a cambiare BPM o time signature. Questo soddisfa molto meglio l'obiettivo §16 senza alterare il contratto esecutivo. ([Spec §16/§20/§32](../../CYMATICA_Specifica_Agentica_Sviluppo.md)) ([Stato milestone](../progress.md))

Il **minimo dimostratore artistico** che considererei convincente per M2.4 sarebbe una forma in cui l'ascoltatore possa riconoscere, senza vedere la telemetria, almeno: esposizione, sviluppo, transizione, nuova sezione e ritorno/eco del materiale precedente. Non è necessario che tutti gli ascoltatori nominino queste funzioni; è sufficiente che A/B blind mostri un vantaggio nella percezione di coerenza, memoria e intenzionalità rispetto alla baseline scale+pattern.

Per il sound design, il principio dovrebbe essere: **ogni ruolo possiede una firma timbrica prima di possedere molti preset**. Un singolo Pulso ben progettato, un Corpo riconoscibile, una Trama con due trasformazioni e un Vettore espressivo sono più utili artisticamente di cinquanta combinazioni casuali. La storia della FM, della wavetable/virtual-analog e dei physical models dimostra che variazioni timbriche molto ampie possono emergere da piccoli insiemi parametrici; la sfida è scegliere regioni musicalmente significative dello spazio, non massimizzarne la dimensione. ([Chowning 1973](https://www.aes.org/e-lib/); [Välimäki & Huovilainen 2007](https://doi.org/10.1109/MSP.2007.323276); [Tablas de Paula et al. 2026](https://doi.org/10.17743/jaes.2026.0283))

Per le **milestone successive**, il primo investimento dovrebbe essere l'adattamento identity-preserving e la trasformazione cross-modale. Solo dopo avere una baseline musicalmente giudicata valida ha senso sperimentare MVS/IDyOM-like probabilities, factor oracle o search più sofisticata. È metodologicamente importante: se la musica di base non è artistica, un generatore probabilistico più complesso rende soltanto più difficile diagnosticare il problema.

Per la **ricerca esplorativa**, tre filoni hanno il maggior potenziale distintivo:

1. **Grammatica audiovisiva transformation-equivariant.** Una trasformazione musicale e la corrispondente trasformazione geometrica condividono semantic ID e preservano invarianti riconoscibili. Il risultato sperato è apprendimento cross-modale, non semplice sincronia.
2. **Shared surprisal budget.** Novità musicale, geometrica e ludica vengono distribuite temporalmente anziché accumulate nello stesso istante; l'information content diventa una risorsa di pacing, non un obiettivo da massimizzare. La base scientifica viene dai modelli probabilistici di expectation, mentre l'estensione al gameplay è nuova. ([Pearce & Wiggins 2012](https://doi.org/10.1111/j.1756-8765.2012.01214.x); [Sears et al. 2018](https://doi.org/10.1080/09298215.2017.1367010))
3. **Risonanza fisica come firma timbrica e formale.** Delay-line/waveguide non soltanto come synth, ma come stato che conserva energia e memoria fra eventi: gameplay, musica e visual possono condividere concetti come eccitazione, decadimento, accoppiamento e risonanza. La fattibilità realtime dei waveguide ha basi tecniche solide; la loro integrazione compositivo-ludica sarebbe una ricerca specifica di CYMATICA. ([Karplus & Strong 1983](https://doi.org/10.2307/3680062); [Tablas de Paula et al. 2026](https://doi.org/10.17743/jaes.2026.0283))

**ADR-0003: cosa non richiede modifica.** Una gerarchia `FormPlan`/`PhrasePlan`, motif memory interna, funzione armonica, scoring dei candidati e voice-leading fuori callback possono compilare nel `MusicIntentEvent` esistente. Non modificano il canale audio, le otto voci o lo scheduler. Sono quindi implementabili in M2.4 mantenendo D-M2-02 e D-M2-03 intatti. ([ADR-0003](../adr/0003-m2-decisions-contracts.md))

**Decisioni che richiederebbero invece una futura revisione di ADR-0003:**

| Cambiamento futuro | Decisione interessata | Motivo |
|---|---|---|
| aggiungere `motifId`, `phraseId`, `transformId`, articulation o dati armonici a `MusicIntentEvent` | **D-M2-02** | cambia il contratto simbolico congelato |
| introdurre nuovi tipi di evento oltre la semantica pitch/velocity/duration/timbre corrente | **D-M2-02** | modifica la rappresentazione condivisa |
| Euclidean pattern oltre il limite strutturale attuale di 32 step | **D-M2-02** | il limite è parte della decisione congelata |
| passare da 2 voci per ruolo a pool dinamico | **D-M2-03** | cambia ownership/priorità/voice-stealing |
| aumentare il massimo oltre otto voci | **D-M2-03** | cambia hard DSP budget |
| definire una composite voice con polifonia interna non contabilizzata come note | **D-M2-03** | cambia il significato di “voice budget” |
| cambiare la finestra/semantica dello scheduling ahead-of-time | **D-M2-03** | modifica il contratto di scheduling |
| sostituire il voice stealing locale con stealing globale prioritario | **D-M2-03** | cambia la saturation policy |

Una rappresentazione strutturale più ricca **non dovrebbe essere aggiunta a `MusicIntentEvent` solo perché sarebbe elegante**. Il vantaggio dell'evento attuale è che separa composizione e rendering. Finché il planner può mantenere internamente `motifId` e `phraseId` e compilare in eventi correnti, questa separazione è sana. Solo quando gameplay, replay, tooling o telemetry avranno bisogno di tali semantic IDs come contratto pubblico sarà giustificato revisionare D-M2-02.

Analogamente, suggerisco di **non modificare D-M2-03 prima di avere l'esperimento sulle otto voci**. Il budget fisso può produrre un'estetica di sottrazione molto adatta a CYMATICA. Una modifica è giustificata soltanto se si osserva sistematicamente un fallimento musicale — per esempio release interrotte, incapacità di realizzare cadenze o perdita del motivo — che il sound design e l'orchestrazione sparsa non risolvono.

## Matrice delle evidenze e bibliografia (metadati verificati a campione)

La tabella seguente distingue fra evidenza scientifica/tecnica, applicabilità diretta e inferenze proposte per CYMATICA. “Fondamentale” non significa necessariamente “più recente”; molte delle tecniche con il rapporto migliore fra qualità, costo e controllabilità hanno decenni di maturità.

| Fonte | Evidenza riportata dalla fonte | Livello | Applicabilità a CYMATICA |
|---|---|---|---|
| **Spec CYMATICA 0.8.3, §7/16/20/32** | gerarchia temporale, Music Intent condiviso, motif memory, Euclidean rhythm, planner fuori callback | normativa progetto | baseline, non tema da riaprire ([Spec §16/§20/§32](../../CYMATICA_Specifica_Agentica_Sviluppo.md)) |
| **DESIGN 0.3.1, §8/16** | quattro funzioni musicali/ludiche; forma, motivi, ritorni e mix stabile | normativa progetto | criterio artistico primario ([DESIGN](../../DESIGN.md)) |
| **ADR-0003, 2026-10-12** | `MusicIntentEvent`, Bjorklund ≤32, scheduler, 8 voci 2/ruolo | contratto accepted | confine architetturale M2.4 ([ADR-0003](../adr/0003-m2-decisions-contracts.md)) |
| **progress.md** | M2.1–M2.3 completate; M2.4 prossimo passo | evidenza progetto | definisce scope reale della ricerca ([Stato milestone](../progress.md)) |
| **Conklin & Witten, 1995** | multiple viewpoints + short/long-term models; prediction/generation su Bach | peer reviewed | variation engine e novelty/expectation model ([Conklin & Witten 1995](https://doi.org/10.1080/09298219508570672)) |
| **Rohrmeier, 2011** | armonia tonale modellata gerarchicamente; limite delle sole transizioni Markov | peer reviewed/formale | harmonic-function grammar ([Rohrmeier 2011](https://doi.org/10.1080/17459737.2011.573676)) |
| **Anders & Miranda, 2011** | constraint programming applicato a harmony, counterpoint, rhythm, form | survey tecnico peer reviewed | solver locale/bounded, non callback ([Anders & Miranda 2011](https://doi.org/10.1145/1978802.1978809)) |
| **Assayag & Dubnov, 2004** | factor oracle per apprendimento e improvisation realtime | peer reviewed | motif/self-memory futura ([Assayag & Dubnov 2004](https://doi.org/10.1007/s00500-004-0385-4)) |
| **Nika, 2016** | temporal scenarios per mediare planning e reactivity in improvisazione | tesi IRCAM | modello concettuale per adaptation senza perdere forma ([Nika 2016](https://jeromenika.com/research-designing-generative-agents/phd-thesis/)) |
| **Pearce & Wiggins, 2012/2018** | expectation, variable-order models, information content legati a dati percettivi | evidenza cognitiva + computazionale | controllare sorpresa, non qualità assoluta ([Pearce & Wiggins 2012](https://doi.org/10.1111/j.1756-8765.2012.01214.x)) |
| **Farbood, 2012** | tensione dipende da più parametri e trend temporali | studi percettivi | tension-state multidimensionale ([Farbood 2012](https://doi.org/10.1525/mp.2012.29.4.387)) |
| **Sears et al., 2018/2020** | prevedibilità e aspettativa di chiusura cambiano con il contesto cadenzale | studi empirici | cadence goals e phrase closure ([Sears et al. 2020](https://doi.org/10.1177/0305735618803676); [Sears et al. 2018](https://doi.org/10.1080/09298215.2017.1367010)) |
| **Huron, 2001** | molte regole di voice-leading derivabili da principi percettivi | peer reviewed | piccolo cost model percettivo ([Huron 2001](https://doi.org/10.1525/mp.2001.19.1.1)) |
| **Tymoczko, 2006** | voice-leading come distanza fra configurazioni accordali | peer reviewed, *Science* | scoring armonico; nuova ipotesi morph geometrico ([Tymoczko 2006](https://doi.org/10.1126/science.1126287)) |
| **Toussaint, 2005** | Euclidean algorithm produce distribuzioni maximally even affini a molti rhythm timelines | proceedings | ottimo pattern primitive, non form generator ([Toussaint 2005](https://www.archive.bridgesmathart.org/2005/bridges2005-47.html)) |
| **Reich, 1968/2004** | processo musicale percepibile come principio compositivo | fonte primaria compositore | trasformazioni graduali e leggibili ([Reich 1968](https://doi.org/10.1093/acprof:oso/9780195151152.003.0004)) |
| **Frühauf et al., 2013** | nel test rock, timing quantizzato valutato meglio di deviazioni crescenti | studio N=93 | evitare random humanization ([Frühauf et al. 2013](https://doi.org/10.1177/1029864913486793)) |
| **Danielsen et al., 2024** | attack, durata, timbro ed expertise influenzano timing percepito | sintesi di programma empirico | progettare groove tramite timbro+envelope, non solo onset ([Danielsen et al. 2024](https://doi.org/10.1525/mp.2024.41.3.176)) |
| **Chowning, 1973** | FM controlla spettri complessi e loro evoluzione con struttura semplice | fondamentale DSP | Vettore/Pulso, macro timbre control ([Chowning 1973](https://www.aes.org/e-lib/)) |
| **Välimäki & Huovilainen, 2006/2007** | virtual analog e oscillatori antialiased/band-limited | peer reviewed DSP | Corpo/Vettore, baseline synth di qualità ([Välimäki & Huovilainen 2007](https://doi.org/10.1109/MSP.2007.323276); [Välimäki & Huovilainen 2006](https://doi.org/10.1162/comj.2006.30.2.19)) |
| **Karplus & Strong, 1983** | delay feedback filtrato per plucked/drum timbres | fondamentale DSP | firma risonante a basso costo ([Karplus & Strong 1983](https://doi.org/10.2307/3680062)) |
| **Tablas de Paula et al., 2026** | waveguide rimangono efficienti per physical modeling realtime | review AES peer reviewed | conferma attualità della famiglia physical modeling ([Tablas de Paula et al. 2026](https://doi.org/10.17743/jaes.2026.0283)) |
| **Roads, 1978** | granular synthesis come organizzazione automatizzata di grains | fondamentale | Trama futura; richiede budgeting interno ([Roads 1978](https://doi.org/10.2307/3680222)) |
| **Yang & Lerch, 2020** | tassonomia e criticità dell'evaluation dei sistemi generativi | peer reviewed | separare metriche automatiche e percezione ([Yang & Lerch 2020](https://doi.org/10.1007/s00521-018-3849-7)) |
| **Lerch et al., 2025** | survey interdisciplinare di evaluation output/model use | peer reviewed, recente | framework sperimentale raccomandato ([Lerch et al. 2025](https://doi.org/10.1145/3769106)) |
| **Wu, Yu & Xia, 2026** | generazione simbolica realtime controllata da gesto su timing/density/pitch/layer | NIME 2026 | evidenza recente sulla separazione control/generator; non baseline RT per CYMATICA ([Wu, Yu & Xia 2026](https://nime.org/proc/nime2026_32/index.html)) |
| **Adaptive BGM, 2025** | adattamento rule-based multi-instrumentale ha migliorato player experience nel contesto studiato | studio applicato games | supporto indiretto all'adaptation, non alla composizione pienamente generativa ([Fighting to the beat 2025](https://doi.org/10.1016/j.entcom.2025.100985)) |

**Bibliografia essenziale con identificatori persistenti**

Conklin, D.; Witten, I. H. (1995), “Multiple viewpoint systems for music prediction”, *Journal of New Music Research*, 24(1), 51–73. DOI [10.1080/09298219508570672](https://doi.org/10.1080/09298219508570672). ([Conklin & Witten 1995](https://doi.org/10.1080/09298219508570672))

Rohrmeier, M. (2011), “Towards a generative syntax of tonal harmony”, *Journal of Mathematics and Music*, 5(1), 35–53. DOI [10.1080/17459737.2011.573676](https://doi.org/10.1080/17459737.2011.573676). ([Rohrmeier 2011](https://doi.org/10.1080/17459737.2011.573676))

Anders, T.; Miranda, E. R. (2011), “Constraint programming systems for modeling music theories and composition”, *ACM Computing Surveys*, 43(4), Article 30. DOI [10.1145/1978802.1978809](https://doi.org/10.1145/1978802.1978809). ([Anders & Miranda 2011](https://doi.org/10.1145/1978802.1978809))

Assayag, G.; Dubnov, S. (2004), “Using factor oracles for machine improvisation”, *Soft Computing*, 8(9), 604–610. DOI [10.1007/s00500-004-0385-4](https://doi.org/10.1007/s00500-004-0385-4). ([Assayag & Dubnov 2004](https://doi.org/10.1007/s00500-004-0385-4))

Pearce, M. T.; Wiggins, G. A. (2012), “Auditory Expectation: The Information Dynamics of Music Perception and Cognition”, *Topics in Cognitive Science*, 4(4), 625–652. DOI [10.1111/j.1756-8765.2012.01214.x](https://doi.org/10.1111/j.1756-8765.2012.01214.x). ([Pearce & Wiggins 2012](https://doi.org/10.1111/j.1756-8765.2012.01214.x))

Farbood, M. M. (2012), “A Parametric, Temporal Model of Musical Tension”, *Music Perception*, 29(4), 387–428. DOI [10.1525/mp.2012.29.4.387](https://doi.org/10.1525/mp.2012.29.4.387). ([Farbood 2012](https://doi.org/10.1525/mp.2012.29.4.387))

Sears, D. R. W.; Pearce, M. T.; Caplin, W. E.; McAdams, S. (2018), “Simulating melodic and harmonic expectations for tonal cadences using probabilistic models”, *Journal of New Music Research*, 47(1), 29–52. DOI [10.1080/09298215.2017.1367010](https://doi.org/10.1080/09298215.2017.1367010). ([Sears et al. 2018](https://doi.org/10.1080/09298215.2017.1367010))

Sears, D. R. W.; Spitzer, J.; Caplin, W. E.; McAdams, S. (2020; online 2018), “Expecting the end: Continuous expectancy ratings for tonal cadences”, *Psychology of Music*, 48(3), 358–375. DOI [10.1177/0305735618803676](https://doi.org/10.1177/0305735618803676). ([Sears et al. 2020](https://doi.org/10.1177/0305735618803676))

Huron, D. (2001), “Tone and Voice: A Derivation of the Rules of Voice-Leading from Perceptual Principles”, *Music Perception*, 19(1), 1–64. DOI [10.1525/mp.2001.19.1.1](https://doi.org/10.1525/mp.2001.19.1.1). ([Huron 2001](https://doi.org/10.1525/mp.2001.19.1.1))

Tymoczko, D. (2006), “The Geometry of Musical Chords”, *Science*, 313(5783), 72–74. DOI [10.1126/science.1126287](https://doi.org/10.1126/science.1126287). ([Tymoczko 2006](https://doi.org/10.1126/science.1126287))

Reich, S. (1968; rist. 2004), “Music as a Gradual Process”, in *Writings on Music 1965–2000*, Oxford University Press, pp. 34–36. DOI capitolo `10.1093/acprof:oso/9780195151152.003.0004`. ([Reich 1968](https://doi.org/10.1093/acprof:oso/9780195151152.003.0004))

Frühauf, J.; Kopiez, R.; Platz, F. (2013), “Music on the timing grid: The influence of microtiming on the perceived groove quality of a simple drum pattern performance”, *Musicae Scientiae*, 17(2), 246–260. DOI [10.1177/1029864913486793](https://doi.org/10.1177/1029864913486793). ([Frühauf et al. 2013](https://doi.org/10.1177/1029864913486793))

Danielsen, A. et al. (2024), “There’s More to Timing than Time: Investigating Musical Microrhythm Across Disciplines and Cultures”, *Music Perception*, 41(3), 176–198. DOI [10.1525/mp.2024.41.3.176](https://doi.org/10.1525/mp.2024.41.3.176). ([Danielsen et al. 2024](https://doi.org/10.1525/mp.2024.41.3.176))

Chowning, J. M. (1973), “The Synthesis of Complex Audio Spectra by Means of Frequency Modulation”, *Journal of the Audio Engineering Society*, 21(7), 526–534. Si rinvia alla [AES E-Library](https://www.aes.org/e-lib/) per i metadati della pubblicazione; non viene attribuito un DOI non verificato. ([Chowning 1973](https://www.aes.org/e-lib/))

Välimäki, V.; Huovilainen, A. (2006), “Oscillator and Filter Algorithms for Virtual Analog Synthesis”, *Computer Music Journal*, 30(2), 19–31. DOI [10.1162/comj.2006.30.2.19](https://doi.org/10.1162/comj.2006.30.2.19). ([Välimäki & Huovilainen 2006](https://doi.org/10.1162/comj.2006.30.2.19))

Välimäki, V.; Huovilainen, A. (2007), “Antialiasing Oscillators in Subtractive Synthesis”, *IEEE Signal Processing Magazine*, 24(2), 116–125. DOI [10.1109/MSP.2007.323276](https://doi.org/10.1109/MSP.2007.323276). ([Välimäki & Huovilainen 2007](https://doi.org/10.1109/MSP.2007.323276))

Karplus, K.; Strong, A. (1983), “Digital Synthesis of Plucked-String and Drum Timbres”, *Computer Music Journal*, 7(2), 43–55. DOI [10.2307/3680062](https://doi.org/10.2307/3680062). ([Karplus & Strong 1983](https://doi.org/10.2307/3680062))

Roads, C. (1978), “Automated Granular Synthesis of Sound”, *Computer Music Journal*, 2(2), 61–62. DOI [10.2307/3680222](https://doi.org/10.2307/3680222). ([Roads 1978](https://doi.org/10.2307/3680222))

Tablas de Paula, P.; Smith III, J. O.; Välimäki, V.; Reiss, J. D. (2026), “Four Decades of Digital Waveguides”, *Journal of the Audio Engineering Society*, 74(7/8), 464–484. DOI [10.17743/jaes.2026.0283](https://doi.org/10.17743/jaes.2026.0283). ([Tablas de Paula et al. 2026](https://doi.org/10.17743/jaes.2026.0283))

Yang, L.-C.; Lerch, A. (2020), “On the evaluation of generative models in music”, *Neural Computing and Applications*, 32(9), 4773–4784. DOI [10.1007/s00521-018-3849-7](https://doi.org/10.1007/s00521-018-3849-7). ([Yang & Lerch 2020](https://doi.org/10.1007/s00521-018-3849-7))

Lerch, A.; Arthur, C.; Bryan-Kinns, N.; Ford, C.; Sun, Q.; Vinay, A. (2025), “Survey on the Evaluation of Generative Models in Music”, *ACM Computing Surveys*. DOI [10.1145/3769106](https://doi.org/10.1145/3769106). ([Lerch et al. 2025](https://doi.org/10.1145/3769106))

Wu, T.; Yu, R.; Xia, G. (2026), “Calliphony: A Calligraphy-Driven Interface for Real-Time Generative Music Performance”, *Proceedings of NIME 2026*, pp. 270–275. DOI [10.5281/zenodo.20784121](https://doi.org/10.5281/zenodo.20784121). ([Wu, Yu & Xia 2026](https://nime.org/proc/nime2026_32/index.html))



### Riferimenti complementari e fonti progettuali (revisione editoriale)

- Nika, J. (2016), *Guiding human-computer music improvisation: introducing authoring and control with temporal scenarios*. Tesi di dottorato, IRCAM. [Pagina dell'autore e tesi](https://jeromenika.com/research-designing-generative-agents/phd-thesis/).
- Toussaint, G. (2005), *The Euclidean Algorithm Generates Traditional Musical Rhythms*. Bridges / Renaissance Banff, pp. 47–56. [Archivio Bridges](https://www.archive.bridgesmathart.org/2005/bridges2005-47.html).
- Herremans, D.; Roy, A. (2026), *Aligning Generative Music AI with Human Preferences: Methods and Challenges*. AAAI 2026, 40(46), 39699–39706. [DOI 10.1609/aaai.v40i46.41323](https://doi.org/10.1609/aaai.v40i46.41323). È un lavoro di prospettiva metodologica, **non** una prova sperimentale di superiorità artistica dei modelli neurali per CYMATICA.
- *Fighting to the beat: Multi-instrumental adaptive background music approach* (2025), *Entertainment Computing*, vol. 55, art. 100985. [DOI 10.1016/j.entcom.2025.100985](https://doi.org/10.1016/j.entcom.2025.100985). Adattamento di gruppi strumentali su musica precomposta: evidenza indiretta, non conferma del generatore simbolico CYMATICA.
- Fitterer, D. (intervista, 2008), *Catching Waveforms: Audiosurf Creator Dylan Fitterer Speaks*, Ars Technica. [Fonte primaria intervista](https://arstechnica.com/gaming/2008/03/catching-waveforms-audiosurf-creator-dylan-speaks/).
- miniaudio, *Programming Manual*. [Documentazione ufficiale](https://miniaud.io/docs/manual/index.html).
- Fournel, N. (GDC 2010), *Procedural Audio for Video Games: Are we there yet?*. [Materiale tecnico GDC](https://media.gdcvault.com/gdc10/slides/Fournel_Nicolas_ProceduralAudioForVideoGames.pdf). Era presente nelle fonti NotebookLM caricate.
- Khalifa, A.; Lee, S.; Nealen, A.; Togelius, J. (2018), *Talakat: Bullet Hell Generation through Constrained Map-Elites*. [DOI 10.1145/3205455.3205470](https://doi.org/10.1145/3205455.3205470). Studio pertinente soprattutto a M4 e M8, non prova della superiorità di un mapping musica-geometria.
- Couture, J. (2018), *How jams become levels in the co-op bullet hell musical Just Shapes & Beats*. [Game Developer](https://www.gamedeveloper.com/design/how-jams-become-levels-in-the-co-op-bullet-hell-musical-i-just-shapes-beats-i-). Riferimento di design industriale, non un generatore musicale procedurale.
- *Synesthesia in Video Games: A Look Into Tetsuya Mizuguchi's Game Design Philosophy* (2024). [Articolo di critica/design](https://www.brothers-in-gaming.com/post/synesthesia-in-video-games). Fonte ispirazionale, non validazione scientifica di esperimenti cross-modali.

## Incertezze, limiti e decisione complessiva

La prima incertezza è fondamentale: **non esiste una tecnica scientificamente dimostrata capace di produrre in generale musica di “qualità artistica elevata”**. La letteratura permette di misurare coerenza, previsione, tensione, similarità, voice-leading, preferenza e qualità percepita, ma la qualità artistica resta un costrutto multidimensionale e dipendente da contesto, cultura, materiale e intenzione. I survey dedicati alla valutazione dei sistemi generativi confermano la necessità di combinare metriche computazionali, ascolto umano e analisi dell'uso del sistema. ([Yang & Lerch 2020](https://doi.org/10.1007/s00521-018-3849-7); [Lerch et al. 2025](https://doi.org/10.1145/3769106))

La seconda riguarda il linguaggio musicale. Gran parte della letteratura più quantitativa su expectation, cadence e voice-leading è radicata nella musica tonale occidentale. ([Sears et al. 2020](https://doi.org/10.1177/0305735618803676); [Sears et al. 2018](https://doi.org/10.1080/09298215.2017.1367010); [Huron 2001](https://doi.org/10.1525/mp.2001.19.1.1)) CYMATICA può ricavarne principi di aspettativa, continuità e segregazione delle voci, ma **non dovrebbe scambiarli per una prescrizione stilistica**. L'identità dell'opera dovrebbe essere deliberatamente composta: quali intervalli, scale, simmetrie, registri, ritmi, cadenze e tensioni appartengono a CYMATICA è una decisione artistica, non qualcosa che un algoritmo può dedurre universalmente.

La terza è il budget di otto voci. Non è ancora possibile stabilire dalla letteratura se sia insufficiente: questa è una proprietà del repertorio timbrico e dell'orchestrazione concreta. Due voci per ruolo sono insufficienti per una scrittura accordale convenzionale ricca, ma potrebbero essere ideali per una partitura elettronica minimalista in cui ogni ruolo è timbricamente complesso e l'armonia è distribuita. La decisione razionale è **misurare prima di aumentare il budget**, mantenendo D-M2-03 durante M2.4. ([ADR-0003](../adr/0003-m2-decisions-contracts.md))

La quarta riguarda il microtiming: non c'è sostegno empirico per l'assunto generico “più deviazione dalla griglia = più umano = più bello”. I risultati dipendono fortemente da stile e timbro, e in alcuni esperimenti il timing perfettamente quantizzato è preferito. ([Frühauf et al. 2013](https://doi.org/10.1177/1029864913486793); [Danielsen et al. 2024](https://doi.org/10.1525/mp.2024.41.3.176)) La humanization casuale dovrebbe quindi essere una delle ultime aggiunte, non una scorciatoia per l'espressività.

La quinta riguarda modelli probabilistici e neurali. Esistono forti evidenze che i modelli probabilistici siano utili per aspettativa e stile locale, e risultati moderni mostrano possibilità molto più ampie; ma da ciò non segue che un generatore neurale produca automaticamente forma migliore, maggiore originalità o musica più adatta a CYMATICA. ([Pearce & Wiggins 2012](https://doi.org/10.1111/j.1756-8765.2012.01214.x); [Herremans & Roy 2026](https://doi.org/10.1609/aaai.v40i46.41323)) Sotto i vincoli di determinismo, portabilità, hard realtime, trasparenza e identità autoriale, il loro costo di integrazione deve essere giustificato contro una baseline simbolica già forte.

La sesta riguarda la corrispondenza musica–geometria. La **grammatica audiovisiva equivarante**, il **budget condiviso di sorpresa** e la **memoria cross-modale del leitmotiv** sono proposte originali di questa ricerca, non tecniche già validate. Le evidenze esistenti supportano i componenti — aspettativa musicale, trasformazioni, voice-leading, adaptive game music — ma non dimostrano ancora che la loro combinazione migliori apprendimento, fairness o valore artistico in un bullet hell. ([Pearce & Wiggins 2012](https://doi.org/10.1111/j.1756-8765.2012.01214.x); [Tymoczko 2006](https://doi.org/10.1126/science.1126287); [Fighting to the beat 2025](https://doi.org/10.1016/j.entcom.2025.100985)) Proprio per questo costituiscono opportunità di ricerca più interessanti della semplice adozione di una tecnologia generativa recente.

La decisione tecnica complessivamente più difendibile è quindi:

> **M2.4 dovrebbe costruire un piccolo sistema compositivo autoriale, gerarchico e deterministico — forma → frase → motivo → funzione armonica → voice-leading → ruolo → `MusicIntentEvent` — e una palette di sintesi role-specific basata soprattutto su wavetable/subtractive/FM, con physical modeling come spike distintivo.**

Questa soluzione preserva integralmente la separazione planner/callback, il determinismo, l'attuale `MusicIntentEvent` e le otto voci congelate da ADR-0003. ([ADR-0003](../adr/0003-m2-decisions-contracts.md)) Sfrutta tecniche compositive con basi teoriche e percettive solide, ma lascia al compositore il controllo del linguaggio e dell'identità. ([Rohrmeier 2011](https://doi.org/10.1080/17459737.2011.573676); [Huron 2001](https://doi.org/10.1525/mp.2001.19.1.1); [Pearce & Wiggins 2012](https://doi.org/10.1111/j.1756-8765.2012.01214.x)) Usa la casualità per la **variazione**, non come surrogato dell'intenzione; il constraint solving per **rifinire**, non come compositore totale; il process music per rendere le trasformazioni **percepibili**, non per sostituire la forma. ([Anders & Miranda 2011](https://doi.org/10.1145/1978802.1978809); [Reich 1968](https://doi.org/10.1093/acprof:oso/9780195151152.003.0004))

Soprattutto, sposta il criterio di successo da “quante tracce diverse può generare?” a una domanda molto più adatta a CYMATICA: **dopo alcuni minuti, l'ascoltatore-giocatore riconosce che questa musica ricorda, sviluppa, sorprende e ritorna secondo una logica propria — e che la stessa logica vive nel campo che sta attraversando?** È questa proprietà, non l'infinità statistica dell'output, che offre al progetto la possibilità concreta di diventare un'opera audiovisiva con una propria identità.