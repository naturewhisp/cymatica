# Coordinamento dei ruoli nella musica generativa multi-parte: direttore centrale o agenti, generazione sequenziale, congiunta o iterativa

Consultazione delle fonti: 2026-11-03. Queste note non ripetono il confronto di `docs/research/M2_4_Deep_Research_Reviewed.md` (righe 45-55), che classifica metodi compositivi (grammatica, Markov, CSP, factor oracle, neurale). Qui la domanda è diversa: dato un metodo, come si coordinano le parti tra loro.

**Livelli di verifica usati.** FT = testo integrale letto direttamente; FT* = letto tramite estrattore automatico; AM = solo abstract o metadati; SEC = noto solo da fonte secondaria. WebFetch non decodifica i PDF, quindi tutti i PDF sono stati estratti con `pdftotext` e il testo risultante è stato letto direttamente, senza riassuntore intermedio. **Nessuna fonte è quindi marcata FT**: anche la lettura più completa è FT*, e le tabelle e le figure vanno lette con la cautela dovuta all'estrazione. Il registro completo, con la copertura di ogni lettura, è nella sezione sulle prove d'ascolto (ultima domanda). Le fonti non raggiungibili sono elencate nelle Gaps di quella stessa sezione.

**Correzione di una premessa, verificata nel codice.** La consegna dice che i quattro ruoli nascono da un accordo e una scala condivisi. Nel codice l'accordo condiviso raggiunge solo Corpo e Trama; **Vettore condivide soltanto la scala**. Dettagli nella domanda 3.

## 1. Direttore centrale o sistemi multi-agente: come «ascoltano» gli agenti e quali risultati si riportano?

### Takeaway
Nei sistemi che coordinano parti generate dallo stesso programma, l'«ascolto» è quasi sempre simbolico: stato condiviso, messaggi, griglia di accordi. L'analisi audio serve per l'ingresso di un musicista umano o per il timbro. Non ho trovato alcuno studio d'ascolto controllato che confronti direttore e agenti a parità di materiale; il caso meglio valutato (Hutchings & McCormack 2020) non isola il coordinamento.

### Cited Findings
**Hutchings & McCormack 2020 (AMS per videogiochi).**
- Pubblicato in IEEE Transactions on Games 12(3), pp. 270-280, settembre 2020; l'anno 2019 è quello dell'online first — [Crossref](https://doi.org/10.1109/TG.2019.2921979) [AM]
- L'agente armonia non suona: produce un contesto armonico, una matrice 12 × suddivisioni del tempo letta da tutti gli agenti melodia. Valori: fondamentale 1,0; note dell'accordo 0,8; le altre ereditate dalla battuta precedente e limitate a 0-0,5; prima battuta inizializzata a 0,3 — [Hutchings & McCormack 2020, sez. VI.B](https://arxiv.org/abs/1907.01154) [FT*]
- Ogni agente melodia prova in modo esaustivo trasposizioni e spostamenti temporali di un frammento, con fitness M = H + P (armonia + stile). Quando un frammento viene piazzato, le note «consumano» tutte le risorse della propria cella e metà di quelle del semitono sopra e sotto e del tritono, così gli agenti successivi evitano gli scontri — [stessa fonte](https://arxiv.org/abs/1907.01154) [FT*]
- Il ruolo di guida è dinamico: la confidenza softmax della rete armonica si confronta con quella del primo agente melodia. Se l'armonia è più sicura, la melodia cerca di adattarsi; altrimenti si valutano progressivamente accordi meno probabili per adattarli alla melodia. Si generano due battute per volta. Sotto una soglia di fitness armonica un agente può restare in silenzio — [stessa fonte](https://arxiv.org/abs/1907.01154) [FT*]
- Risultati: due gruppi da 17 persone (34 in tutto), 20 minuti per gioco, confronto tra AMS e colonna sonora originale. Il confronto tra soggetti per singolo gioco non raggiunge p < 0,05: immersione p = 0,121 (Zelda) e 0,053 (StarCraft); correlazione musica-eventi p = 0,057 e 0,055. Il Wilcoxon entro gruppo dà p < 0,05 sull'effetto percepito della musica sull'immersione. Il riconoscimento dei concetti associati cala con AMS: 70→53% (Zelda), 59→41% (StarCraft) — [stessa fonte, sez. VII, tab. III](https://arxiv.org/abs/1907.01154) [FT*]
- Gli autori ammettono che la qualità «is not that of a skilled composer» e la dichiarano fuori ambito — [stessa fonte](https://arxiv.org/abs/1907.01154) [FT*]
- Il precursore del 2017 è descritto come alternanza di guida tra agenti tramite un sistema di valutazione — [Hutchings & McCormack 2017](https://doi.org/10.1007/978-3-319-55750-2_8) [SEC: sintesi dell'abstract da ricerca web; Springer non accessibile]

**Eigenfeldt, Kinetic Engine: una traiettoria da direttore ad agenti e ritorno.**
- v1: quattro esecutori passivi comandati da un direttore virtuale, che fissa densità e livello aggregato di variazione. È l'unica versione ritirata, perché il modello gerarchico si è rivelato troppo difficile da estendere e aggiornare. La forma funzionava ma restava limitata nella capacità di sorprendere — [Eigenfeldt & Pasquier 2009](http://www.sfu.ca/~eigenfel/A%20Realtime%20Generative%20Music%20System.pdf) [FT*, integrale]
- v2: il direttore si riduce a tempo, metro, suddivisione, impulso globale e a una valutazione sfumata della densità («molto bassa», «troppo alta», «abbastanza vicina»). Gli agenti non conoscono la densità esatta. Le relazioni tra agenti passano da messaggi con riscontro, modellati sul contatto visivo tra musicisti — [Eigenfeldt 2007](http://www.sfu.ca/~eigenfel/Plymouth07_VirtualDrum.pdf) [FT*, sezioni sul direttore e sulle interazioni]
- v2 risulta flessibile, ma a lungo andare omogenea perché il suo insieme di regole è statico — [Eigenfeldt & Pasquier 2009](http://www.sfu.ca/~eigenfel/A%20Realtime%20Generative%20Music%20System.pdf) [FT*]
- v3: gli agenti trasmettono una lista di eventi futuri (le loro «intenzioni») e allineano i cambi a livello di frase. Il costo di generare e analizzare popolazioni in tempo reale ha però tolto reattività — [stessa fonte](http://www.sfu.ca/~eigenfel/A%20Realtime%20Generative%20Music%20System.pdf) [FT*]
- Conclusione degli autori: non hanno trovato un metodo autonomo di controllo formale riuscito, e propongono un terzo livello simile al direttore di v1, combinando analisi dall'alto e generazione dal basso. La valutazione è dichiarata artistica e soggettiva, con validazione oggettiva solo pianificata — [stessa fonte](http://www.sfu.ca/~eigenfel/A%20Realtime%20Generative%20Music%20System.pdf) [FT*]

**Coming Together (negoziazione).**
- Quattro agenti partono senza materiale, e la convergenza è l'opera stessa (circa dieci minuti per esecuzione). Gli agenti conservano dati di esecuzione altrui, non le intenzioni. In modalità MIDI il segnale è simbolico; la modalità AUDIO aggiunge l'analisi dell'audio per negoziare il timbro. Stato iniziale casuale, negoziazione non deterministica, convergenza non garantita: dopo troppi fallimenti il sistema si azzera. Il testo letto non riporta studi d'ascolto — [Eigenfeldt 2010](http://www.sfu.ca/~eigenfel/ComingTogether_Beauty&Truth.pdf) [FT*, sezioni 3-6]

**Musebot (ensemble distribuiti).**
- Ogni musebot corrisponde a una parte (basso, batteria, synth). Un Conductor genera il tempo e fa da snodo di tutti i messaggi OSC; il protocollo comunica stati e intenzioni. Il messaggio di densità basta, secondo l'autore, a produrre musica che sembra intenzionale. L'algoritmo di curatela evita semplicemente di mettere insieme due generatori di basso. Il testo letto non riporta studi controllati — [Eigenfeldt 2016](https://zenodo.org/records/4285186/files/Eigenfeldt_Musebots.pdf) [FT*, parziale]
- Specifica originale — [Bown, Carey & Eigenfeldt 2015](https://isea-archives.siggraph.org/presentation/manifesto-for-a-musebot-ensemble-a-platform-for-live-interactive-performance-between-multiple-autonomous-musical-agents) [SEC: non letta, nota tramite articoli successivi]

**Blackwell (sciami).**
- In Swarm Music l'ambiente è fatto di eventi MIDI, compreso audio ricondotto a parametri MIDI, provenienti da umani o da altri sciami. Gli eventi diventano attrattori nello spazio dello sciame (stigmergia): il coordinamento è indiretto, attraverso l'ambiente. Gli autori osservano che la parametrizzazione MIDI perde le qualità timbriche, ed è il motivo per cui estendono il modello — [Blackwell & Young 2004](https://eprints-gro.gold.ac.uk/106/1/OSYoung04.pdf) [FT*, sezioni 1-3]

**Pachet (Reflexive Looper).**
- Un agente di loop per ciascun modo esecutivo (accordi, batteria, basso, non melodia), ognuno su un thread dedicato. Ognuno risolve un'ottimizzazione su circa 10 secondi di battiti futuri, con costo di sostituzione armonica più costo di discontinuità, tramite programmazione dinamica con complessità limitata dal quadrato del massimo numero di candidati. L'audio analizzato è quello del musicista, per classificare il modo esecutivo. Gli agenti sono allineati dalla griglia di accordi condivisa, non dall'ascolto reciproco. Nessuno studio controllato nel testo letto — [Marchini, Pachet & Carré 2017](https://www.francoispachet.fr/wp-content/uploads/2021/01/marchini-17nime.pdf) [FT*, sezione 2]
- Continuator: generazione Markov da ingresso MIDI singolo — [Wikipedia, François Pachet](https://en.wikipedia.org/wiki/Fran%C3%A7ois_Pachet) [SEC]

**Tatar & Pasquier 2019.**
- Censisce 78 sistemi e propone una tipologia che comprende architetture, numero di agenti e di ruoli, tipi di comunicazione e modalità di ingresso/uscita; esamina anche i metodi di valutazione — [Tatar & Pasquier 2019](https://doi.org/10.1080/09298215.2018.1511736) [AM, più SEC per l'elenco delle dimensioni]. **Testo integrale non ottenuto**: nessuna distribuzione o conclusione del survey è citabile da queste note.

### Inferences
- In un planner a thread singolo, «direttore» e «agenti» non sono architetture alternative ma due modi di schedulare la stessa computazione. Un ruolo è una funzione, e «ascoltare» significa leggere gli eventi simbolici già prodotti, gratis e senza latenza. L'idea del titolare, ruoli che non possono sentirsi, descrive il codice attuale e non un vincolo strutturale.
- Il pattern ricorrente non è né puro direttore né puro agente: un **piano condiviso sottile** (tempo, griglia armonica, densità, a volte forma) più **parti autonome che leggono uno stato comune**. Lo si ritrova in Kinetic v2, nei Musebot, in Reflexive Looper e in Hutchings. Il direttore sottile di questi sistemi non giudica la musica: pubblica globali e misura scarti aggregati.
- La traiettoria di Kinetic Engine indica che la forma richiede controllo dall'alto, mentre l'interazione locale viene meglio dal basso. È però la riflessione di un compositore sul proprio lavoro, non un risultato sperimentale.
- Il meccanismo di Hutchings è il più trasferibile: una matrice a dimensione fissa che un ruolo scrive e gli altri leggono. Non richiede reti neurali, perché il ruolo della rete armonica è già coperto dal piano armonico deterministico.
- Limite del solo simbolico: nulla dice su mascheramento e timbro, che Coming Together e Blackwell trattano proprio con l'audio. Per rispettare il divieto di analisi audio nel planner, una via è assegnare a ciascun ruolo un profilo spettrale statico e dichiarato (registro e banda) e ragionare su quello. È un'ipotesi da verificare, non un risultato pubblicato.

### Gaps
- Nessun confronto controllato tra direttore e agenti a parità di materiale; ricerca non esaustiva, e il survey Tatar & Pasquier non è stato letto.
- Non ho potuto verificare i dettagli di Hutchings & McCormack 2017, in particolare la valutazione d'ascolto online collegata.
- Nessuna misura pubblicata del costo di calcolo per messaggio o per agente nei sistemi letti.

## 2. Generazione sequenziale, congiunta o per revisione iterativa: che cosa si guadagna e che cosa si perde?

### Takeaway
Il dato più solido è che un'intenzione condivisa migliora la relazione armonica tra le parti rispetto a generatori indipendenti, senza peggiorare il ritmo. La revisione iterativa batte il passaggio unico ordinato nei modelli appresi, ma gli autori attribuiscono il vantaggio a condizionali mal modellate, quindi non si trasferisce automaticamente a un planner a regole. Il dato più utile per CYMATICA viene dall'ablazione di MetaCompose: rendere casuale l'accompagnamento è devastante, mentre una melodia casuale ma nella tonalità peggiora solo una parte dei criteri.

### Cited Findings
**Congiunta contro indipendente (MuseGAN).**
- Tre modelli: *jamming* (M generatori indipendenti con vettori privati), *composer* (un generatore e un vettore condiviso), *hybrid* (M generatori che ricevono un vettore condiviso e uno privato). L'ibrido permette architetture e ingressi diversi per traccia senza perdere l'interdipendenza — [Dong et al. 2018](https://arxiv.org/abs/1709.06298) [FT*, sezioni modelli, metriche e studio]
- Distanza tonale tra tracce (minore = più armonico), generazione da zero, coppie B-G, B-S, B-P, G-S, G-P, S-P: jamming 1,56/1,60/1,54/1,05/0,99/1,05; composer 1,37/1,36/1,30/0,95/0,98/0,91; hybrid 1,34/1,35/1,32/0,85/0,85/0,83. Dati reali: 1,57/1,58/1,51/1,10/1,02/1,04; dati reali rimescolati: 1,59/1,59/1,56/1,14/1,12/1,13 — [stessa fonte, tab. 2](https://arxiv.org/abs/1709.06298) [FT*]
- Le metriche intra-traccia favoriscono invece il jamming, forse perché ogni generatore si concentra sulla propria traccia — [stessa fonte](https://arxiv.org/abs/1709.06298) [FT*]
- Ascolto: 144 soggetti reclutati online, di cui 44 «pro»; nove clip, scala Likert a 5 punti. Da zero, armonia: non-pro 2,83 (jamming), 3,12 (composer), 3,15 (hybrid); pro 2,31, 2,66, 2,92. Giudizio globale: non-pro 2,88, 3,12, 3,16; pro 2,42, 2,73, 2,93. Ritmo quasi invariato: non-pro 3,29, 3,36, 3,33. In generazione condizionata a una traccia, i non-pro preferiscono il jamming. Nelle sezioni lette non compaiono test statistici — [stessa fonte, tab. 3](https://arxiv.org/abs/1709.06298) [FT*]

**Revisione iterativa contro passaggio ordinato (Coconet).**
- Campionamento ancestrale in ordine casuale: O(IT) valutazioni del modello. Gibbs a blocchi ancestrale: O(ITN), proibitivo. Gibbs a blocchi indipendente: O(N), con N che può restare sotto IT. Quello più economico è anche quello migliore — [Huang et al. 2017](https://arxiv.org/abs/1903.07227) [FT*, sezioni 1-3 e 5-7]
- NLL sotto il modello, con T = 32 e N = 100 passi: NADE 1,09 ± 0,06; Gibbs ancestrale con contesto crescente 1,08, 0,97, 0,80, 0,74; Gibbs indipendente 0,52 ± 0,01. Gli autori lo attribuiscono al fatto che le condizionali con poco contesto sono mal modellate — [stessa fonte, sez. 6.2](https://arxiv.org/abs/1903.07227) [FT*]
- Ascolto: 96 valutazioni a coppie su MTurk, frammenti da 34 s. Kruskal-Wallis χ²(2) = 12,23, p < 0,001; Gibbs giudicato più vicino a Bach dell'ancestrale (p < 0,05/3); differenza significativa tra Bach e ancestrale, non tra Bach e Gibbs — [stessa fonte, sez. 6.3](https://arxiv.org/abs/1903.07227) [FT*]

**Revisione stocastica con parti fissabili (DeepBach).**
- Pseudo-Gibbs: si sceglie a caso una voce e un istante e si ricampiona. Le condizionali possono essere incompatibili, quindi manca la garanzia teorica di convergenza; gli autori verificano che la catena non è reversibile. I vincoli unari (note, ritmi, cadenze) si impongono non ricampionando quelle posizioni. In parallelo su GPU è circa dieci volte più veloce, con pochi secondi per corale — [Hadjeres, Pachet & Nielsen 2017](https://arxiv.org/abs/1612.01010) [FT*, sezioni 1-3]
- Test «Bach o computer»: 1272 persone (261, 646 e 365 per livello di competenza); circa il 50% attribuisce a Bach le riarmonizzazioni di DeepBach, e più della metà degli estratti ottiene una maggioranza per Bach, contro un terzo per il modello MLP. Il confronto è tra modelli, non tra strategie di coordinamento — [stessa fonte, sez. 3.2](https://arxiv.org/abs/1612.01010) [FT*]

**Sequenziale puro (BachBot).**
- Codifica in ordine SATB dentro ogni frame, generazione in un solo passaggio. Discriminazione al 51% (446 successi su 874, p = 0,282, 2336 partecipanti senza compenso) — [Liang et al. 2017](https://archives.ismir.net/ismir2017/paper/000156.pdf) [FT*, sezione 3 e metodo]
- Il completamento con parti date riesce peggio della composizione libera, contro l'intuizione. Gli autori propongono due spiegazioni: la ricerca greedy 1-best e un possibile vantaggio dell'origine coerente, tutto Bach o tutto BachBot — [stessa fonte](https://archives.ismir.net/ismir2017/paper/000156.pdf) [FT*]

**Sequenziale a dominio (Song From PI).**
- Melodia, poi accordi e batteria, indipendenti tra loro dato il tema. 27 partecipanti, clip da 30 s: versione completa 81,6% contro Magenta 14,4%; solo melodia 69,6% contro 13,6%; completa contro versione senza allineamento 75,2% contro 12,0%. Alcuni partecipanti notano che la batteria non segue la melodia. È un preprint — [Chu, Urtasun & Fidler 2016](https://arxiv.org/abs/1611.03477) [FT*, sezioni 4-5]

**Sequenziale dall'alto verso il basso (Whole-Song).**
- Quattro livelli a cascata: forma; lead sheet ridotto; lead sheet; accompagnamento. Ogni livello è condizionato da quelli superiori. Sondaggio in doppio cieco con 57 persone: risultati significativamente migliori dei baseline sulle metriche di struttura, soprattutto per l'accompagnamento, e nessun degrado di qualità — [Wang, Min & Xia 2024](https://arxiv.org/abs/2405.09901) [FT*, sezioni 3-5]

**Sistema di gioco a componenti (MetaCompose).**
- Accordi tramite cammino casuale su un grafo di gradi, poi melodia evolutiva su quegli accordi, poi accompagnamento a pattern. 298 partecipanti, 1291 risposte, 10 brani per gruppo. Completo contro versioni degradate: piacevole 654-240 (p = 7,44e-21), armonioso 671-199 (p = 2,05e-29), interessante 482-327 (p = 0,078, non significativo) — [Scirea et al. 2016](https://researcher.itu.dk/ws/files/82418836/scirea_evomusart16.pdf) [FT*, sezioni 4, 5 e 7]
- Ablazione, valori p per piacevole / casuale / armonioso / interessante: accordi casuali 3,23e-2 / 4,90e-4 / 2,68e-2 / 0,528; melodia casuale non vincolata 5,15e-40 / 1,44e-43 / 4,11e-49 / 5,46e-7; **melodia casuale ma nella tonalità** 5,47e-2 / 2,49e-2 / 8,68e-3 / 0,18; **accompagnamento casuale** 5,00e-35 / 6,58e-32 / 3,01e-46 / 4,35e-7 — [stessa fonte, tab. 2-6](https://researcher.itu.dk/ws/files/82418836/scirea_evomusart16.pdf) [FT*]

### Inferences
- Il modello *hybrid* è l'analogo più vicino all'architettura attuale: vettore condiviso (piano) più vettori privati (ruoli). Il risultato incoraggia a mantenere ruoli separati con intenzione comune, ma non prova nulla su un planner a regole. I dati sono rock, il modello è appreso, e non ci sono test statistici.
- Due cautele sul dato MuseGAN. Composer e hybrid scendono **sotto** i dati reali in distanza tonale, quindi più armonico non significa più musicale: l'obiettivo di un termine di punteggio dovrebbe essere una fascia, non un minimo. Inoltre i dati reali rimescolati distano pochissimo da quelli originali, quindi la metrica discrimina poco.
- Il vantaggio Gibbs di Coconet non si può portare a CYMATICA come tale, perché nasce da condizionali deboli. Quel che si trasferisce è l'idea della **riscrittura**: un passaggio aggiuntivo che corregge scelte già fatte, con costo proporzionale al numero di passi e non al prodotto dei ruoli.
- Applicato a CYMATICA, il risultato D di MetaCompose dice che Vettore solo-scala è probabilmente già «piacevole» ma paga su armonia e casualità percepita. Il risultato E dice che il legame tra accompagnamento e armonia pesa moltissimo. L'estensione è prudente: quello studio usa musica tonale in stile generico e sostituisce le parti con il caso, senza confrontare ordini diversi.
- Il dato BachBot avverte che, se una parte resta fissa e le altre vengono riempite in modo greedy, il risultato può peggiorare. Il motif di Vettore è fisso per scelta, quindi le parti che si adattano a esso andrebbero provate su più di un candidato, non su uno solo.

### Gaps
- Nessuno studio confronta sequenziale, congiunto e iterativo sullo stesso modello e in uno stile vicino a CYMATICA. Coconet confronta solo procedure di campionamento; MuseGAN solo architetture apprese.
- Mancano esperimenti su quanti passi di revisione bastino in un sistema a regole.
- MuseGAN e i lavori multi-traccia più recenti (MMM e simili) non offrono test di ordine o di rigenerazione controllati nelle parti lette o trovate.

## 3. Quale ordine delle parti è raccomandato e perché? Esistono prove?

### Takeaway
Non ho trovato alcuno studio d'ascolto controllato che confronti ordini diversi. Le uniche misure comparative riguardano l'efficienza dei solutori, e dicono che l'ordine dipende dal problema. Nelle pratiche pubblicate l'ordine segue le dipendenze del genere: armonia prima per la musica corale e i sistemi tonali, batteria prima per l'EDM, melodia prima in un caso pop. In tutti i sistemi gerarchici la forma viene prima di tutto.

### Cited Findings
**Efficienza (unica prova comparativa).**
- Per un contrappunto fiorito con ritmo e altezze vincolati, visitare le variabili da sinistra a destra per tempo di attacco è tre ordini di grandezza più veloce che determinare prima tutto il ritmo. Per i CSP di contrappunto armonico conviene spesso fissare prima l'armonia e poi le altezze delle note. La conclusione è che l'ordine ottimale dipende dal problema — [Anders & Miranda 2011, sez. 4.5](https://doi.org/10.1145/1978802.1978809) [FT*, tramite [PDF aperto](https://cm-mail.stanford.edu/pipermail/cmdist/attachments/20111228/c7bd1856/attachment.pdf); misure di Anders 2007, SEC]

**Ordini dichiarati dai sistemi.**

| Sistema | Ordine | Motivazione dichiarata | Verifica |
|---|---|---|---|
| Phon-Amnuaisuk (corali) | scheletro armonico → scheletro del basso → bozza delle altre voci → ornamentazione | adattabilità del sistema | [SEC via Anders & Miranda](https://doi.org/10.1145/1978802.1978809) |
| Coppelia | piano di funzioni armoniche (Aaron) → accordi a quattro voci (COMPOzE) | due sottomodelli separati | [SEC via Anders & Miranda](https://doi.org/10.1145/1978802.1978809) |
| CHORAL | scheletro di accordi, modulazione e cadenze, poi generazione melodica con cura delle voci esterne | circa 350 vincoli sui due sottocompiti | [SEC via Anders & Miranda](https://doi.org/10.1145/1978802.1978809) |
| MetaCompose (giochi) | accordi → melodia → accompagnamento | la composizione è un'astrazione identitaria | [FT*](https://researcher.itu.dk/ws/files/82418836/scirea_evomusart16.pdf) |
| GESMI (EDM) | forma → batteria → percussioni → basso (attacchi dalla batteria) → synth (attacchi dal basso) → correzione di tutte le altezze secondo l'armonia implicita del basso | come lavorerebbe un produttore, dall'alto e dal basso insieme | [FT*, sezioni generazione e basso](http://www.sfu.ca/~eigenfel/Eigenfeldt%20ICCC%2013.pdf) |
| GEDMAS (EDM) | forma (Markov di 1° ordine) → sequenze strumentali | corpus di 24 brani breakbeat | [AM](https://metacreation.net/gedmas/) |
| Hutchings 2020 | contesto armonico → linea più acuta → linea più grave → linee interne → percussioni (dalla linea più grave) | pratica di armonizzazione e contrappunto | [FT*](https://arxiv.org/abs/1907.01154) |
| Song From PI | melodia → accordi e batteria | nel corpus l'accordo correla con la melodia | [FT*](https://arxiv.org/abs/1611.03477) |
| Whole-Song | forma → melodia ridotta **e** accordi insieme → lead sheet → accompagnamento | gerarchia compositiva | [FT*](https://arxiv.org/abs/2405.09901) |
| Coconet | nessun ordine privilegiato | la procedura non è legata a una direzione causale | [FT*](https://arxiv.org/abs/1903.07227) |

**Tracce misurate dell'ordine.**
- In BachBot gli errori di predizione sono più alti per soprano e contralto che per tenore e basso. Gli autori lo spiegano con l'ordine SATB: il soprano si predice senza contesto, il basso con tutto il contesto. La discriminabilità segue la stessa linea, dal soprano (la più facile) al basso (la più difficile), ma gli autori ammettono anche che l'ascoltatore segue di più la melodia. Propongono di studiare ordini diversi come lavoro futuro — [Liang et al. 2017](https://archives.ismir.net/ismir2017/paper/000156.pdf) [FT*]
- In Coconet gli ordinamenti casuali danno verosimiglianza significativamente migliore di quelli cronologici. Il dato riguarda l'addestramento di un modello orderless, non l'ascolto — [Huang et al. 2017, sez. 6.1](https://arxiv.org/abs/1903.07227) [FT*]

**Stato del codice CYMATICA (ordine di fatto).**
- Dentro una battuta l'ordine è Pulso → Corpo → Trama → Vettore, ma le dipendenze reali sono poche: Trama salta gli step dove suona Pulso e prende le altezze dalla voicing della battuta; Vettore non legge né accordo né battuta — [`music_planner.cpp`, righe 1014-1042](../../../../engine/audio/music_planner.cpp)
- `phrase.chords` viene letto solo alle righe 905, 948 e 1130: apertura del pad, ciclo di battuta e fallback. La trasformazione del motivo non riceve l'accordo (riga 931) — [`music_planner.cpp`](../../../../engine/audio/music_planner.cpp)
- L'«ancora» del motivo si aggancia ai gradi 0, 2, 4 della scala, cioè alla triade di tonica, non all'accordo della battuta (righe 24 e 136-143). I vincoli hard sono solo strutturali e di capacità: nessuno è armonico (righe 1140-1222) — [`music_planner.cpp`](../../../../engine/audio/music_planner.cpp)
- Tutti i parametri di ruolo dipendono dallo stesso indice k (righe 929-938): i 6 candidati sono 6 punti di un unico filo, quindi un motivo non si può abbinare al ritmo di Trama di un altro candidato — [`music_planner.cpp`](../../../../engine/audio/music_planner.cpp)

### Inferences
- La regola che regge l'intera tabella è informativa, non estetica: **una parte va generata dopo quelle che deve leggere**. Il vero ordine è una dipendenza tra griglie, non una classifica di importanza.
- Conviene separare due griglie: la griglia temporale (dove cadono gli attacchi), in cui GESMI va da batteria a basso a synth, e la griglia delle altezze (quali note sono disponibili), in cui armonia e contesto vengono prima. Questa separazione concilia l'apparente conflitto tra «armonia prima» e «ritmo prima»; è una lettura mia, non un risultato sperimentale.
- Proposta per CYMATICA da verificare: forma → frase → armonia → griglia di Pulso → Corpo → Trama → Vettore. Il motivo è fisso per identità, quindi per i quattro ruoli funziona come un *cantus firmus*: va stabilito prima, ma adattato per ultimo, e nelle battute in cui l'ancora cade fuori accordo deve cedere uno dei due. Hutchings è l'unico sistema letto con una regola esplicita su chi cede.
- Il dato di Whole-Song suggerisce di non separare troppo melodia e accordi al livello più astratto: melodia ridotta e accordi nascono insieme.
- La correzione finale di GESMI ricorda che anche un ordine ben scelto può richiedere un ultimo passaggio di aggiustamento verticale.

### Gaps
- Nessuna prova d'ascolto su un ordine contro un altro. La ricerca di studi empirici sul processo compositivo umano ha restituito solo testimonianze di forum e blog, inutilizzabili come prova; manca una ricerca mirata in psicologia della musica.
- L'ordine interno di Band-in-a-Box non è documentato nelle fonti trovate.
- GESMI riporta selezione a festival, non una validazione d'ascolto.

## 4. Come risolvono i conflitti tra parti i sistemi a regole e a vincoli: vincoli hard o punteggi soft?

### Takeaway
La soluzione dominante è filtrare con pochi vincoli hard e ordinare con preferenze soft. Le regole musicali sono perlopiù linee guida, quindi rappresentarle come vincoli hard rende il problema sovravincolato. Esistono però altri meccanismi utili, tra cui consumare una risorsa condivisa, far tacere una parte e correggere a posteriori. CYMATICA ha già il filtro e il punteggio; manca il contenuto armonico tra ruoli.

### Cited Findings
- Servono vincoli hard per ciò che definisce il materiale, come le classi di altezze di un accordo, e soft per le molte regole che sono solo indicazioni e si possono violare quando il problema è sovravincolato — [Anders & Miranda 2011, sez. 5](https://doi.org/10.1145/1978802.1978809) [FT*]
- La maggior parte dei sistemi ha vincoli hard come primitiva e simula quelli soft: con euristiche sull'ordine dei valori (PWConstraints: la regola restituisce un numero, e la più importante prevale, senza garanzia di ottimalità) o con vincoli reificati (Strasheela). OMClouds è l'eccezione: tutto è funzione di costo, quindi un problema non può risultare sovravincolato, ma mancano i vincoli hard veri — [stessa fonte, sez. 4 e tab. III](https://doi.org/10.1145/1978802.1978809) [FT*]
- Su un problema senza soluzione, i sistemi con propagazione lo segnalano spesso subito, mentre quelli a backtracking possono cercare molto a lungo — [stessa fonte, sez. 5](https://doi.org/10.1145/1978802.1978809) [FT*]
- Ordini di grandezza del numero di regole: CHORAL circa 350; Tsang & Aitken 20 — [stessa fonte](https://doi.org/10.1145/1978802.1978809) [SEC per i due sistemi]
- MetaCompose separa 3 vincoli hard sulla melodia (niente salti oltre la quinta, almeno il 50% di seconde, nessuna ripetizione immediata) da 3 obiettivi soft (risoluzione dei salti, note del salto nell'accordo, prima nota dell'accordo nell'accordo). Gli obiettivi non vengono sommati: restano su un fronte di Pareto, e gli autori osservano che soddisfarli tutti renderebbe la musica meccanica — [Scirea et al. 2016](https://researcher.itu.dk/ws/files/82418836/scirea_evomusart16.pdf) [FT*]
- Reflexive Looper: vincolo hard = candidati entro una soglia di distanza armonica; soft = somma di costi di sostituzione e discontinuità — [Marchini, Pachet & Carré 2017](https://www.francoispachet.fr/wp-content/uploads/2021/01/marchini-17nime.pdf) [FT*]
- Quando l'insieme dei vincoli risulta insoddisfacibile, Pachet et al. lo rilevano con l'arc-consistency e rimandano a tecniche di rilassamento — [Pachet, Roy & Barbieri 2011](https://www.ijcai.org/Proceedings/11/Papers/113.pdf) [FT*]
- Hutchings usa soglie, consumo di risorse e silenzio: se nessun frammento è abbastanza adatto, l'agente non suona, e gli autori lo presentano come buona orchestrazione — [Hutchings & McCormack 2020](https://arxiv.org/abs/1907.01154) [FT*]
- GESMI limita le continuazioni a quelle attestate nel corpus insieme alla parte già generata, quindi corregge le altezze sull'armonia implicita del basso — [Eigenfeldt & Pasquier 2013](http://www.sfu.ca/~eigenfel/Eigenfeldt%20ICCC%2013.pdf) [FT*]
- Coming Together risolve i disaccordi con medie di gruppo, voti casuali e, se fallisce, azzeramento collettivo — [Eigenfeldt 2010](http://www.sfu.ca/~eigenfel/ComingTogether_Beauty&Truth.pdf) [FT*]
- In CYMATICA la pipeline è già vincoli hard → simulazione dei furti di voce → punteggio intero → ordine totale con hash di spareggio → fallback a livelli. Il punteggio pesa 25/25/25/25 voice leading, tensione, identità del motivo e ritmo, con penalità sui furti. Densità e tensione sommano più ruoli, ma non li confrontano — [`music_planner.cpp`, righe 1059-1066 e 1247-1308](../../../../engine/audio/music_planner.cpp); [`music_planner.h`, righe 440-443](../../../../engine/audio/music_planner.h)

### Inferences
**Catalogo dei meccanismi di risoluzione, con compatibilità per il planner:**

| Meccanismo | Esempio | Costo | Adatto? |
|---|---|---|---|
| Filtro hard + punteggio | CYMATICA, Reflexive Looper | fisso | sì, già presente |
| Risorsa consumata su matrice condivisa | Hutchings | fisso | sì; è il candidato più interessante |
| Silenzio o rarefazione di una parte | Hutchings | fisso | sì, e ha valore musicale |
| Priorità o ruolo guida | Hutchings | fisso | sì, ma con regola deterministica, non con la confidenza di una rete |
| Correzione verticale finale | GESMI | fisso | sì, ma rischia di intaccare il motivo |
| Curatela a monte | Musebot | nessuno a runtime | sì, nella tabella delle ricette |
| Pareto senza scalarizzazione | MetaCompose | variabile | solo se poi la scelta è deterministica |
| Azzeramento e nuovo tentativo | Coming Together | illimitato | no |

- Un termine di punteggio da solo non basta. Se nessuno dei 6 candidati è verticalmente buono, il punteggio sceglie il meno peggio, e aggiungerlo sposta soltanto i pesi. Il coordinamento dovrebbe agire **durante la generazione**, come fanno Hutchings e GESMI, non solo durante la selezione. Il punteggio resta utile per scegliere tra alternative già compatibili.
- Le relazioni dipendenti dall'accordo candidate a vincolo hard sono poche; la maggior parte (ancore fuori accordo, urti di registro tra Trama e Vettore, densità sovrapposte) è meglio trattata come soft, in linea con Anders & Miranda.
- L'aggiunta di un termine incrociato cambia il profilo di scoring e quindi richiede una nuova versione esplicita, con valori iniziali dichiarati e criterio di valutazione, come prevede `AGENTS.md`.

### Gaps
- Nessuna fonte misura come scegliere i pesi tra termini verticali e orizzontali in un sistema a regole.
- Il rapporto tra quantità di vincoli e qualità percepita non è studiato in modo controllato nelle fonti lette, salvo l'ablazione di MetaCompose. Anders & Miranda notano che manca persino un insieme di benchmark per i CSP musicali.

## 5. Quali approcci mantengono determinismo esatto, budget logici fissi e replay da seed e traccia di decisioni?

### Takeaway
Tutti gli approcci simbolici eseguiti in un unico thread, con ordine fisso e un budget in passi logici, sono deterministici. La differenza vera è se il budget **garantisce un risultato valido**. La generazione sequenziale con lettura e la programmazione dinamica lo garantiscono; backtracking, ricerca locale ed evoluzione richiedono un taglio e un fallback. Le negoziazioni asincrone, gli ensemble su rete e tutto ciò che ascolta l'audio restano fuori.

### Cited Findings
- Compilazione dei vincoli in Markov non omogeneo: se i vincoli restano nell'ambito di Markov (unari e binari adiacenti, per l'ordine 1), l'arc-consistency basta a calcolare una sola volta le matrici di transizione, dopodiché non serve altra propagazione né backtracking. Coefficienti in O(L × n²), e misure lineari nella lunghezza L; il tempo di generazione cresce linearmente con l'alfabeto n, quello di compilazione quadraticamente. La distribuzione del modello originale è preservata in proporzione — [Pachet, Roy & Barbieri 2011](https://www.ijcai.org/Proceedings/11/Papers/113.pdf) [FT*, sezioni 1, 3, 4 e misure]
- Avvertenza degli stessi autori: i vincoli di controllo non si riducono a costi cumulativi, quindi il principio di Bellman non vale nel loro contesto, e i vincoli oltre l'ambito di Markov richiedono altro — [stessa fonte](https://www.ijcai.org/Proceedings/11/Papers/113.pdf) [FT*]
- Programmazione dinamica su costi additivi: complessità limitata dal quadrato del massimo numero di candidati (Reflexive Looper) — [Marchini, Pachet & Carré 2017](https://www.francoispachet.fr/wp-content/uploads/2021/01/marchini-17nime.pdf) [FT*]
- Costo dei CSP musicali: pochi millisecondi per problemi semplici, pochi secondi per armonizzazione o contrappunto fiorito, ma i problemi complessi possono richiedere molto tempo. La strategia efficiente dipende dal problema. Il backtracking ha ordine statico e rischia thrashing e ricerca ridondante — [Anders & Miranda 2011, sez. 4.1, 5, 7](https://doi.org/10.1145/1978802.1978809) [FT*]
- OMClouds usa una ricerca locale euristica su funzioni di costo a partire da una soluzione casuale, senza vincoli hard — [stessa fonte](https://doi.org/10.1145/1978802.1978809) [FT*]
- Hutchings: ricerca esaustiva di trasposizioni e spostamenti, quindi finita — [Hutchings & McCormack 2020](https://arxiv.org/abs/1907.01154) [FT*]
- Coconet: costo O(N) in passi di Gibbs. DeepBach: numero massimo di iterazioni come parametro, nessuna garanzia di convergenza — [Huang et al. 2017](https://arxiv.org/abs/1903.07227) [FT*]; [Hadjeres, Pachet & Nielsen 2017](https://arxiv.org/abs/1612.01010) [FT*]
- Kinetic v3: generazione e analisi di popolazioni in tempo reale con perdita di reattività — [Eigenfeldt & Pasquier 2009](http://www.sfu.ca/~eigenfel/A%20Realtime%20Generative%20Music%20System.pdf) [FT*]
- Coming Together: non determinismo e convergenza non garantita sono parte del progetto — [Eigenfeldt 2010](http://www.sfu.ca/~eigenfel/ComingTogether_Beauty&Truth.pdf) [FT*]
- Musebot: applicazioni autonome su messaggi OSC, avvio con conteggio degli heartbeat — [Eigenfeldt 2016](https://zenodo.org/records/4285186/files/Eigenfeldt_Musebots.pdf) [FT*]
- MetaCompose: ricerca evolutiva FI-2POP con NSGA-II. L'articolo letto non riporta dimensione della popolazione, numero di generazioni né tempi — [Scirea et al. 2016](https://researcher.itu.dk/ws/files/82418836/scirea_evomusart16.pdf) [FT*]
- Il repository impone già budget logici fissi, ordinamento stabile, flussi casuali isolati e replay dalle decisioni accettate — [`AGENTS.md`, sez. 3](../../../../AGENTS.md); il planner ha 4-8 candidati, default 6 — [`music_planner.h`, righe 228 e 466](../../../../engine/audio/music_planner.h)

### Inferences
**Quali progetti possono girare sul thread di pianificazione, con lavoro fisso e senza analisi audio.** La tabella è una mia valutazione basata sui fatti citati sopra; nessuna fonte la fornisce così.

| Progetto | Sul planner a lavoro fisso, senza audio? | Determinismo | Risultato valido garantito entro il budget? | Limite di lavoro |
|---|---|---|---|---|
| A. Piano condiviso + ruoli indipendenti (attuale) | **Sì** | esatto | sì | C × B × S |
| B. Sequenziale con lettura, ordine fisso (GESMI, MetaCompose senza evoluzione) | **Sì** | esatto | sì, con silenzio come ripiego | C × somma dei ruoli |
| C. Matrice condivisa a consumo (Hutchings senza reti) | **Sì** | esatto, con valori interi | sì, con silenzio come ripiego | matrice 12 × S per frase, ricerca finita |
| D. Candidati per ruolo con fascio ridotto | **Sì** | esatto con spareggio stabile | sì | L × K × R |
| E. Programmazione dinamica sugli step (Reflexive Looper) | **Sì** | esatto con spareggio stabile | sì, se ogni posizione ha un candidato | P × K² |
| F. Compilazione di vincoli (Pachet 2011) | **Sì**, se i vincoli restano nell'ambito | esatto solo con aritmetica intera o razionale | sì, oppure insoddisfacibilità rilevata | O(L × n²) |
| G. Revisione a K passaggi fissi, ordine fisso (Gibbs reso deterministico) | **Sì** | esatto | sì, se si riparte da una soluzione già valida | K × R × C |
| H. Backtracking con budget di nodi | **Sì, con taglio** | esatto, senza parallelismo | **no**: serve un fallback | budget di nodi |
| I. Ricerca locale, VNS, OMClouds | **Sì, con taglio** | esatto con flusso isolato | **no** | iterazioni fisse |
| J. Evolutiva a popolazione × generazioni fisse | **Sì, con taglio**, ma costosa | esatto con flusso isolato | **no** | popolazione × generazioni × valutazione |
| K. Negoziazione in lockstep, turni fissi | Solo come variante di G | esatto solo se sincrona | no | turni × agenti |
| L. Negoziazione asincrona (Coming Together, Kinetic) | **No** | no | no | illimitato |
| M. Ensemble su rete (Musebot) | **No** | no | no | dipende dal tempo reale |
| N. Sciami a dinamica continua | Tecnicamente sì, ma senza scopo | solo a passo fisso e aritmetica definita | no | passi × particelle |
| O. Modalità con analisi audio (AUDIO di Coming Together, Reflexive Looper) | **No**: vietato | — | — | — |
| P. Reti neurali (Coconet, DeepBach, MuseGAN, Song From PI, Whole-Song) | Fuori perimetro: interessano solo come progetti di coordinamento | oneroso | no | — |

Legenda: C candidati, B battute, S step, L profondità del fascio, K candidati per ruolo, R ruoli, P posizioni.

- **Il risparmio della sequenza è combinatorio.** Con 6 opzioni per ciascuno dei 4 ruoli, il prodotto congiunto vale 6⁴ = 1296 combinazioni; una sequenza greedy ne valuta 24; un fascio di ampiezza 2 ne valuta poche decine. Il costo fisso è esattamente ciò che rende la soluzione compatibile con le regole Pure Seed. Il prezzo è la miopia: una scelta iniziale non si ripara, ed è ciò che la revisione G dovrebbe correggere.
- **Per G il determinismo richiede ordine dei ruoli fisso e passi fissi.** Il Gibbs vero campiona posizioni casuali e non offre convergenza nemmeno in DeepBach. La variante deterministica è un miglioramento coordinato a passaggi limitati, che non garantisce un ottimo, ma se accetta solo mosse valide conserva sempre un candidato ammissibile. Non ho una fonte musicale che la valuti.
- **F presenta un rischio specifico:** la rinormalizzazione lavora con probabilità reali, mentre il planner vuole aritmetica intera. Va verificata prima di adottarla. In più, i vincoli di fine frase e di ancora rientrano bene nell'ambito, mentre quelli di ripetizione a lungo raggio no.
- **Replay.** Ogni fase di ruolo dovrebbe avere il proprio slot in `deriveDecisionId` e il proprio spareggio, e la traccia dovrebbe registrare la scelta per ruolo, non solo il candidato vincente. Con la selezione a sei candidati l'indice unico basta; con scelte per ruolo non basta più. Un budget esaurito deve essere una decisione registrata, non un effetto del tempo di orologio.
- **Il parallelismo va escluso.** Anders & Miranda vedono nella ricerca parallela il futuro delle prestazioni, ma per CYMATICA introdurrebbe un ordine dipendente dalla schedulazione.

### Gaps
- Nessuna delle fonti misura il proprio sistema in termini di determinismo o replay: l'intera colonna è analisi mia, non un risultato pubblicato.
- Non ho misurato il costo reale sul planner di CYMATICA di B-G. Per la decisione servirebbe una misura nel repository, con procedura e ripetizioni.
- Non ho letto Herremans & Sörensen sulla VNS per il contrappunto, nota solo tramite Coconet (SEC), né studi sul budget ottimale della ricerca locale in musica.

## 6. Quali risultati vengono da studi d'ascolto controllati e quali sono solo affermazioni degli autori?

### Takeaway
Otto sistemi hanno una valutazione con ascoltatori, e di questi solo due confrontano direttamente progetti di coordinamento (Coconet e MuseGAN). Nessuno riguarda un sistema a regole deterministico nello stile di CYMATICA. I sistemi multi-agente a messaggi e a sciame, così come i sistemi a vincoli letti tramite il survey, non riportano studi controllati nelle parti lette.

### Cited Findings
**Forza delle prove.**

| Fonte | Tipo di prova | Partecipanti | Confronta il coordinamento? | Limiti principali |
|---|---|---|---|---|
| [Huang et al. 2017](https://arxiv.org/abs/1903.07227) | ascolto a coppie con statistica | 96 valutazioni (MTurk) | **sì**: procedure di campionamento | Bach; 4 campioni per sorgente; modello appreso |
| [Dong et al. 2018](https://arxiv.org/abs/1709.06298) | Likert, reclutamento tramite cerchie sociali | 144 (44 pro) | **sì**: jamming, composer, hybrid | nessun test statistico nelle sezioni lette; analogia lontana |
| [Hutchings & McCormack 2020](https://arxiv.org/abs/1907.01154) | gioco controllato, due gruppi | 34 | no: AMS contro colonna sonora | confronti per gioco non significativi tra soggetti; misure auto-riportate |
| [Scirea et al. 2016](https://researcher.itu.dk/ws/files/82418836/scirea_evomusart16.pdf) | ablazione a coppie, test binomiale | 298 (1291 risposte) | in parte: contributo delle parti | componenti sostituite con il caso; nessun confronto tra ordini; autoselezione online |
| [Hadjeres, Pachet & Nielsen 2017](https://arxiv.org/abs/1612.01010) | discriminazione | 1272 | no: confronto tra modelli | Bach; estratti da 12 s |
| [Liang et al. 2017](https://archives.ismir.net/ismir2017/paper/000156.pdf) | discriminazione web | 2336 | no | ambiente non controllato; Bach |
| [Wang, Min & Xia 2024](https://arxiv.org/abs/2405.09901) | doppio cieco, Likert | 57 | in parte: gerarchia contro baseline | cambiano anche i modelli |
| [Chu, Urtasun & Fidler 2016](https://arxiv.org/abs/1611.03477) | voti a coppie, ablazione | 27 | in parte | preprint, campione piccolo, baseline monotraccia |
| Kinetic Engine, Coming Together, Musebot, Swarm Music, Reflexive Looper | valutazione artistica o descrittiva degli autori | 0 | no | nessun gruppo di confronto |
| Sistemi a vincoli (CHORAL ecc.) | giudizio degli autori, tramite il survey | 0 nelle parti lette | no | manca persino un benchmark comune |

- Il giudizio secondo cui CHORAL raggiunge la competenza di uno studente di talento è attribuito al suo autore; Anders & Miranda lo riportano come tale — [Anders & Miranda 2011](https://doi.org/10.1145/1978802.1978809) [FT* del survey, SEC per CHORAL]
- Il paper DeepBach riporta che, secondo l'autore di CHORAL, l'output non suona come Bach se non per pattern occasionali — [Hadjeres, Pachet & Nielsen 2017](https://arxiv.org/abs/1612.01010) [FT*, SEC per CHORAL]
- Un confronto diretto tra metodi a vincoli è escluso dagli stessi autori del survey per mancanza di problemi di riferimento comuni — [Anders & Miranda 2011](https://doi.org/10.1145/1978802.1978809) [FT*]
- La valutazione successiva di MetaCompose comprende due studi utente e un'annotazione dell'umore in tempo reale; numeri non verificati — [Scirea et al. 2017](https://doi.org/10.1007/s10710-017-9307-y) [SEC, da sintesi di ricerca]

**Registro delle fonti.**

| Fonte | URL / DOI | Livello | Copertura effettiva |
|---|---|---|---|
| Hutchings & McCormack 2020, IEEE ToG 12(3) | [10.1109/TG.2019.2921979](https://doi.org/10.1109/TG.2019.2921979); [arXiv:1907.01154](https://arxiv.org/abs/1907.01154) | FT* | preprint: sez. VI.B (agenti), VII (implementazione, studio), metadati Crossref |
| Hutchings & McCormack 2017, EvoMUSART | [10.1007/978-3-319-55750-2_8](https://doi.org/10.1007/978-3-319-55750-2_8) | SEC | sintesi da ricerca; Springer non accessibile |
| Huang et al. 2017 (Coconet), ISMIR | [arXiv:1903.07227](https://arxiv.org/abs/1903.07227) | FT* | sez. 1-3, 5, 6, 7; tabelle ricostruite dall'estrazione |
| Hadjeres, Pachet & Nielsen 2017 (DeepBach), ICML | [arXiv:1612.01010](https://arxiv.org/abs/1612.01010) | FT* | sez. 1, 2.3-2.4, 3; passi cercati per parola chiave |
| Dong et al. 2018 (MuseGAN), AAAI | [arXiv:1709.06298](https://arxiv.org/abs/1709.06298) | FT* | modelli, metriche, risultati, tab. 2-3 |
| Liang et al. 2017 (BachBot), ISMIR | [PDF](https://archives.ismir.net/ismir2017/paper/000156.pdf) | FT* | codifica, sez. 3 e discussione |
| Chu, Urtasun & Fidler 2016 (preprint) | [arXiv:1611.03477](https://arxiv.org/abs/1611.03477) | FT* | sez. 1, 4, 5 |
| Wang, Min & Xia 2024, ICLR | [arXiv:2405.09901](https://arxiv.org/abs/2405.09901) | FT* | sez. 3 e valutazione soggettiva; figure non leggibili |
| Anders & Miranda 2011, ACM CSUR | [10.1145/1978802.1978809](https://doi.org/10.1145/1978802.1978809); [PDF](https://cm-mail.stanford.edu/pipermail/cmdist/attachments/20111228/c7bd1856/attachment.pdf) | FT* | sez. 3 (parti), 4.1, 4.4, 4.5, 5, 7 |
| Pachet, Roy & Barbieri 2011, IJCAI | [PDF](https://www.ijcai.org/Proceedings/11/Papers/113.pdf) | FT* | introduzione, sez. 3-4, misure |
| Marchini, Pachet & Carré 2017, NIME | [PDF](https://www.francoispachet.fr/wp-content/uploads/2021/01/marchini-17nime.pdf) | FT* | sez. 2 |
| Scirea et al. 2016 (MetaCompose), EvoMUSART | [10.1007/978-3-319-31008-4_14](https://doi.org/10.1007/978-3-319-31008-4_14); [manoscritto](https://researcher.itu.dk/ws/files/82418836/scirea_evomusart16.pdf) | FT* | sez. 4, 5, 6, 7 |
| Eigenfeldt & Pasquier 2009, Generative Art | [PDF](http://www.sfu.ca/~eigenfel/A%20Realtime%20Generative%20Music%20System.pdf) | FT* | integrale |
| Eigenfeldt 2007, Virtual Primitive Drum | [PDF](http://www.sfu.ca/~eigenfel/Plymouth07_VirtualDrum.pdf) | FT* | sezioni direttore e interazione |
| Eigenfeldt 2010, Coming Together | [PDF](http://www.sfu.ca/~eigenfel/ComingTogether_Beauty&Truth.pdf) | FT* | sez. 1-6 |
| Eigenfeldt & Pasquier 2013, ICCC | [PDF](http://www.sfu.ca/~eigenfel/Eigenfeldt%20ICCC%2013.pdf) | FT* | ordine di generazione, percussioni, basso, conclusioni |
| Eigenfeldt 2016, Musebots at One Year, MUME | [PDF](https://zenodo.org/records/4285186/files/Eigenfeldt_Musebots.pdf) | FT* | passi selezionati |
| Blackwell & Young 2004, Organised Sound | [PDF](https://eprints-gro.gold.ac.uk/106/1/OSYoung04.pdf) | FT* | sez. 1-3 |
| Tatar & Pasquier 2019, JNMR | [10.1080/09298215.2018.1511736](https://doi.org/10.1080/09298215.2018.1511736) | AM | solo abstract e metadati; dimensioni della tipologia da SEC |
| Anderson, Eigenfeldt & Pasquier 2013 (GEDMAS), AIIDE/MUME | [AAAI](https://ojs.aaai.org/index.php/AIIDE/article/view/12649); [progetto](https://metacreation.net/gedmas/) | AM | pagina del progetto, abstract |
| Bown, Carey & Eigenfeldt 2015, ISEA | [ISEA](https://isea-archives.siggraph.org/presentation/manifesto-for-a-musebot-ensemble-a-platform-for-live-interactive-performance-between-multiple-autonomous-musical-agents) | SEC | tramite articoli successivi |
| Scirea et al. 2017, GPEM | [10.1007/s10710-017-9307-y](https://doi.org/10.1007/s10710-017-9307-y) | SEC | sintesi da ricerca |
| Pachet, Continuator | [Wikipedia](https://en.wikipedia.org/wiki/Fran%C3%A7ois_Pachet) | SEC | nessuna lettura diretta |
| CHORAL, Tsang & Aitken, Coppelia, Phon-Amnuaisuk, Anders 2007, Pachet & Roy 2001 | tramite [Anders & Miranda 2011](https://doi.org/10.1145/1978802.1978809) | SEC | mai letti direttamente |
| Herremans VNS, Lattner et al. | tramite [Huang et al. 2017](https://arxiv.org/abs/1903.07227) | SEC | mai letti direttamente |

### Inferences
- Le prove dirette sul coordinamento si riducono a due studi, entrambi su modelli appresi. Per tutto il resto la letteratura offre progetti plausibili, non verdetti. Una scelta in CYMATICA non può quindi poggiare solo su di essa: richiede ascolti e metriche propri su questo materiale, come prevede la pratica del progetto (CACP, ascolto esperto).
- I test «Bach o computer» misurano l'imitazione di uno stile, che non è l'obiettivo di CYMATICA. Valgono per il metodo di coordinamento, non come misura di qualità artistica. La richiesta di musica che «ricordi, sviluppi, sorprenda e ritorni» non è misurata da nessuno di questi studi.
- L'ablazione alla MetaCompose è il disegno sperimentale più facile da riprodurre sul progetto: stesso seed, stessa forma, con e senza lettura dell'accordo da parte di Vettore. Darebbe una prova diretta che la letteratura non offre.

### Gaps
**Pagine non recuperate.**
- `core.ac.uk/download/157695848.pdf` e `130975351.pdf` (Anders & Miranda): HTTP 403; usato invece il PDF di cm-mail.stanford.edu.
- `core.ac.uk/download/50528908.pdf` (MetaCompose): HTTP 403; usato il manoscritto ITU.
- `ojs.aaai.org` (PDF GEDMAS): bloccato da protezione anti-bot.
- `summit.sfu.ca/item/19665` (Tatar): bloccato da protezione anti-bot; `labs.sonicfield.org`: host non risolvibile; `link.springer.com`: reindirizzato all'autenticazione.
- `www.smc2019.uma.es/.../D2_01_SMC2019_paper.pdf`: sito temporaneamente fuori servizio.

**Limiti di questa ricerca.**
- Il testo integrale di Tatar & Pasquier 2019 non è stato ottenuto, quindi una delle fonti richieste resta a livello AM.
- Blackwell & Bentley 2002 non è stato letto: Swarm Music è noto qui da Blackwell & Young 2004.
- Le tabelle numeriche provengono da estrazione automatica. I valori citati sono stati controllati contro il testo estratto, non contro l'impaginato originale.
- Le ricerche sono state in numero superiore a quello consigliato dalle istruzioni, a causa dei PDF, ma non sono esaustive della letteratura.
