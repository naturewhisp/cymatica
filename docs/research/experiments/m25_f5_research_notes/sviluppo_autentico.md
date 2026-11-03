# Sviluppo musicale autentico e varietà fittizia: definizioni operative, misure e verifica

**Sintesi.** La letteratura non offre una metrica validata dello «sviluppo»: una rassegna del 2025 afferma che metriche specifiche per forma e ripetizione non sono ancora state proposte. Il risultato più utile è negativo e formale: ogni misura che non cambia rimescolando le frasi (entropia, conteggio di materiali distinti, distanza media) è cieca allo sviluppo, perché lo sviluppo è una proprietà dell'ordine. Da qui derivano la definizione proposta (il §7 raccoglie definizioni, misure e procedure), il test con ordine rimescolato sia simbolico sia d'ascolto e l'ablazione a quattro esiti. Con due ascoltatori si possono ottenere solo conclusioni per persona e per clip, mai di popolazione. Quanto scritto nelle sezioni Inferences e nel §7 è **proposta**, non contratto né risultato validato.

**Livelli di verifica usati.**
- **FT**: testo verbatim (estratto con `pdftotext` dal PDF salvato da WebFetch) letto da me. **FT-parz** è lo stesso livello, ma con lettura selettiva: abstract e sezioni citate, non l'intero articolo.
- **FT\***: letto tramite l'estrattore automatico di WebFetch, che risponde a un prompt: non ho visto il testo.
- **AM**: solo abstract o metadati, inclusi gli estratti restituiti dalla ricerca web.
- **SEC**: noto solo tramite una fonte secondaria, indicata.

**Pagine non recuperate:** Abdallah & Plumbley su core.ac.uk (403) e su qmul (redirect); Cheung et al. su cell.com (403); Project MUSE per Ariza (verifica anti-bot); `axon.cs.byu.edu` (host irraggiungibile); `cip2012/draft.tex` (connessione rifiutata); hal.science per Bimbot (accesso negato); scispace per Egermann e per Saunders & Gero (pagina vuota); Springer per Egermann (redirect a login); documentazione ufficiale Wwise (403); `mit.bme.hu` per Boden (restituisce la homepage).

## 1. Che cosa dicono teoria e cognizione sullo sviluppo? Che cosa rende una variazione percepita come imparentata e insieme nuova?

### Takeaway
La teoria definisce lo sviluppo come trasformazione cumulativa e orientata di un materiale, ma la percezione immediata della parentela dipende soprattutto da caratteri di superficie. Una variazione «imparentata e nuova» deve quindi conservare almeno un invariante udibile e cambiare qualcosa di udibile: la parentela soltanto strutturale può restare inudita.

### Cited Findings
**Teoria**
- Schoenberg considerava la variazione in sviluppo uno dei principi compositivi principali dal 1750 circa, ne vedeva in Brahms lo stato più avanzato e ne parlò solo in modo sporadico: il concetto è quindi ricostruito da osservazioni sparse — [Wikipedia, Developing variation](https://en.wikipedia.org/wiki/Developing_variation) [SEC, estratto di ricerca].
- Haimo la descrive come categoria speciale di variazione che implica un processo teleologico — riportato dalla stessa ricerca [SEC; Haimo 1997 non letto].
- Nella teoria citata da Lalitte & Bigand, la forma garantisce unità e coerenza dell'opera; il testo cita Schoenberg 1967 su questo punto — [Lalitte & Bigand 2006](https://lead.ube.fr/wp-content/uploads/2023/09/000296-music-in-the-moment-revisiting-the-effect-of-large-scale-structures.pdf) [FT-parz, OCR rumoroso].
- Il concatenazionismo di Levinson sostiene il contrario: la musica si coglierebbe momento per momento, con strutture solo locali e transitorie — Lalitte & Bigand 2006 [FT-parz].

**Variazione, invarianza, somiglianza**
- La similarità può dipendere da relazioni astratte dagli attributi più che dai loro valori. Le dimensioni portatrici di forma sono quelle su cui si configurano relazioni invarianti e devono essere facili da discriminare. Lo spazio delle trasformazioni di un materiale è limitato. Il materiale interposto può degradare il riconoscimento dell'invarianza — [McAdams & Matzkin 2001](https://www.mcgill.ca/mpcl/files/mpcl/mcadams_2001_ann.n.y.acad_.sci_.pdf) [FT\*, porzione visibile].
- Formula chiave: «although something has changed, something else remains the same» — [McAdams et al. 2004](https://mcgill.ca/mpcl/files/mpcl/mcadams_2004_musperc_1.pdf) [FT-parz].
- Classificazione libera di 34 sottosezioni tematiche con descrizione verbale, N=78 (39 senza esperienza musicale, 39 con 5–25 anni) per il pianoforte e N=40 per l'orchestra. Gli ascoltatori hanno classificato su similarità di superficie: tempo, tessitura ritmico-melodica, registro, contorno, articolazione, in parte l'umore evocato. Fra musicisti e non musicisti: r(559)=.83 nell'esperimento orchestrale, 32% di varianza non spiegata in quello pianistico. La sensibilità non dipendeva in modo sistematico dalla formazione; cambiava il vocabolario — McAdams et al. 2004 [FT-parz].
- Stesse note, altra orchestrazione: le versioni pianistiche spiegano poco meno della metà della varianza delle dissimilarità orchestrali. Le relazioni interne ai temi restano correlate solo per tre temi su cinque (r=.76, .88, .74) — McAdams et al. 2004 [FT-parz].
- McAdams et al. dichiarano coerenti con questo primato della superficie Lamont & Dibben 2001 e altri studi, e collegano il risultato al confronto immediato, dove tali caratteri dominano — McAdams et al. 2004 [FT-parz]; Lamont & Dibben [SEC, tramite McAdams].
- Anche la struttura profonda arriva, ma più debolmente: nell'ordinare variazioni per struttura sottostante, con superfici fuorvianti, i musicisti ottengono il 72% di risposte corrette e i non musicisti il 58%, sopra il caso — Bigand 1990 [SEC, tramite [Bigand & Poulin-Charronnat 2006](https://doi.org/10.1016/j.cognition.2005.11.007), FT-parz].
- Con N=51 valutatori online, la dimensione a bassa complessità (ritmo o altezza) domina il giudizio: modificarla riduce la similarità più che modificare quella complessa — [Ens, Riecke & Pasquier 2017](https://archives.ismir.net/ismir2017/paper/000079.pdf) [FT-parz].

**Memoria, ripetizione, forma**
- Ascoltando lo stesso brano quattro volte di fila, i partecipanti individuano prima ripetizioni piccole (motivi) e poi via via più grandi (frasi) — Margulis 2012, descritto da [Margulis 2013](https://doi.org/10.3389/fpsyg.2013.00167) [FT-parz]. Più esposizioni migliorano il rilevamento delle unità lunghe e peggiorano quello delle brevi — [Margulis 2012](https://collaborate.princeton.edu/en/publications/musical-repetition-detection-across-multiple-exposures/) [AM].
- Ascolto attento: il gradimento segue una U rovesciata con l'esposizione per gli stimoli più ecologici. Ascolto incidentale: il gradimento cresce con l'esposizione — [Szpunar, Schellenberg & Pliner 2004](https://doi.org/10.1037/0278-7393.30.2.370) [AM].
- La finestra percettiva scorrevole è stimata fra 5 e 10 s, fino a 30 s per Levinson — Lalitte & Bigand 2006 [FT-parz].
- Nel modello di tensione di Farbood la finestra attentiva e quella di memoria migliori durano 3 s ciascuna — [Farbood 2012](https://doi.org/10.1525/mp.2012.29.4.387) [FT-parz].

**Forma su larga scala: che cosa si sente davvero**
- Chunk di circa 6 s rimescolati in 6 brani da 3 minuti (20 musicisti, 20 non musicisti), mantenendo primo e ultimo chunk e transizioni lisce. I partecipanti rilevano solo il 17% dei collegamenti alterati nel pop e circa il 13% nella contemporanea, ma giudicano il rimescolato meno coerente. Il gradimento cala solo nel pop. Gli autori concludono per una «moderate sensitivity to large-scale musical structures»; la formazione incide poco. Le correlazioni fra compiti sono basse o moderate (.25–.50) — Lalitte & Bigand 2006 [FT-parz].
- Sempre secondo Lalitte & Bigand [FT-parz]: la chiusura tonale influisce solo per estratti di 30 s (Cook 1987); riordinare movimenti o variazioni non cambia chiaramente i giudizi (Konečni 1984; Gotlieb & Konečni 1985); in Tillmann & Bigand 1996 il retrogrado per chunk non altera coerenza ed espressività, e chi lo ascolta risponde correttamente solo al 43% sulla propria condizione. Gli autori osservano però che la sensibilità sarebbe potuta emergere con un confronto diretto delle due versioni. Tutti questi studi sono [SEC].
- Gli ascoltatori possono non distinguere originale e riarrangiato, e possono non preferire l'originale (Eitan & Granot 2008). Sono sensibili ad alcuni aspetti della forma, come ABA e collocazione dello sviluppo, ma non alla struttura armonica complessiva (Granot & Jacoby 2011, 2012) — tramite [Phillips et al. 2020](https://doi.org/10.3389/fpsyg.2020.01001) [FT-parz], quindi [SEC].
- In 259 spettatori dal vivo l'accordo sui confini di sezione si concentra su interruzioni della tessitura, dinamica e registro; l'effetto della formazione è piccolo e gli ascoltatori non sempre sanno come decidono — Phillips et al. 2020 [FT-parz].

**Aspettativa e sorpresa**
- ITPRA (Immaginazione, Tensione, Predizione, Reazione, Valutazione) descrive l'aspettativa come cinque sistemi di risposta distinti — [Huron 2006, tramite recensione e scheda](https://academic.oup.com/ml/article/89/3/396/1135734) [SEC].
- IDyOM combina un modello a lungo termine, che rappresenta l'esposizione di una vita, e uno a breve termine, che parte vuoto per ogni brano e impara la ripetizione interna. I limiti dichiarati sono: input simbolico, timbro e dinamica non modellati, armonia e polifonia ancora iniziali — [Pearce 2018](https://pmc.ncbi.nlm.nih.gov/articles/PMC6849749/) [FT\*].
- Accordi di canzoni pop: il piacere è alto per forte sorpresa dopo bassa incertezza e per prevedibilità dopo alta incertezza — [Cheung et al. 2019](https://doi.org/10.1016/j.cub.2019.09.067) [AM; numero di partecipanti non verificato].

### Inferences
- «Sviluppo» implica tre proprietà distinte: **derivazione** (viene da qualcosa), **cumulatività** (dipende dallo stadio precedente, non solo dall'originale) e **direzione** (l'ordine conta). La variazione semplice soddisfa solo la prima.
- CYMATICA ha frasi medie di circa 7,6 s (fixture di 182,857 s, 96 battute, 24 frasi): sono vicine ai chunk di Lalitte & Bigand e al limite della finestra percettiva. Differenze fini fra frasi consecutive rischiano di non essere trattenute. È coerente con «frasi quasi identiche» percepite come un solo oggetto, ma non lo dimostra.
- Il «nuovo suono» chiesto dall'esperto è un'opzione legittima: McAdams et al. mostrano che la strumentazione riorganizza la similarità. Diventa sviluppo solo se assume una funzione, per esempio se ritorna o trasforma un materiale; altrimenti è solo differenza.
- **Ipotesi verificabile (da Ens et al.):** Pulso, Trama e Vettore hanno lo stesso schema ritmico per 64 battute (B e A′), e Vettore suona sui passi 0, 4, 8, 12 in tutte le 24 frasi ([ascolto del 2026-11-03](../../../../docs/m2_4_human_listening_2026-11-03.md)). Il ritmo è quindi la dimensione a bassa complessità: cambiarlo dovrebbe pesare più che cambiare le altezze. Una «decisa variazione melodica» a ritmo fisso conserverebbe l'identità, ma rischierebbe di restare poco percepita. Il contesto di Ens (melodie brevi, valutatori online) è lontano da CYMATICA.
- Con ascolto incidentale (Szpunar) il giocatore potrebbe tollerare più ripetizione dell'ascoltatore attento. Valutare solo in ascolto attento può sovrastimare il bisogno di varietà in gioco.

### Gaps
- Non ho letto Schoenberg (*Fundamentals*, *Brahms the Progressive*), Frisch, Haimo, Meyer 1956, Huron 2006, Margulis 2005 né *On Repeat*: nessuna affermazione su questi testi è di prima mano. Di Meyer non cito contenuti perché non ho trovato una fonte letta che lo esponga.
- Welker 1982 e Halpern 1984 (astrazione del tema dalle variazioni), Rosner & Meyer e Prince (primato del ritmo), Tan et al. 2005/2006 (composizioni patchwork) sono comparsi solo in riassunti automatici di ricerca, con attribuzione non verificabile. Non li uso come evidenza.
- Non ho trovato studi percettivi sullo sviluppo motivico in musica elettronica generativa o per videogiochi.

## 2. Quali misure formali esistono e quali predicono i giudizi degli ascoltatori, con quali effetti?

### Takeaway
Information content (IC) ed entropia sono le misure simboliche con il legame più solido a giudizi umani, ma misurano aspettativa nota per nota, non sviluppo. Per auto-similarità, compressione e dinamiche dell'informazione non ho trovato effetti su giudizi di sviluppo; nessun effetto riportato qui riguarda lo sviluppo come tale.

### Cited Findings
| Misura | Che cosa calcola | Legame con giudizi umani | Limite principale | Livello |
|---|---|---|---|---|
| IC, IDyOM | −log₂ della probabilità dell'evento dato il contesto | Fino all'83% della varianza delle aspettative di altezza, su più paradigmi; [Pearce 2018](https://pmc.ncbi.nlm.nih.gov/articles/PMC6849749/) | Cifra massima, soprattutto melodie; nessun coefficiente unico per i rating | FT\* |
| IC ed entropia sul gradimento | Medie per stimolo, pesate per durata | Studio 1, N=43, 55 estratti monofonici da ~30 s: IC 26,3%, entropia 19,1%. Studio 2, N=27, 12 stimoli ripetuti 7 volte: IC 41,6%, entropia 34,9%; [Gold et al. 2019](https://pmc.ncbi.nlm.nih.gov/articles/PMC6867811/) | U rovesciata; interazione IC×entropia solo marginale (p=.06) | FT\* |
| Effetto Wundt in generale | Forma a U rovesciata | Trovato in 15 su 57 studi (Chmiel & Schubert 2017, citati da Gold et al.) | L'optimum non è una legge generale | SEC |
| Complessità percepita | IC manipolato | N=28: seguono soprattutto IC melodico e armonico; le differenze individuali spiegano più dei predittori; il background musicale non è significativo; [Sauvé & Pearce 2019](https://doi.org/10.1525/mp.2019.37.2.165) | Estratti brevi | AM |
| Tensione parametrica | Pendenze di feature in finestre mobili | Esp. 1, N=2.661; esp. 2, N=35 (2 scartati), 10 stimoli. r=.67–.99 fra modello e curva media; r medio fra soggetti .47; [Farbood 2012](https://doi.org/10.1525/mp.2012.29.4.387) | Finestre scelte sugli stessi dati; pochi gradi di libertà; aspettativa melodica esclusa dal modello | FT-parz |
| Predictive information rate | Quanto il presente informa sul futuro, dato il passato | Analisi di brani di Glass che concorda largamente con un ascoltatore esperto; [Abdallah & Plumbley 2009](https://core.ac.uk/download/30696123.pdf) | Confronto con un solo esperto; agreement non quantificato per quanto ho potuto vedere | AM |
| Self-similarity, novelty di Foote | Kernel a scacchiera lungo la diagonale della matrice di similarità | Non ho trovato effetti su giudizi di sviluppo | Kernel piccoli rumorosi; rileva confini, non direzione; [Müller, FMP](https://audiolabs-erlangen.de/resources/MIR/FMP/C4/C4S4_NoveltySegmentation.html) | FT\*; Foote 2000 AM |
| Compressione | Lunghezza di descrizione | Correlazione fra fattore di compressione e punteggio di analisi; [Louboutin & Meredith 2016](https://doi.org/10.1080/09298215.2015.1133656) | Criterio: compiti di analisi e classificazione, non ascoltatori | AM |
| Importanza dei pattern ripetuti | Compattezza, rapporto di compressione, occorrenze attese | Predicono rating di studenti di musica (Collins et al. 2011, in [Collins et al. 2016](https://doi.org/10.1017/S0890060414000687)) | Importanza, non sviluppo | SEC |
| Attributi di stile | Regressione stepwise | R²=.83 con 7 predittori su 32 stimoli; Collins et al. 2016 | Gli autori avvertono che le variabili metriche possono separare altri periodi, non i sistemi | FT-parz |

- Modelli lineari e quadratici della tensione sono risultati scarsi fuori campione, nonostante l'ottimo adattamento in addestramento: il modello quadratico andava in overfitting — Farbood 2012 [FT-parz]. Il modello temporale funziona meglio; l'assenza di finestra di memoria peggiora nettamente l'esito — Farbood 2012 [FT-parz].
- In isolamento dinamica e altezza pesano di più; i musicisti rispondono più forte su tutti i parametri tranne l'altezza — Farbood 2012 [FT-parz].
- Il predictive information rate è proposto come spiegazione possibile della U rovesciata fra entropia e piacere — Abdallah & Plumbley 2009 [AM, estratto].
- La sorpresa dipende dal contesto di incertezza — Cheung et al. 2019 [AM]; in Gold et al. la preferenza per alta entropia è emersa solo a basso IC (p=.001) [FT\*].
- Le ripetizioni riducono il gradimento (β=−0,003), ma l'interazione fra IC e ripetizione non è significativa (p=.38) — Gold et al. 2019 [FT\*].
- Una rassegna del 2025 afferma: «specific metrics to measure these higher-level features have not yet been proposed», riferendosi a forma e ripetizione — [Lerch et al. 2025](https://doi.org/10.1145/3769106) [FT-parz].

### Inferences
- **Limite formale:** ogni statistica funzione del solo insieme delle frasi (entropia globale, n-gram interni alla frase, numero di gruppi, distanza media a coppie) resta invariata permutando le frasi. Non può quindi distinguere sviluppo e materiale rimescolato. Un generatore che alza queste misure senza intenzione musicale è esattamente la varietà fittizia.
- **Derivazione mia, non citata:** il predictive information rate è I(Xₜ; futuro | passato). Per sequenze i.i.d. è zero, perché il presente non informa il futuro. Per sequenze periodiche già apprese è zero, perché H(Xₜ | passato)=0. È quindi l'unica grandezza di questa tabella che vale zero sia per il loop sia per il rumore, e per questo merita una prova su fixture avverse. Le stime con pochi dati saranno però instabili.
- Le percentuali di varianza non sono confrontabili: differiscono per stimoli, compito e livello di analisi. Non ho verificato a quale livello Gold et al. calcolino la varianza. Sono risultati su monodie, accordi o Western art music, non su musica elettronica a quattro ruoli.
- Una U rovesciata su IC non autorizza un IC-bersaglio: fornirebbe proprio un obiettivo da «giocare» (§4), e l'effetto è assente in molti studi.

### Gaps
- Non ho trovato studi che correlino direttamente una misura simbolica con un giudizio di «sviluppo» o «sviluppo autentico».
- Non ho letto Foote 2000 né Abdallah & Plumbley 2009 in testo integrale; Cilibrasi et al. e Dubnov non sono stati verificati.
- Non verificati: Egermann et al. 2013 (IC e risposte fisiologiche dal vivo) e i dati numerici di Cheung et al. 2019.

## 3. Come viene trattata autenticità o intenzionalità nella creatività computazionale?

### Takeaway
Il campo rifiuta la «mera generazione» ma non l'ha mai definita con precisione: il criterio utilizzabile è l'intenzionalità di novità e valore, verificabile dal percorso che ha prodotto l'output. Per CYMATICA conta dove sta la conoscenza: una decisione presa durante la generazione si può spiegare, un filtro a posteriori solo in parte.

### Cited Findings
- Ventura sostiene che la «mera generazione» non è mai stata definita adeguatamente; ordina i sistemi in randomizzazione, plagio, memorizzazione, generalizzazione, filtrazione, inception e creazione — [Ventura 2016](http://www.computationalcreativity.net/iccc2016/wp-content/uploads/2016/01/Mere-Generation.pdf) [FT-parz].
- In un sistema stocastico la novità è quasi certa ma non voluta: «this novelty is not intentional»; il valore è quasi certamente assente — Ventura 2016 [FT-parz].
- La filtrazione porta la prima evidenza di autovalutazione, tramite una funzione di fitness — Ventura 2016 [FT-parz].
- Soglie candidate di Ventura, in ordine crescente: possedere conoscenza; conoscenza in parte strutturata dal sistema; ragionevole probabilità di novità e valore; uno dei due intenzionale; entrambi intenzionali. L'autore chiede una soglia inclusiva e conclude che il campo l'ha ormai superata — Ventura 2016 [FT-parz].
- Generate-and-test contro generazione guidata: la conoscenza nel filtro spiega perché un artefatto è nuovo e valido, ma non come è nato; la conoscenza nel processo spiega anche la ragione della generazione — Ventura 2016 [FT-parz].
- FACE descrive atti creativi come tuple di atti generativi: espressioni (E), concetti (C), misure estetiche (A), framing (F), ciascuno a livello g (generare l'oggetto) o p (generare il metodo). IDEA descrive l'impatto con benessere e sforzo cognitivo del pubblico. La teoria è limitata al software e ipotizza una soglia di distanza sotto cui un atto è troppo simile al noto — [Colton, Charnley & Pease 2011](https://computationalcreativity.net/iccc2011/proceedings/the_foundational/colton_1_iccc11.pdf) [FT-parz].
- In IDEA il pubblico dà due valutazioni: benessere (−1…+1) e sforzo cognitivo (0…1), cioè il tempo che è disposto a spendere per capire l'atto. Gli autori evitano così la circolarità di chiedere direttamente quanto sia creativo — Colton et al. 2011 [FT-parz].
- Il framing è testo comprensibile che riferisce gli atti generativi e aggiunge valore, per esempio descrivendo i processi — Colton et al. 2011 [FT-parz]. Motivazione, intenzione e processo ne fanno parte; il software può conservare una traccia precisa del processo; gli autori ammettono però storie di framing non fattualmente accurate — [Charnley, Pease & Colton 2012](https://kt.ijs.si/senja_pollak/CC_resources/corpus/Proc_folders/2012/2012_12.txt) [FT\*].
- Boden distingue creatività psicologica e storica, e tre forme: combinatoria, esplorativa, trasformazionale, quest'ultima come alterazione di un vincolo costitutivo dello spazio — [Boden 2009](https://www.mit.bme.hu/eng/system/files/oktatas/targyak/8866/computer_models_of_creativity.pdf) [AM, estratti di ricerca; la pagina non si è aperta].
- Collins et al. riprendono da Pearce & Wiggins 2007 i rating di successo stilistico — [Collins et al. 2016](https://doi.org/10.1017/S0890060414000687) [FT-parz], quindi Pearce & Wiggins [SEC]. Un articolo ISMIR riporta che confrontarono con giudici tre varianti di modelli di Markov sulle melodie di corale — [ISMIR 2013](https://archives.ismir.net/ismir2013/paper/000228.pdf) [AM, estratto di ricerca], quindi anch'esso [SEC].
- In Autocousmatic i compositori criticano «problems of structure». Gli autori spiegano che agenti critici confrontano il materiale a esempi e non a un'intenzionalità del sistema, per cui il contesto delle decisioni si perde — [Eigenfeldt et al. 2016](http://www.computationalcreativity.net/iccc2016/wp-content/uploads/2016/01/Flexible-Generation-of-Musical-Form.pdf) [FT-parz].

### Inferences
- «Intenzionalità» diventa controllabile se riformulata così: per ogni variazione esiste una ragione registrata **prima** dell'esito e ricavata dal percorso decisionale reale, e rimuovere la decisione cambia qualcosa di rilevabile. È una proposta di traduzione, non una tesi di questi autori.
- La traccia decisionale prevista dal progetto è un framing a basso costo. A differenza di Charnley et al., per CYMATICA deve essere **vera**: un racconto generato a posteriori sarebbe varietà fittizia a livello verbale.
- Punteggiare candidati a valle di una generazione casuale corrisponde alla filtrazione di Ventura. Autocousmatic indica il rischio: un punteggio a posteriori non sostituisce un piano. Il piano gerarchico delle ricerche M2.4 corrisponde invece alla conoscenza in situ.
- La domanda «quanto tempo dedicheresti ancora a questo brano?» è un possibile analogo dello sforzo cognitivo di IDEA: evita di chiedere agli ascoltatori se il brano sia sviluppato o creativo.
- Le trasformazioni interne a un vocabolario fisso sono esplorazione nel senso di Boden. Introdurre o abbandonare una regola di forma sarebbe trasformazionale: è un obiettivo legittimo ma non richiesto a questo stadio.

### Gaps
- Non ho letto Boden 1990/2004, Wiggins 2006, né Pearce & Wiggins 2001/2007 in testo integrale.
- Non ho trovato criteri condivisi per stabilire quando un sistema generativo sia «autentico»: il termine non risulta operativo nei testi letti.

## 4. Quali metodi di valutazione sono raccomandati e come ci si difende dal gaming delle metriche?

### Takeaway
La raccomandazione convergente è usare criteri multipli e non aggregati, con metriche esterne al sistema che le ottimizza. Il rischio centrale è confondere una misura con il suo costrutto. Test di Turing e CAT classico non sono adatti a CYMATICA, il primo per ciò che misura e il secondo per il numero di giudici.

### Cited Findings
**Metriche**
- Yang & Lerch propongono un metodo formativo: feature di altezza e ritmo, calcolate su tutta la sequenza e per battuta, distanze intra-set e inter-set, stima di densità, KLD e area di sovrapposizione — [Yang & Lerch 2020](https://doi.org/10.1007/s00521-018-3849-7), [postprint](https://musicinformatics.gatech.edu/wp-content_nondefault/uploads/2018/11/postprint.pdf) [FT-parz].
- Il metodo non mira a valutare creatività né a modellare la percezione estetica, e richiede melodie monofoniche con numero fisso di battute — Yang & Lerch 2020 [FT-parz].
- Le metriche ad hoc rischiano di valutare un solo aspetto o di «evaluating with a metric that is part of the system design» — Yang & Lerch 2020 [FT-parz].
- KLD e area di sovrapposizione possono ciascuna ingannare in casi diversi, quindi vanno lette insieme — Yang & Lerch 2020 [FT-parz].
- Usare caratteristiche dell'output come proxy di una valutazione complessiva è discutibile, soprattutto se la proprietà è stata obiettivo di addestramento; il testo cita la legge di Goodhart — [Lerch et al. 2025](https://doi.org/10.1145/3769106), [arXiv](https://arxiv.org/pdf/2506.05104v1) [FT-parz].
- La somiglianza di distribuzioni non basta: copiare il training set darebbe punteggi perfetti, premiando conformità sulla novità — Lerch et al. 2025 [FT-parz]. Theis et al. notano che il buon esito su un criterio non implica buon esito sugli altri — [SEC, tramite Lerch].
- Anche per criteri specifici, le impressioni soggettive possono predire le risposte meglio delle misure oggettive — Lerch et al. 2025 [FT-parz].
- L'ottimizzazione di un proxy fallisce in almeno quattro modi, con rischio crescente all'aumentare della forza di ottimizzazione — [Manheim & Garrabrant 2018](https://arxiv.org/abs/1803.04585) [AM; i nomi dei meccanismi non erano nella pagina].
- Un «cavallo» sembra risolvere un compito senza farlo: alte prestazioni possono non significare ciò che sembrano — [Sturm 2016](https://arxiv.org/abs/1606.03044) [AM].

**Ascolto**
- Gli studi di ascolto spesso trascurano l'expertise dei soggetti e usano esempi proprietari; il campione nella tabella degli studi rassegnati va da 16 a 1.272 partecipanti — Yang & Lerch 2020 [FT-parz].
- Ariza conclude che questi test rischiano di sovrastimare la comprensione del soggetto — tramite Yang & Lerch 2020 [SEC; [articolo](https://muse.jhu.edu/article/266410) non letto].
- Il test di Turing regge solo in condizioni spesso assenti in esperimenti piccoli: riconoscere le norme di uno stile richiede esperienza; un esito non distingue imitatore bravo e giudice debole; porre la domanda uomo o macchina introduce un bias contro la musica AI — Lerch et al. 2025 [FT-parz].
- Non esistono pratiche standard per i test soggettivi; il survey chiede di definire operativamente i tratti da misurare — Lerch et al. 2025 [FT-parz]. Riporta anche prospettive in prima persona e autoetnografie come metodo legittimo per l'esperienza d'uso [FT-parz].
- Jordanous: solo un terzo di 75 articoli di creatività computazionale valutava la creatività, e valutazioni esterne al team erano rare — tramite Lerch et al. 2025 [SEC].

**CAT**
- Giudici esperti indipendenti, senza criteri imposti. Nei 21 studi di Amabile l'affidabilità è .72–.93; la media è di poco oltre 10 giudici (min 2, max 40) — [Baer & McKool 2014](https://www.rider.edu/sites/default/files/2024-05/GoldStandard.pdf) [FT-parz; Amabile SEC].
- Su 205 poesie, 10 esperti e 106 novizi: alpha riportato a 10 giudici .804 contro .575, correlazione fra le medie dei due gruppi solo .21 — [Kaufman et al. 2008](https://doi.org/10.1080/10400410802059929) [FT-parz].
- Collins et al. adattano il CAT a 16 ascoltatori di concerti e 16 esperti su 32 stimoli con scale 1–7: concordanza di Kendall W=.52 e .61; 15 esperti su 16 distinguono sopra il caso — Collins et al. 2016 [FT-parz].

**Ablazione**
- Il termine viene dalle neuroscienze: danneggiare parti di un sistema per capirne il contributo; Meyes et al. lo applicano a reti neurali — [Meyes et al. 2019](https://arxiv.org/abs/1901.08644) [AM].
- Il protocollo interno prevede già confronti incrementali A→B→C→D, a parità di seed, durata e renderer — [M2_4_Evaluation_Protocol.md](../../../../docs/research/M2_4_Evaluation_Protocol.md). Sturm et al. 2019 presentano invece una valutazione con musicisti e concerto pubblico — [Sturm et al. 2019](https://doi.org/10.1080/09298215.2018.1515233) [AM].

### Inferences
Difese proposte per CYMATICA, in ordine di costo crescente:
1. **Separazione obiettivo-evidenza.** Una misura usata dal planner nello scoring non vale più come prova. Se serve per guidare il planner, si dichiara e la si ritira dalle evidenze.
2. **Nullo per permutazione.** Una misura di sviluppo è ammessa solo se cambia, oltre la distribuzione nulla, rimescolando le frasi.
3. **Fixture avverse, prima di qualsiasi uso.** Rumore i.i.d., loop letterale, sviluppo reale rimescolato, trasposizione casuale per frase, ornamenti casuali, più un controllo positivo scritto a mano. Una metrica che non li ordina correttamente è un «cavallo».
4. **Verità sigillata.** Il pacchetto di valutazione custodisce lignaggio e trasformazioni; un analizzatore cieco deve ricostruirli.
5. **Nessun punteggio aggregato**, in linea con il protocollo esistente.
6. **Ipotesi e soglie fissate prima dell'ascolto**, con esiti negativi registrati.
7. **Ablazione** come prova finale, poiché un componente che non cambia nulla di rilevabile non può vantare intenzionalità.

Il CAT classico non è applicabile: con due persone di formazione diversa non esiste consenso. Le scale possono essere riusate come descrittori. Kaufman et al. suggeriscono inoltre di non sommare esperto e non musicista.

### Gaps
- **Non ho trovato studi di ablazione su sistemi generativi musicali** con questo nome; una ricerca testuale non trova il termine né nel survey di Lerch né in Yang & Lerch. Il protocollo sarebbe un adattamento.
- Non ho verificato l'attribuzione dell'origine del termine a Newell 1975 né i nomi dei meccanismi di Goodhart.
- Yang & Lerch riguarda dati e output addestrati, spesso con corpus di riferimento: CYMATICA ne è priva, quindi il confronto fra set va ripensato.
- Mancano resoconti su gaming di metriche specificamente musicali in sistemi non neurali.

## 5. Come rendere la variazione guidata dallo stato e non casuale, mantenendo il determinismo?

### Takeaway
Sotto il nome di «budget di novità» non ho trovato una tecnica musicale consolidata. Esistono però ingredienti verificati: vincoli rigidi di forma, profili di tensione, memoria come vincolo e modelli di forma top-down e bottom-up. La memoria è una funzione pura del piano accettato, quindi è compatibile con il determinismo.

### Cited Findings
- MorpheuS fissa ritmo e dinamica di un brano modello e cerca nuove altezze con variable neighborhood search, imponendo i pattern come vincoli rigidi e seguendo un profilo di tensione — [Herremans & Chew](https://arxiv.org/pdf/1812.04832) [FT-parz].
- Pattern brevi e frequenti vincolano troppo e riconducono al brano originale, mentre pattern rari perdono la struttura lunga — Herremans & Chew [FT-parz].
- La valutazione d'ascolto dei concerti di MorpheuS è aneddotica, da ascoltatori esperti — Herremans & Chew [FT-parz].
- Eigenfeldt et al. distinguono forma architetturale top-down e autorganizzazione bottom-up, e propongono agenti che negoziano forme con punti di controllo trattati come obiettivi, modificabili in tempo reale. È un position paper — Eigenfeldt et al. 2016 [FT-parz].
- Anche la forma momento, non teleologica, non può essere arbitraria pur dovendolo sembrare — Eigenfeldt et al. 2016, citando Kramer [FT-parz; Kramer SEC].
- Racchmaninof eredita la forma da un modello con pattern ripetuti e vincoli — Collins et al. 2016 [FT-parz]. Una versione successiva genera struttura ripetitiva e fraseologica lunga con metodo markoviano — [Collins & Laney 2017](https://doi.org/10.5920/JCMS.2017.02) [AM].
- MaxOrder impone con un automa che nessuna sottosequenza del corpus più lunga di un limite venga copiata, con arc consistency in O(n·T) — [Papadopoulos, Roy & Pachet 2014](https://doi.org/10.1609/aaai.v28i1.9126) [AM]; versione con k-gram proibiti e campionamento — [Papadopoulos, Pachet & Roy 2016](https://www.francoispachet.fr/wp-content/uploads/2021/01/max_order.pdf) [FT-parz, solo introduzione].
- System & Contrast descrive un segmento come sistema di elementi legati più un contrasto, di solito l'ultimo, che devia dalla sua logica; è applicato a 380 canzoni pop — [Bimbot et al. 2016](https://hal-univ-tlse3.archives-ouvertes.fr/INRIA/hal-01188244) [AM].
- Saunders & Gero modellano agenti che cercano novità, in linea con Martindale — [Saunders & Gero](https://robsaunders.netlify.app/publication/the-digital-clockwork-muse-a-computational-model-of-aesthetic-evolution) [AM; funzione edonica non verificata]; Martindale [SEC].
- Novelty search abbandona l'obiettivo e cerca novità comportamentale — [Lehman & Stanley 2011](https://gpbib.pmacs.upenn.edu/gp-html/Lehman_2011_EC.html) [AM; l'archivio non è confermato dalle fonti lette].
- In Wwise un contenitore Shuffle con «Avoid repeating last 10» su 15 varianti è una memoria di recenza industriale; serializzarne lo stato è una domanda aperta sul forum — [forum Audiokinetic](https://www.audiokinetic.com/qa/3617/routing-puzzle-impossible-to-solve), [salvataggio stato](https://www.audiokinetic.com/qa/1388/saving-state-of-random-container) [AM, solo discussioni della community].
- Nel repository: i 64 seed danno un solo contenuto perché il seed agisce solo sui pareggi, e se ne misurano 0 (Teorema D) — [m2_procedural_evidence.md](../../../../docs/m2_procedural_evidence.md). AGENTS.md vieta di dichiarare che il seed da solo riproduca una run adattiva, e richiede decisioni riproducibili da seed, versione di policy e traccia.

### Inferences
Schema proposto, deterministico per costruzione:
- **Stato:** registro dei motivi (occorrenze, ultima frase, trasformazioni usate, profondità del lignaggio), carico per ruolo, posizione nella forma, novità introdotta di recente.
- **Decisione:** funzione pura di stato, piano e flusso casuale indicizzato per dominio (`seed, sezione, frase, decisione`). Il runtime consuma decisioni accettate, come già richiesto per il replay.
- **Budget di novità:** quante dimensioni portatrici di forma possono cambiare insieme in una frase. Raggiunto il limite, le altre restano invarianti. È l'estensione del budget di sorpresa già proposto in M2.4.
- **Debito di contrasto:** cresce con le ripetizioni e si estingue con un contrasto, ricollegandosi a System & Contrast. Non è un timer casuale.
- **Consumo obbligato della novità:** un elemento nuovo crea un obbligo di riuso entro N frasi. È il punto che separa sviluppo e ornamento.

Il seed deve quindi agire **sopra i pareggi**: scegliere motivo, percorso di trasformazioni e forma fra alternative autoriali. Lo stato decide *quando*; il seed sceglie *quale percorso*. Se la tabella delle scelte è autoriale, la diversità fra seed non è fittizia per costruzione.

Anti-ripetizione di tipo Wwise o MaxOrder evita il difetto ma non costruisce sviluppo: un sistema può non ripetersi mai e non andare da nessuna parte. Vanno usate come vincoli, non come motori. Il rischio di MorpheuS è speculare: troppi vincoli riconducono al modello, per cui la granularità dei vincoli è una scelta da misurare.

Il debito di contrasto trova un riscontro qualitativo nell'esperto, che in due ascolti indica lo stesso punto, «dopo il secondo loop». Non ne deriva la posizione del contrasto: la lunghezza del «loop» non è definita nei registri.

### Gaps
- Non ho trovato letteratura che usi l'espressione «novelty budget» in musica, né penalità di recenza valutate percettivamente.
- Non ho verificato se MorpheuS o Racchmaninof siano deterministici da seed. MaxOrder è letto solo nell'introduzione.
- Le grammatiche di forma (Quick, Holtzman, Steedman) non sono state esaminate.
- Non verificati: scelte operative di giochi commerciali a musica procedurale.

## 6. Quali campioni e disegni sono realistici con due ascoltatori, uno esperto e uno non musicista? Che cosa possono onestamente concludere?

### Takeaway
Due persone non permettono inferenze di popolazione, ma permettono esperimenti su singolo soggetto, se ogni persona produce molte prove controllate e se le clip nuove replicano l'effetto. Il controllo più prezioso non è il numero di ascoltatori ma le prove fittizie.

### Cited Findings
- I disegni small-N sono forti con teoria esplicita, misure precise e molte prove per persona: l'individuo è l'unità di replica. La simulazione usa N=4 e 400 prove per condizione; l'analisi di gruppo raggiunge potenza simile solo a N=128. Non servono a stimare parametri di popolazione — [Smith & Little 2018](https://pmc.ncbi.nlm.nih.gov/articles/PMC6267527/) [FT\*, primi 100.000 caratteri].
- Gli ascoltatori non formati mostrano capacità sofisticate per esposizione, inclusi rapporto tema-variazioni, tensione e integrazione in larga scala — Bigand & Poulin-Charronnat 2006 [FT-parz].
- Nella tensione i due gruppi rispondono in modo simile, con accordo più alto fra musicisti e maggiore sensibilità di questi ai parametri — Farbood 2012 [FT-parz, che cita studi precedenti].
- Nei disegni a trattamenti alternati, un test di randomizzazione è applicabile solo se la sequenza delle condizioni è stata davvero randomizzata — [Manolov](https://www.uv.es/revispsi/articulos1.09/7MANOLOV.pdf) [AM, estratto di ricerca; standard di Kratochwill non verificati].
- Registro interno: stesso esperto, stesso PCM il 2026-10-28 e il 2026-11-03. L'esperto ha rilevato da solo l'identità del PCM e ha indicato lo stesso punto per il bisogno di variazione. Le clip di rilascio da 7–8 s sono risultate «uguale a prima»; il registro precisa che non è un ABX né una prova di inaudibilità — [ascolto del 2026-11-03](../../../../docs/m2_4_human_listening_2026-11-03.md).

**Calcoli miei** (binomiale esatta, unilaterale, α=.05; script `C:\Users\dendo\AppData\Local\Temp\claude\20bb74b6ce2af0d8\b9117a47-49dd-4a71-8af3-3eeb5bbb3cc7\scratchpad\n2_power.py`):

| Prove | Corrette minime | α effettivo | Potenza con Pc=.70 | Pc=.75 | Pc=.80 | Pc=.90 |
|---|---|---|---|---|---|---|
| 8 | 7 | .035 | .26 | .37 | .50 | .81 |
| 12 | 10 | .019 | .25 | .39 | .56 | .89 |
| 16 | 12 | .038 | .45 | .63 | .80 | .98 |
| 20 | 15 | .021 | .42 | .62 | .80 | .99 |
| 24 | 17 | .032 | .57 | .77 | .91 | >.99 |
| 30 | 20 | .049 | .73 | .89 | .97 | >.99 |

- Per 80% di potenza servono circa 76, 42, 28, 18 o 11 prove con Pc pari a .65, .70, .75, .80, .90.
- Test del segno su K coppie di clip: p bilaterale minimo 2·0,5ᴷ. Servono **almeno 6 clip tutte concordi** per p<.05 (6/6 dà .031).
- Dopo 9/12 corrette, con prior uniforme, l'intervallo credibile al 95% per Pc è .46–.91: il dato è compatibile col caso. Con 13/16 è .57–.93.

### Inferences
**Disegno raccomandato**
- Le **clip**, non gli ascoltatori, sono l'unità replicabile: gli stimoli sono economici, le persone no. Ogni nuova clip è una replica dello stesso contrasto. Prima serve però contenuto diverso per seed: oggi esiste un solo stimolo, e ogni risultato vale per quel brano.
- **Ogni persona si analizza da sola**, senza sommare i due giudizi. L'accordo fra i due è un dato descrittivo, non una replica indipendente del costrutto.
- **Prove fittizie obbligatorie:** coppie A/A identiche per stimare i falsi positivi e duplicati nascosti per l'affidabilità test-retest. Senza queste, una preferenza non si distingue dall'aspettativa di miglioramento.
- **Ordine controbilanciato e chiave sigillata:** l'ascolto del 2026-11-03 non era alla cieca e la chiave era nota a chi conduceva.
- **Il test rimescolato non si può ripetere sulla stessa clip:** l'ascolto ripetuto sposta l'attenzione su unità più grandi (Margulis), quindi il secondo ascolto non è lo stesso test.
- **Budget realistico:** due sessioni da 30–40 minuti. Clip brevi da 8–16 battute per 16–24 prove di rilevazione; 6 coppie di clip lunghe per i giudizi di forma (6 × 2 × 3 minuti = 36 minuti). Lalitte & Bigand hanno lavorato con 12 brani di 3 minuti in circa 50 minuti.

**Conclusioni oneste**

| Si può dire | Non si può dire |
|---|---|
| Questa persona distingue queste due versioni in queste condizioni | La differenza è udibile in generale |
| Questa persona preferisce B ad A su K clip, con questo tasso di coerenza | B è musicalmente migliore |
| L'esperto nomina senza imbeccate la relazione custodita nella verità sigillata | Il brano ha sviluppo autentico per il pubblico |
| Il componente rimosso produce una differenza rilevata su K clip | Il componente causa gradimento nei giocatori |
| Con questo numero di prove, un effetto di dimensione X non è escluso | L'assenza di effetto è provata |

**Inferenza asimmetrica.** Un esito positivo è informativo. Un esito nullo è debole, soprattutto per la forma: Lalitte & Bigand mostrano che anche materiale autentico viene distinto poco dal rimescolato, e con 12 prove una persona con Pc=.75 fallisce la soglia circa 6 volte su 10.

**Divisione dei ruoli.** Il non musicista è un giudice valido per contrasto, segmentazione, ritorno riconosciuto e noia. L'esperto aggiunge diagnosi armonica e vocabolario. Kaufman et al. sconsigliano di trattare uno come sostituto dell'altro, anche se riguarda poesie e non musica.

### Gaps
- Gli standard per disegni a trattamenti alternati (repliche minime) non sono stati verificati: ho trovato solo una cifra non attribuibile, che non uso.
- Non ho trovato studi sulla stabilità test-retest di giudizi estetici di un singolo esperto su musica generativa.
- I calcoli presumono prove indipendenti. Fatica e apprendimento violano l'assunto, quindi le potenze sono ottimistiche.
- L'ascolto del 2026-10-28 e quello del 2026-11-03 non usano le stesse scale: il protocollo chiede di riusare le scale 1–7.

## 7. Obiettivo: definizioni operative, misure simboliche e procedure d'ascolto che separano sviluppo reale e rumore

### Takeaway
Proposta di lavoro: la variazione è **sviluppo** se è derivata, cumulativa, dipendente dall'ordine e consumata in seguito, ed è **autentica** se tutto ciò resta rilevabile da un ascoltatore che non conosce la verità sigillata. È **fittizia** se sparisce sotto permutazione o ablazione, oppure se cambia solo la metrica. Nessuna soglia numerica è fissata: vanno calibrate su fixture avverse e controllo positivo.

### Cited Findings
Le basi empiriche sono nei §§1–6. Gli elementi direttamente ripresi sono:
- la permutazione per chunk e il compito d'ascolto da [Lalitte & Bigand 2006](https://lead.ube.fr/wp-content/uploads/2023/09/000296-music-in-the-moment-revisiting-the-effect-of-large-scale-structures.pdf) [FT-parz];
- la classificazione libera con descrizione da [McAdams et al. 2004](https://mcgill.ca/mpcl/files/mpcl/mcadams_2004_musperc_1.pdf) [FT-parz];
- la segmentazione a tocco da [Phillips et al. 2020](https://doi.org/10.3389/fpsyg.2020.01001) [FT-parz];
- la matrice di similarità e il nucleo a scacchiera da [Müller, FMP](https://audiolabs-erlangen.de/resources/MIR/FMP/C4/C4S4_NoveltySegmentation.html) [FT\*];
- IC, entropia e modello a breve termine da [Pearce 2018](https://pmc.ncbi.nlm.nih.gov/articles/PMC6849749/) [FT\*];
- gli strumenti esistenti sono descritti in [progress.md](../../../../docs/progress.md): fingerprint per frase, `phraseGroupIds`, M1–M4, `presence_stats_v1`, `evaluation_pack_v3`, ablazione rimandata.

### Inferences
**Definizioni operative proposte**

| Criterio del titolare | Definizione | Prova simbolica | Prova d'ascolto |
|---|---|---|---|
| **Ricorda** | Una frase ha un genitore precedente, una trasformazione nominata e almeno un invariante in una dimensione udibile | Recupero cieco del genitore dagli eventi, contro controllo rimescolato | Riconoscimento vecchio/nuovo con esche; classificazione libera |
| **Sviluppa** | La trasformazione parte dallo stadio precedente, non sempre dall'originale; l'ordine conta | Profondità del lignaggio; pendenza del variogramma oltre il nullo | Originale contro rimescolato: «va da qualche parte?» |
| **Sorprende** | Evento improbabile dato un contesto stabile, poi ripreso | Sorpresa con contesto; riduzione di IC alla ripresa | Punti di sorpresa segnalati senza imbeccate |
| **Ritorna** | Riapparizione riconoscibile dopo un allontanamento reale | Picco di similarità a lag lungo dopo una valle | Tocco al ritorno, con confini sigillati |
| **Varietà fittizia** | Differenza senza conseguenza, indipendente dall'ordine, o solo metrica | Nessun riuso; stessa misura dopo permutazione | ABX al caso, oppure rilevata ma non motivabile |

**Misure simboliche proposte** (tutte su eventi di piano, per ruolo e per punto di vista: altezze relative, contorno, griglia di onset, durate, registro, densità, timbro; riportate come vettori, mai come punteggio unico):

1. **Variogramma fraseologico.** V(k) = media di d(Pₜ, Pₜ₊ₖ), con la Levenshtein normalizzata già esistente (M4). Il rumore i.i.d. dà V piatto da k=1. Il loop dà zeri periodici. Lo sviluppo dà V crescente con k entro la sezione. Il ritorno dà una caduta al lag del ritorno. È la media delle superdiagonali della matrice di similarità, quindi riusa quel lavoro.
2. **Test di permutazione.** Ogni statistica si confronta con la distribuzione ottenuta rimescolando le frasi, a parità di contenuto.
3. **Informazione d'ordine.** Differenza fra compressione del brano rimescolato e dell'originale, per punto di vista.
4. **Consequenzialità.** Quota di n-gram nuovi che ricompare, anche trasformata, entro W frasi, separata dal volume di novità introdotta. Il rumore ha molta novità e bassa consequenzialità; il loop ha novità nulla.
5. **Apprendibilità.** Differenza di IC, dal modello a breve termine, fra prima e seconda occorrenza di un elemento nuovo. Un modello a variabile ordine deterministico, a stato finito, è sufficiente offline.
6. **Sorpresa con contesto.** Quota di eventi ad alto IC in contesti a bassa entropia, contro quelli in contesto già incerto (ipotesi derivata da Cheung et al.; non validata).
7. **Curva del vocabolario.** Crescita cumulativa di n-gram distinti: lineare per il rumore, satura per il loop, a gradini con assestamento per lo sviluppo.
8. **Forza del ritorno.** Similarità con il materiale precedente meno similarità delle frasi interposte, con vincolo di lag minimo.
9. **Lignaggio** (dalla verità sigillata): profondità massima, quota di frasi a profondità ≥2, quota di trasformazioni riprese.
10. **Novità per frase e ruolo** (matrice di similarità multi-ruolo): quanti ruoli e dimensioni cambiano insieme, per vedere il budget e il rischio «tutto nuovo insieme».

Le statistiche su 24 frasi hanno pochi dati: entro una sezione la permutazione ha potenza modesta. Le stime vanno lette come diagnosi finché non esistono più seed effettivi. Per quanto ho letto in `progress.md`, M1–M4 confrontano seed fra loro e non misurano l'ordine interno a una run; va verificato sul codice.

**Validazione delle misure, prima di usarle**

| Fixture | Atteso | Che cosa smaschera |
|---|---|---|
| Loop letterale | Nessuna novità, V periodico | Metriche che premiano la sola coerenza |
| Rumore casuale nei vincoli di scala | Molta novità, V piatto, consequenzialità bassa | Metriche che premiano la diversità |
| Sviluppo reale rimescolato | Stesso contenuto, nessuna direzione | Metriche cieche all'ordine |
| Trasposizione casuale per frase | Distanza alta sulle altezze, bassa sugli intervalli | Rappresentazioni assolute |
| Ornamenti casuali | Novità superficiale, lignaggio piatto | Conteggi di n-gram |
| Controllo positivo scritto da una persona | Deve superare tutte le precedenti | Metriche senza validità di costrutto |

Il controllo positivo, composto dal titolare o dall'esperto sullo stesso motivo e reso con lo stesso renderer, è l'unico riferimento esterno disponibile. È anche un dato percettivo: se gli ascoltatori non lo distinguono dal rimescolato, il test è troppo debole.

**Batteria d'ascolto proposta**

| Test | Domanda | Procedura | Analisi | Limite |
|---|---|---|---|---|
| B0 Controlli | Quanto è rumoroso il giudizio? | Coppie identiche e duplicati in ogni sessione | Tasso di falsi positivi, accordo test-retest | Da riportare sempre |
| B1 Rilevazione | Il cambiamento si sente? | ABX su 8–16 battute, 16–24 prove | Binomiale per persona | Non dice nulla sul valore |
| B2 Ritorno | Il materiale viene ricordato? | Vecchio/nuovo su A, A′, esca dello stesso stile | Hit e falsi allarmi per persona | Richiede motivi diversi come esche |
| B3 Ordinamento libero | La parentela sentita coincide con il lignaggio? | Raggruppare 10–15 frammenti e descrivere | Confronto con la verità sigillata; analisi delle parole | Il confronto immediato privilegia la superficie |
| B4 Segmentazione | Le sezioni sono udibili? | Tocco a ogni fine sezione e ritorno | Precisione e richiamo sui confini sigillati | Non prova la funzione del confine |
| B5 Ordine | L'ordine conta? | Originale contro rimescolato con transizioni controllate, primo e ultimo chunk fissi | Segno su K≥6 clip | Nullo poco informativo; serve materiale nuovo per ogni coppia |
| B6 Ablazione | Il componente serve? | Completo contro versione senza il componente, un componente alla volta | Prima ABX, poi preferenza e scale | Richiede prima B1 |
| B7 Attenzione divisa | Regge in gioco? | Ripetere il giudizio con un compito o in gioco | Confronto con ascolto attento | Stimoli diversi; rilevante per M7 |

La descrizione libera viene **prima** delle scale, per non suggerire i termini. Nel non musicista la domanda è sull'esperienza, non sulla teoria.

**Regola di interpretazione per un componente sotto ablazione**
1. **Inerte:** il fingerprint non cambia. La variazione è nulla a livello simbolico.
2. **Attivo, non rilevato:** il contenuto cambia, B1 non distingue. Da rivedere; non è prova di inaudibilità.
3. **Rilevato, non motivato:** si sente, ma non cambia ordine, ritorno o giudizio. Candidato alla varietà fittizia.
4. **Rilevato e motivato:** l'ascoltatore lo descrive e la descrizione concorda con la verità sigillata. È il massimo verdetto ottenibile con N=2, valido per quelle persone e quelle clip.

**Ordine di lavoro proposto:** (1) validare le misure sulle fixture avverse, attività automatica e reversibile; (2) dare al seed un'azione sopra i pareggi, altrimenti ablazione e repliche non hanno stimoli; (3) introdurre il primo intervento udibile, con ipotesi dichiarate; (4) eseguire B0, B1 e B4, che costano poco; (5) eseguire B5 e B6 sulla build candidata. Non è una decisione di milestone: P-M25-06 resta aperta.

**Limite complessivo.** Queste misure possono falsificare la varietà fittizia, ma non certificare il valore artistico. Superarle è necessario per sostenere che il sistema sviluppa, non sufficiente per sostenere che la musica sia bella, in accordo col principio del protocollo di non confondere quadri diversi.

### Gaps
- Nessuna soglia è proposta: fissarla ora sarebbe arbitrario. Va decisa dopo la calibrazione, prima dell'ascolto.
- Il variogramma, la consequenzialità, l'apprendibilità e la sorpresa con contesto sono proposte mie, non validate in letteratura né provate su dati CYMATICA.
- Non ho eseguito alcuna misura sul piano reale: questa ricerca non modifica il repository.
- Non è stabilito come rimescolare frasi in A–B–A′ senza creare discontinuità udibili di tempo e metro: il cambio di tempo fra sezioni limita il rimescolamento alle singole sezioni.
- Va chiarito se il controllo positivo debba restare dentro il linguaggio CYMATICA, e chi lo scrive senza conoscere le metriche.
