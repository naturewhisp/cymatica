# Regole d'insieme per una tessitura a quattro ruoli valutabili da un planner deterministico su dati simbolici

Convenzioni di queste note.

- **Livelli di verifica:** FT = testo integrale letto direttamente da me. FT parz. = testo integrale disponibile e letto direttamente da me, ma solo nelle sezioni citate (estratte con `pdftotext`). FT* = letto tramite estrattore automatico (WebFetch). AM = solo abstract, metadati o estratti del motore di ricerca. SEC = noto solo tramite fonte secondaria. Il registro completo, con ciò che non ho potuto leggere, è in fondo alla sesta domanda.
- **Tipo di regola:** H = vincolo duro (candidato scartato). S = preferenza morbida (termine di punteggio). O = controllo offline su audio, fuori dal planner.
- **Base di evidenza:** E = sperimentale. C = corpus. T = modello o teoria. P = pratica di mixaggio. A = scelta autoriale di CYMATICA.
- **Misure mie:** i calcoli etichettati «Misura mia» non sono risultati pubblicati. Sono stati eseguiti su `docs/research/experiments/cacp_methods/fixture_intents_seed1337.csv` (sola lettura, seed 1337) e su spettri teorici. Gli script stanno in `C:\Users\dendo\AppData\Local\Temp\claude\20bb74b6ce2af0d8\b9117a47-49dd-4a71-8af3-3eeb5bbb3cc7\scratchpad\calc`, fuori dal repository. Griglia: 16 passi per battuta, ciclica. A = 120 BPM, B = 140 BPM, A′ = 120 BPM, 32 battute ciascuna.
- **Stato:** tutte le regole sono **proposte**, non decisioni accettate. Ogni soglia è un valore iniziale da versionare e validare, non un valore validato.

## Riferimento metrico: quanta sincope funziona, cosa deve restare ancorato, e può ancorare un ruolo diverso dal kick?

### Takeaway

La sincope funziona in dose media, ma l'evidenza sperimentale principale la misura sopra un riferimento stabile: non esiste una dose trasferibile senza quel riferimento. L'àncora non deve essere il kick. Può stare in un altro ruolo, e nel piano attuale regge su Vettore e Corpo. Il modello a soli onset, però, mostra che il backbeat da solo non riporta la fase sul tempo forte: serve un'àncora dichiarata e, soprattutto, uno stato metrico esplicito (ancorato, spostato, ritorno).

### Cited Findings

**Dose di sincope**

- In Witek et al. 2014 (66 partecipanti, 50 drum break di due battute a 120 BPM, 16 s) le risposte seguono una U rovesciata: il modello quadratico spiega R² = .347 per il desiderio di muoversi e .427 per il piacere. Per il movimento vale Medio > Basso > Alto; per il piacere Medio > Basso e Alto, senza differenza fra Basso e Alto — [Witek et al. 2014, PLOS ONE](https://doi.org/10.1371/journal.pone.0094446) [FT*, letti 100 000 caratteri su 108 969].
- Nello stesso studio la sincope stava solo in kick e snare, mentre l'hi-hat teneva ottavi costanti. L'indice, ispirato a Longuet-Higgins & Lee con pesi per strumento, va da 0 a 81 — [Witek et al. 2014](https://doi.org/10.1371/journal.pone.0094446) [FT*]. Le soglie Basso/Medio/Alto sono nella Tabella S1, che non ho letto.
- La correzione del 2015 riguarda indicizzazione degli stimoli e trascrizioni e dichiara invariati gli esiti — [Correction](https://doi.org/10.1371/journal.pone.0139409) [FT*, via PMC4581707].
- Gli autori avvertono che ritmi tanto sincopati da rompere il metro dovrebbero suscitare meno movimento, e che una sincope alta può impedire l'entrainment; la conclusione sul livello medio resta prudente — [Witek et al. 2014](https://doi.org/10.1371/journal.pone.0094446) [FT*].
- Gli ascoltatori tendono a spostare la fase dell'impulso interno con pattern molto sincopati, riascoltandoli come meno sincopati — [Fitch & Rosenfeld 2007, scheda bibliografica](https://peeref.com/works/67049890) [AM; il PDF indicato ha rifiutato la connessione].

**Misure simboliche della sincope**

- Longuet-Higgins & Lee assumono che l'ascoltatore interpreti il ritmo minimizzando le sincopi. La sincope è la differenza di peso fra una pausa o nota legata su posizione forte e la nota sonora precedente — [Smith & Honing 2006](https://www.mcg.uva.nl/mcg-2023/papers/smith-honing-2006.pdf) [FT].
- La misura correla con giudizi di complessità su 35 ritmi (r = .75; variante con gerarchie di Palmer & Krumhansl per musicisti r = .73, per non musicisti r = .64), con outlier evidenti. Sommare le sincopi in un solo valore può essere semplicistico — [Smith & Honing 2006](https://www.mcg.uva.nl/mcg-2023/papers/smith-honing-2006.pdf) [FT]. Nota: la variabile validata è la complessità percepita, non il groove.
- Povel & Essens esprimono la controevidenza contro un clock come C = 4E0 + E1, dove E0 conta i tick che cadono su silenzio. Pattern con clock forte si riproducono meglio — estratto da ricerca su [Povel & Essens 1985](https://doi.org/10.2307/40285311) [SEC/AM; Springer ha reindirizzato al login, DOI riportato a memoria e non verificato]. Non ho verificato la definizione di E1 né le regole degli accenti: uso solo E0.
- Tan, Lustig & Temperley misurano la sincope su sillabe accentate di 80 canzoni rock (23 129 note): SQ a livello di ottavo .228 (2 382 su 10 433), a livello di sedicesimo .105, totale .333. Nelle canzoni inglesi dell'Ottocento del confronto SQ = 0 — [Tan et al. 2019](https://doi.org/10.1525/MP.2019.36.4.353) [FT parz.].
- La sincope rock è soprattutto anticipatoria: ASQ .772 all'ottavo e .740 al sedicesimo. La tendenza allo spostamento è 5→4 .554, 1→8 .504, 3→2 .439, 7→6 .351 (posizioni di ottavo 1–8) — [Tan et al. 2019](https://doi.org/10.1525/MP.2019.36.4.353) [FT parz.].
- L'unità di spostamento ottimale sarebbe, in modo molto approssimato, un quarto di secondo; la sincope al sedicesimo cala bruscamente sopra 120 BPM — [Tan et al. 2019](https://doi.org/10.1525/MP.2019.36.4.353) [FT parz.].

**Limiti temporali**

- London colloca il metro fra circa 100 ms e 6 s, con la percezione del beat fra 200 e 1800 ms e picco intorno a 600–700 ms — estratti su [London 2002](https://sonicfield.org/library/cognitive-constraints-on-metric-systems-some-observations-and-hypotheses) [SEC/AM]. *Hearing in Time* non è stato letto.

**Àncora fuori dal kick**

- Il metro può sostenersi internamente: in MEG la corteccia uditiva risponde alla frequenza del pulse anche in sequenze sincopate senza energia a quella frequenza, con aggancio di fase nei momenti in cui il pulse manca — [Tal et al. 2017](https://doi.org/10.1523/JNEUROSCI.2500-16.2017) [AM].
- Nella EDM Butler descrive la «dissonanza di spostamento»: un livello antimetrico sfasato rispetto a quello metrico. Il ruolo metrico può passare da un livello all'altro. In un esempio un piatto sui quarti conferma il tempo forte; in un altro un nuovo drumbeat sfasato di un ottavo può far sentire il beat ribaltato, in modo prima inconcludente e definitivo solo dopo un rinforzo. Un DJ può ottenere lo stesso effetto con un kick sui controtempi. L'ingresso di un livello può risolvere la dissonanza o crearla — [Butler 2001](https://www.mtosmt.org/issues/mto.01.7.6/mto.01.7.6.butler_essay.html) [FT*].
- Una fonte successiva attribuisce a Butler 2006 una «emancipazione» della dissonanza metrica nella EDM — [Duinker, Euromac 2017](https://creaa.unistra.fr/websites/gream/Activites/Euromac_2017_-_Postprint_-_Extended_abstract_-_DUINKER_Ben.pdf) [SEC/AM]. Il libro non è stato letto.
- I sistemi di beat tracking di Goto & Muraoka usano schemi preregistrati di kick e snare per dedurre tipo di beat e valore; il sistema precedente assumeva kick sui tempi forti e snare su quelli deboli. Senza batteria, i cambi d'accordo indicano mezzo tempo e battuta — [Goto & Muraoka 1995](https://staff.aist.go.jp/m.goto/PAPER/ICMC95/node1.html) [AM]. Sono modelli ingegneristici, non test percettivi.
- Con toni simultanei G3 e B♭4, un anticipo di 50 ms del tono grave produce una MMN più ampia di quello del tono acuto (F(1,16) = 16,56) e sposta di più il tapping (14 ms contro 11 ms) — [Hove et al. 2014](https://doi.org/10.1073/pnas.1402039111) [FT*, via PMC4104866]. L'effetto c'è ma è piccolo, e usa toni di pianoforte, non un kick.
- In un contesto polifonico la stabilità percepita dipende, con interazioni, da numero di flussi sincopati, strumentazione, posizione metrica e formazione musicale; le teorie monofoniche non bastano — [Witek et al. 2014, Music Perception](https://doi.org/10.1525/mp.2014.32.2.201) [AM]. Non ho potuto verificare il dettaglio fra kick e snare.
- GTTM ha regole di preferenza metrica che includono parallelismo, tempo forte precoce, evento, accento, lunghezza, basso, cadenza — [Generative theory of tonal music](https://en.wikipedia.org/wiki/Generative_theory_of_tonal_music) [SEC]. Le fonti secondarie numerano le regole in modo diverso: non cito numeri di regola.

**Riscontri del progetto**

- Il titolare e l'esperto trattano lo spostamento come espressivo se scelto; l'esperto propone snare sul 2 e sul 4 come riferimento e ghost notes come varietà — [ascolto del 2026-10-28, §3bis–§3ter](../../../../docs/m2_4_human_listening_2026-10-28.md) [FT].
- Dopo l'ascolto successivo F1 è passato a «da rivalutare»: il metro può reggersi su Vettore e Corpo, e alzare il kick renderà più evidente il controtempo — [proposta M2.5](../../../../docs/research/M2_5_Entry_Music_Quality_Proposal.md) [FT].

**Misura mia sul piano e su pattern di prova**

| Pattern (passi di 16) | LHL ciclico | Tick silenziosi E0 su clock di quarti, fase 0/1/2/3 |
|---|---:|---|
| Pulso A1 `2,5,7,10,13,15` | 11 | 4/2/2/2 |
| Pulso A2 `0,4,8,12` | 0 | 0/4/4/4 |
| Pulso B, A′ `2,6,10,14` | 7 | 4/4/0/4 |
| Esperto `dritto` `0,3,5,8,11,13` | 6 | 2/2/4/2 |
| Esperto `storto` `1,4,6,9,12,15` | 8 | 2/2/3/3 |
| Vettore `0,4,8,12` (tutte le sezioni) | 0 | 0/4/4/4 |
| `2,5,7,10,13,15` + backbeat `4,12` | 9 | 2/2/2/2 |
| `2,6,10,14` + backbeat `4,12` | 5 | 2/4/0/4 |
| `0,3,5,8,11,13` + backbeat `4,12` | 2 | 0/2/4/2 |
| `2,6,10,14` + kick su `0` | 4 | 3/4/0/4 |

- In B e A′ nessun onset di Pulso coincide con Vettore; in A il 50% degli onset di Vettore coincide con Pulso (tutti in A2).
- Vettore ha sincope zero: in tutte le sezioni gli onset sono sui quarti (LHL 0, SQ 0). Tutta la sincope sta in Pulso e Trama.

### Inferences

**Lettura dei dati**

- **`2,6,10,14` non è un pattern «storto» in sé.** Da solo è un four-on-the-floor sfasato di un ottavo, con clock perfetto in fase 2 (E0 = 0). Con Vettore sui quarti le fasi 0 e 2 pareggiano: a rompere la parità sono il cambio d'accordo sul passo 0 e il contorno di Vettore. Questo spiega perché il metro regge oggi e perché alzare il kick può ribaltarlo, come nell'esempio del DJ di Butler e coerentemente con Hove. È un'ipotesi da testare, non un risultato.
- **Il backbeat da solo non riporta la fase 0 nel modello a soli onset.** Con `2,6,10,14` + `4,12` la fase 2 resta perfetta. Nell'esempio dell'esperto il kick `storto` contiene già 4 e 12 e coincide con lo snare, mentre il generatore (`2,5,7,10,13,15`) non li contiene. La proposta §3ter non è quindi trasferibile senza prova. Se funziona, funziona per schema appreso e timbro distinto, che E0 non vede.
- **Inversione dei ruoli.** Nel rock analizzato la melodia sincopa e il beat esplicito resta fermo. In CYMATICA il lead è rigido e il kick sincopa. Nessuna fonte vieta questa disposizione, che in EDM è un dispositivo noto, ma va scelta. Oggi è un effetto collaterale della rotazione del candidato (F1).
- **Il dato di Witek non è una dose assoluta.** La curva a U rovesciata si riferisce a loop di 2 battute con hi-hat stabile. L'indice 0–81 non è confrontabile con LHL.

**Regole candidate**

| ID | Regola | Calcolo simbolico | Tipo | Base |
|---|---|---|---:|---|
| R-MET-1 | Stato metrico esplicito per frase: `ancorato`, `spostato`, `ritorno`. Lo spostamento deve essere dichiarato dal direttore, mai derivato dall'indice del candidato | Campo di frase; test: rotazione di Pulso ≠ 0 solo se lo stato non è `ancorato` | H | A, T |
| R-MET-2 | In stato `ancorato`, almeno un ruolo-àncora dichiarato articola il tempo 1 e la fase 0 è la migliore (o pari) sul clock di quarti, a parità risolta da R-MET-3 | Unione degli onset dei ruoli-àncora; E0 per fase; argmin | H | T, E debole |
| R-MET-3 | Punteggio di fase pesato: onset grave, cambio d'accordo, attacco di frase e backbeat dichiarato sommano evidenza per la fase 0 | Σ peso(ruolo, tipo) sui tick, per fase; margine fase 0 − migliore altra | S | T, E parziale |
| R-MET-4 | Banda di sincope per sezione, legata alla tensione: evitare sezioni lunghe a zero e al massimo | LHL per ruolo e dell'unione; finestra su frase; banda da calibrare con pilot | S | E (forma), non la soglia |
| R-MET-5 | Almeno un ruolo metricamente rigido per volta; numero di ruoli sincopati limitato | Conteggio dei ruoli con LHL > 0 nella frase | S (H con massimo 3 di 4 come valore iniziale) | E parziale, C |
| R-MET-6 | Ciclo di vita dello spostamento: esposizione del riferimento, spostamento, ritorno su confine ipermetrico | Macchina a stati; lunghezza minima di esposizione e massima di spostamento | S; H sul ritorno entro la sezione | A, T |
| R-MET-7 | Sincope preferibilmente anticipatoria: onset debole seguito da silenzio o nota tenuta sul tempo forte | ASQ del ruolo; bonus 5→4 e 1→8 | S | C, dipendente dal genere |
| R-MET-8 | Unità di spostamento vicina a 250 ms: ottavo a 120 BPM. Il sedicesimo solo per ruoli non dominanti | Durata dello spostamento dal tempo della sezione | S | C |
| R-MET-9 | Nessuna suddivisione di ruolo sotto 100 ms | IOI minimo; a 140 BPM il sedicesimo vale 107 ms | H | T |
| R-MET-10 | Àncora alternativa: voce `backbeat` di Pulso su 2 e 4 con timbro distinto, solo dopo prova rispetto al kick sul tempo 1 | Confronto A/B; nel planner è solo un pattern dichiarato | da decidere | P, testimonianza esperta |

Per R-MET-5, «massimo 3 di 4» è un valore iniziale mio, non derivato da una fonte. Per R-MET-6 l'unico indizio temporale è di seconda mano: Chapin et al. 2010 riportano attivazione motoria solo dopo tre ripetizioni — citati in [Fiorin & Delfitto 2024](https://doi.org/10.3389/fpsyg.2024.1304485) [FT*, letti 100 000 caratteri su 251 836; Chapin è SEC].

### Gaps

- Nessuna fonte letta dà una dose di sincope trasferibile a una tessitura sintetica a quattro ruoli.
- Nessuna prova sperimentale letta confronta kick sul tempo forte e backbeat come àncora con kick spostato. Le clip §3ter sono testimonianza di un solo esperto.
- Non ho letto il full text di Witek et al. 2014 su *Music Perception* né quello di Fitch & Rosenfeld: mancano i numeri che servirebbero per R-MET-3.
- London, *Hearing in Time*, e Lerdahl & Jackendoff non sono stati letti. Le loro regole sono riportate solo da fonti secondarie.
- Non esiste un indice validato che sommi in un unico valore sincopi di ruoli diversi: la LHL dell'unione è un'approssimazione.

## Armonia verticale: consonanza sui tempi forti, note estranee, melodia e cambi d'accordo

### Takeaway

La regola utile non è «niente dissonanze». È che una dissonanza del ruolo dominante abbia un motivo leggibile: posizione metrica, preparazione, risoluzione per grado o una stratificazione dichiarata. Il problema misurato sul piano non è la presenza di note estranee, ma che in A e A′ quelle note non risolvono e nascono da un ostinato che ignora l'accordo. Spesso il rimedio più economico è rivedere la diade di Corpo, non la melodia.

### Cited Findings

**Consonanza come fenomeno composito**

- La consonanza simultanea deriva in larga parte da interferenza, periodicità/armonicità e familiarità culturale; il modello composito predice i dati di Bowling et al. con r = .88 — [Harrison & Pearce 2020](https://doi.org/10.1037/rev0000169) [FT*, letti 200 000 caratteri su 333 946 via Europe PMC; DOI riportato a memoria e non verificato; pagina PMC bloccata da verifica del browser].
- Coefficienti non standardizzati: interferenza −1,620; armonicità 1,780; numero di note 0,422; familiarità −0,089 (feature definita come log negativo del conteggio). Il testo descrive i coefficienti standardizzati come «broadly similar» — [Harrison & Pearce 2020](https://doi.org/10.1037/rev0000169) [FT*].
- Nei corpora l'interferenza è il predittore più forte della prevalenza degli accordi in tutti e tre i generi (circa 20–50% della varianza), con effetto negativo; l'armonicità conta soprattutto nella musica classica, poco nel popular, ed è debolmente negativa nel jazz — [Harrison & Pearce 2020](https://doi.org/10.1037/rev0000169) [FT*].
- Gli intervalli appaiono meno consonanti a bassa frequenza; la terza maggiore può essere consonante in alto e dissonante in basso — [Harrison & Pearce 2020](https://doi.org/10.1037/rev0000169) [FT*].

**Tempi forti e note estranee**

- Nel contrappunto di seconda specie il tempo accentato è consonante e la dissonanza compare sul tempo debole come nota di passaggio; la quarta specie ammette dissonanza sul tempo forte per effetto della sincope. La voce enciclopedica segue Fux e rimanda a Salzer & Schachter e Jeppesen — [Counterpoint, Wikipedia](https://en.wikipedia.org/wiki/Counterpoint) [SEC, FT*]. È una convenzione stilistica rinascimentale. Fux non è stato letto.
- Il principio di ancoraggio melodico di Bharucha: una nota instabile si risolve su una nota stabile vicina che la segue. Vicinanza e asimmetria temporale sono le condizioni, e gli ascoltatori giudicano la tensione più risolta con l'ancoraggio — estratti su [Bharucha 1984](https://doi.org/10.1016/0010-0285(84)90018-5) [SEC/AM; DOI riportato a memoria e non verificato].
- Huron deriva molte regole di condotta delle parti da principi percettivi: sei principi di base, in gran parte da stream segregation, più quattro ausiliari — [Huron 2001](https://doi.org/10.1525/mp.2001.19.1.1) [AM]. Il testo non è stato letto.
- Bach evita gli attacchi simultanei proprio per gli intervalli che favoriscono di più la fusione tonale, come unisono, quinta e ottava — [Huron 2008](https://doi.org/10.18061/1811/31695) [AM; repository e copia CORE hanno risposto 403].
- La tesi che dissonanza e streaming siano legati viene da Wright & Bregman 1987, e Huron 2008 ne sarebbe il primo sostegno concreto — [scheda Sonic Field](https://sonicfield.org/library/commentary-on-asynchronous-preparation-of-tonally-fused-intervals-in-polyphonic-) [SEC]. Wright & Bregman non è stato letto: [DOI](https://doi.org/10.1080/07494468708567054) riportato da bibliografia.

**Melodia sopra gli accordi nel pop e nel rock**

- Nel rock l'organizzazione delle altezze è stratificata, con quadri distinti per melodia e accompagnamento — Temperley 2007 citato in [Reed 2022](https://mtosmt.org/ojs/index.php/mto/article/view/876/194) [SEC, FT*]. Una fonte lista fra i segni del «divorzio» note estranee che non risolvono per grado — estratto di ricerca [SEC, non confermato su Reed].
- Nobile distingue tre tipi: *hierarchy divorce* (la melodia articola un orientamento stabile, in genere sulla triade di tonica, e gli accordi cambiano a un livello meno profondo), *loop divorce* (la melodia segue una propria progressione sopra un giro) e *syntax divorce* (melodia e armonia convergono in modi incompatibili) — Nobile 2015 in [Reed 2022](https://mtosmt.org/ojs/index.php/mto/article/view/876/194) [SEC, FT*].
- Nel corpus rock la distribuzione complessiva dei gradi è simile alla musica colta; la posizione metrica delle fondamentali migliora nettamente l'identificazione della tonalità, fino a 97 canzoni su 100 con melodia e fondamentali pesate — [Temperley & de Clercq 2013](https://www.midside.com/publications/temperley_declercq_2013.pdf) [FT parz.]. L'articolo non riporta il tasso di note dell'accordo per posizione metrica.

**Metriche simboliche esistenti**

- Yeh et al. definiscono CTnCTR = (nc + np)/(nc + nn), dove np sono le note estranee entro due semitoni dalla nota successiva. La PCS assegna +1 a unisono, terze, quinta giusta e seste, 0 alla quarta giusta, −1 al resto. La MCTD è una distanza tonale pesata sulla durata — [Yeh et al. 2021](https://arxiv.org/abs/2001.02360) [FT parz.].
- Valori su musica composta da umani: CTnCTR 0,74, PCS 1,42, MCTD 1,03. I metodi a template e HMM superano quei valori (0,91 e 0,89; 1,97 e 1,93), forse troppo conservativi secondo gli autori; il GA con PCS 0,43 ha armonicità troppo bassa. Le metriche sembrano coerenti con la valutazione soggettiva (202 partecipanti). Gli autori notano che i soggetti badano più all'armonicità che alla diversità delle progressioni — [Yeh et al. 2021](https://arxiv.org/abs/2001.02360) [FT parz.].

**Geometria della condotta delle parti**

- Tymoczko rappresenta le successioni come spostamenti fra configurazioni di altezze, con uso diffuso di distanze brevi — [Tymoczko 2006](https://doi.org/10.1126/science.1126287) [SEC, tramite il rapporto del repository; non letto in questa ricerca].

**Misura mia sul piano**

Accordi inferiti come triade diatonica sulla fondamentale di Corpo: A e A′ Dm–G–Am–Dm, B Gm–E♭–F–Gm. Ostinati di Vettore per battuta: A `A F D E` (16) e `D F A G` (15), A′ `D F A G` (31 di 32), B varianti di `G D C D`.

| Sezione | Note dell'accordo, tutte | Tempi 1 e 3 | Tempi 2 e 4 | CTnCTR | PCS |
|---|---:|---:|---:|---:|---:|
| A | 0,56 | 0,74 | 0,37 | 0,63 | 1,05 |
| B | 0,49 | 1,00 | 0,48 | 0,81 | 1,17 |
| A′ | 0,56 | 0,75 | 0,38 | 0,63 | 0,99 |
| Riferimento umano Yeh | — | — | — | 0,74 | 1,42 |

- Sui tempi 2 e 4 di A, CTnCTR 0,39 contro tasso di note dell'accordo 0,37: quasi nessuna nota estranea prosegue per grado. In B lo stesso confronto dà 0,80 contro 0,48: molte note estranee proseguono per grado.
- Coppie Vettore–Corpo sui tempi 2 e 4: tritono nel 6% dei casi in A e A′, seconda minore o settima maggiore nel 13% in B.
- La stima non è strettamente comparabile con Yeh: accordi per battuta anziché per mezza battuta, dataset pop diverso.

### Inferences

- **Il tritono F contro B è un problema di diade, non solo di melodia.** Sul G di A la diade è G3 + B4: se Corpo usasse fondamentale e quinta (G + D), il tritono sparirebbe senza toccare il motivo. Con E♭ (E♭3 + B♭3) e Re di Vettore, la settima maggiore resta perché D è estranea alla triade: lì serve adattare il grado o dichiarare la figura.
- **L'ostinato non è illegittimo.** Una lettura da *hierarchy divorce* è possibile se Vettore resta orientato sulla tonica e gli accordi restano meno profondi. Oggi però la diade di Corpo mette la terza dell'accordo in alto, rendendo quell'accordo profondo proprio dove la melodia lo contraddice.
- **PCS e CTnCTR sono buone metriche a due lati.** Il massimo non è l'obiettivo: i valori troppo alti sono giudicati conservativi. Serve una banda, non una massimizzazione.
- **La dissonanza è espressiva** quando sono vere insieme almeno tre condizioni: è in stato di tensione dichiarato, ha una figura riconoscibile, risolve vicino e in breve tempo. È un'inferenza da convenzioni e dall'ancoraggio, non un risultato sperimentale unico.

**Regole candidate**

| ID | Regola | Calcolo simbolico | Tipo | Base |
|---|---|---|---:|---|
| R-HAR-1 | Sul tempo forte con cambio d'accordo il ruolo dominante è nota dell'accordo, salvo figura dichiarata | Classe di altezza ∈ accordo al frame dell'onset, tempi 1 e 3 | H per il tempo 1 al cambio d'accordo; S altrove | T, convenzione, C indiretto |
| R-HAR-2 | Ogni nota estranea del dominante ha etichetta (`passaggio`, `volta`, `ritardo`, `anticipazione`) e risolve entro 2 semitoni | Controllo sulla nota successiva; np di Yeh | H se tenuta ≥ 1 quarto; S se breve | E (ancoraggio), T |
| R-HAR-3 | Banda di armonicità per frase legata alla tensione | CTnCTR e PCS; valori iniziali intorno a 0,74 e 1,42, da calibrare | S | C |
| R-HAR-4 | Costo per coppie di ruoli a forte interferenza, scalato per registro, durata e velocity | Tabella offline indicizzata per coppia MIDI e ruolo; il planner legge solo la tabella | S; H per coppie sopra soglia tenute ≥ 1 quarto fuori dalla tensione dichiarata | E (Plomp & Levelt), C |
| R-HAR-5 | Riparazione a costo minimo: prima la diade di Corpo, poi il grado di Vettore, infine lo scarto del candidato | Ordine di ricerca dei voicing | S | A |
| R-HAR-6 | Al cambio d'accordo preferire nota comune o moto di grado | Distanza di voice leading fra configurazioni, in semitoni | S | T |
| R-HAR-7 | Stratificazione solo se dichiarata: in `divorzio`, Vettore resta nell'insieme di tonica o pentatonico e Corpo evita la nota che lo contraddice in alto | Modo di frase; appartenenza all'insieme di altezze | S | C, stilistico |
| R-HAR-8 | Limitare unisoni e ottave Vettore–Corpo su attacco simultaneo | Conteggio per frase (misura mia: 31% di unisoni con la voce alta di Corpo in B) | S | C, T |

### Gaps

- Non ho trovato statistiche lette direttamente sul tasso di note dell'accordo della melodia per posizione metrica nel pop. Temperley & de Clercq 2013 non le riporta; Temperley & de Clercq 2019, *Harmony and melody in popular music*, è stato visto solo come scheda.
- *Voice Leading* di Huron (2016) e il testo di «Tone and Voice» non sono stati letti: solo metadati e citazioni di terzi.
- Temperley 2007 e Nobile 2015 sono noti solo tramite Reed 2022. Non ho verificato il criterio sulle note estranee.
- Bharucha 1984 non è stato letto. La regola di risoluzione entro 2 semitoni deriva dalla definizione di Yeh e dal principio di vicinanza, non da una soglia sperimentale.
- Non ho trovato studi sulla melodia ostinata sopra un giro di accordi come tale, né su quando sia percepita come errore.

## Ruoli e salienza: dominante e supporto, stream, densità e contrasto

### Takeaway

Tre flussi indipendenti sono il limite classico per timbri omogenei. Con timbri diversi la capacità sale, ma con costo. Il piano attuale non distingue il dominante: Vettore non è mai la voce superiore in A e A′, mentre la Trama, ruolo di atmosfera, occupa il registro più acuto. DESIGN §8.5 si può tradurre in regole simboliche di assegnazione della salienza.

### Cited Findings

- DESIGN §8.5 chiede di distinguere canale dominante, supporto, atmosfera e opportunità in ogni momento; Pulso aiuta a prevedere, Corpo è massa, Trama densità e atmosfera, Vettore direzione espressiva — [DESIGN.md §8](../../../../DESIGN.md) [FT].
- `dominantRole` è dichiarato nel piano ma non guida la generazione, e nessun termine di punteggio confronta i ruoli — [proposta M2.5](../../../../docs/research/M2_5_Entry_Music_Quality_Proposal.md) [FT].
- Moore divide la tessitura pop in quattro livelli funzionali: beat esplicito, basso funzionale, melodia, riempimento armonico; beat e basso insieme danno il groove — Moore 2012 riportato in [Lavengood 2020](https://www.mtosmt.org/issues/mto.20.26.3/mto.20.26.3.lavengood.pdf) [SEC per Moore; FT parz. per Lavengood].
- Lavengood aggiunge un livello *novelty*: contenuto melodico ma funzione diversa dalla melodia, a volte in opposizione, spesso a domanda e risposta, usato con parsimonia, tipico di intro, outro e interludi. I livelli sono concetti di tessitura, non di timbro — [Lavengood 2020](https://www.mtosmt.org/issues/mto.20.26.3/mto.20.26.3.lavengood.pdf) [FT parz.].
- Nel compito di Huron 1989 su estratti di Bach, gli errori passano da circa 10% con tre voci a circa 50% con quattro. Huron 2016 formula il principio di densità limitata oltre tre voci — riportato da [Siedenburg et al. 2021](https://doi.org/10.3389/fpsyg.2021.608684) [FT*, via PMC8079728; Huron SEC].
- Gli stessi autori notano che i dati provengono solo da miscele timbricamente omogenee. Nei giovani normoudenti la miscela eterogenea dà circa 83% contro 75%; con quattro voci circa 72% contro 56%. Concludono che il limite può valere per miscele omogenee, non necessariamente per quelle eterogenee. Il beneficio è molto minore negli anziani con apparecchi acustici — [Siedenburg et al. 2021](https://doi.org/10.3389/fpsyg.2021.608684) [FT*].
- Gli ingressi di voce si percepiscono più facilmente delle uscite, e quelli delle voci esterne più facilmente di quelli delle voci interne — estratto dell'abstract di [Huron 1989](https://doi.org/10.2307/40285438) [AM].
- Per il pitch la superiorità è della voce acuta, per il timing di quella grave — [Hove et al. 2014](https://doi.org/10.1073/pnas.1402039111) [FT*]. Il tapping e la MMN mostrano l'effetto grave, e la superiorità acuta è riportata da studi precedenti citati nell'articolo.
- La tendenza a sentire due stream si costruisce nel tempo e può azzerarsi con un cambiamento improvviso della sequenza o dell'attenzione. Dipende in parte dall'eccitazione di canali periferici diversi, e la regolarità temporale interna aiuta a stabilizzare uno stream — [Moore & Gockel 2012](https://doi.org/10.1098/rstb.2011.0355) [AM; PMC bloccato da verifica, Royal Society 403, Europe PMC errore 500].

**Misura mia sul piano**

| Ruolo | A | B | A′ |
|---|---|---|---|
| Pulso (nota nominale) | D2 73 Hz | G1 49 Hz | D2 73 Hz |
| Corpo | D3–C5 | G2–D4 | D3–C5 |
| Vettore | D4–A4 | C4–G4 | D3–A3 |
| Trama | D5–C6 | G4–D5 | D5–C6 |

| Vettore rispetto alla voce alta di Corpo | Sopra | Uguale | Sotto | Distanza media |
|---|---:|---:|---:|---:|
| A | 0 | 15 | 109 | 4,9 semitoni |
| B | 70 | 38 | 15 | 2,8 semitoni |
| A′ | 0 | 0 | 127 | 16,5 semitoni |

- Ruoli con onset per battuta: 3 in A1, 4 in A2, 3 o 4 in B, 4 in tutte le 32 battute di A′.
- Onset totali per battuta circa stabili da A2 alla fine. Vettore è sempre sui quarti, 128 note per sezione.
- Corpo e Vettore sono entrambi saw PolyBLEP filtrate ([ascolto, H8](../../../../docs/m2_4_human_listening_2026-10-28.md) [FT]), quindi i due ruoli pitched che si sovrappongono sono anche i più simili di timbro.

### Inferences

- **Mappatura proposta §8.5 → funzione.** Dominante: ruolo con informazione nuova nella frase. Supporto: riempimento o basso. Atmosfera: Trama senza contorno memorizzabile. Opportunità: figura rara nel senso del livello *novelty*, che però lascia aperto il legame col gameplay.
- **Vettore in A′ è un caso limite.** Sta sotto il pad e nel registro del basso di Corpo: se doveva essere dominante, ha perso ogni vantaggio di registro. Può essere una scelta (melodia al basso), ma oggi non è dichiarata.
- **Il conteggio delle voci di Huron non si applica direttamente.** Il compito riguardava linee omogenee di Bach, non basso, batteria e pad. Quattro ruoli funzionali non equivalgono a quattro stream melodici: Pulso e Corpo sostenuto richiedono meno tracciamento. La soglia deve quindi contare **flussi con contorno indipendente**, non ruoli attivi.
- **Il contrasto di tessitura non ha base sperimentale diretta qui.** Il dato disponibile è l'ascolto del progetto (ripetitività 7/7). Va trattato come regola autoriale.

**Regole candidate**

| ID | Regola | Calcolo simbolico | Tipo | Base |
|---|---|---|---:|---|
| R-ROL-1 | Per ogni frase esattamente un dominante; `dominantRole` deve guidare la generazione | Campo di frase; test di coerenza | H | A |
| R-ROL-2 | Il dominante ha almeno due segni di salienza: voce esterna, attacco più alto, densità di contorno massima, timbro unico | Conteggio per frase | S; H su almeno uno | E parziale, A |
| R-ROL-3 | Se un ruolo non dominante è sopra il dominante, deve restare di bassa densità melodica o intermittente | Registro medio per ruolo; varietà intervallare | S | E parziale |
| R-ROL-4 | Massimo 3 flussi con contorno indipendente, 4 solo con timbri distinti e per poche battute | Ruoli con ≥ 2 classi di altezza e ≥ 2 onset nella finestra, esclusi basso sostenuto e percussione | S; H a 4 | E (omogeneo), estensione incerta |
| R-ROL-5 | Ingressi di nuovo materiale in voce esterna; evitare ingressi importanti in voce interna | Registro della nuova voce rispetto ai ruoli attivi | S | E |
| R-ROL-6 | Complementarità ritmica: dominante e supporti sincronizzati sui punti strutturali, sfasati altrove | Rapporto di coincidenza di onset per coppia di ruoli | S | C, T |
| R-ROL-7 | Variare l'insieme dei ruoli attivi: tutti e quattro solo nei punti forti della forma | Maschera di ruolo per frase; quota di frasi a pieno organico | S, autoriale | A |
| R-ROL-8 | Opportunità come livello raro: figura breve, marcata, non in ogni frase | Budget per sezione | S | C, stilistico, A |

### Gaps

- Non ho trovato evidenza diretta su quanti ruoli funzionali eterogenei un ascoltatore segua in musica elettronica.
- Bregman, *Auditory Scene Analysis*, non è stato letto: nessuna affermazione qui dipende direttamente da esso.
- I numeri su stream, confini di fissione e coerenza temporale non sono stati verificati, perché Moore & Gockel non è stato accessibile.
- Il legame fra salienza musicale e leggibilità di gioco (§8.5) non ha riscontro in queste fonti: richiede prova di gioco.
- Definire «contorno indipendente» resta una scelta di progetto.

## Registro e mascheramento: separazione, bande critiche, ripartizione delle frequenze, riferimento al rumore rosa

### Takeaway

Il planner può controllare registro e spaziatura con tabelle derivate dalle bande critiche. Il livello resta un controllo offline. Il dato più scomodo: il divario di 7–9 dB del kick non è anomalo in sé rispetto ai mix misurati da De Man, quindi il «kick troppo basso» può dipendere da spettro, attacco e genere, non solo dal livello. Il metodo del rumore rosa non ha evidenza controllata, e i dischi commerciali sono più ripidi del rosa.

### Cited Findings

**Bande critiche e spaziatura**

- La massima dissonanza tonale si ha a circa il 25% della banda critica e la massima consonanza al 100%, come regola pratica — [Plomp & Levelt 1965](https://doi.org/10.1121/1.1909741) [FT parz.].
- Il valore di 30–40 Hz di Helmholtz vale fra 500 e 1000 Hz; altrove l'ampiezza dell'intervallo più ruvido cresce con la frequenza — [Plomp & Levelt 1965](https://doi.org/10.1121/1.1909741) [FT parz.].
- Nell'analisi di Bach e Dvořák il rapporto fra fondamentali è più largo in basso che in alto, evitando accordi molto dissonanti; un maggior numero di note riduce la distanza media fra parziali — [Plomp & Levelt 1965](https://doi.org/10.1121/1.1909741) [FT parz.].
- Le disposizioni accordali occidentali evitano piccoli intervalli nel registro basso e li ammettono più in alto (Huron & Sellmer 1992, McGowan 2011) — riportato da [Harrison & Pearce 2020](https://doi.org/10.1037/rev0000169) [FT*; fonti primarie SEC].
- ERB di Glasberg & Moore 1990: 24,7(4,37 f/1000 + 1) Hz. Sotto 500 Hz la banda continua a restringersi più di quanto dia la formula, e le stime variano col modello — [CCRMA, ERB](https://ccrma.stanford.edu/~jos/sasp/Equivalent_Rectangular_Bandwidth.html) [AM].

**Livelli e panorama nei mix misurati**

- De Man et al. (8 canzoni, 8 ingegneri in formazione, frammento verso e ritornello) riportano loudness relativa al mix: kick −13,15 ± 3,89 LU, snare −16,78, resto batteria −12,68. Non c'è differenza significativa fra gli elementi non vocali; la voce è significativamente più alta, con un riferimento di circa −3 LU — [De Man et al. 2014](https://archives.ismir.net/ismir2014/paper/000293.pdf) [FT parz.].
- **Incoerenza interna da segnalare:** il testo dà voce −2,7 ± 1,6 LU, ma nella Tabella 2 il valore −2,65 cade nella colonna «Bass» e −9,50 in «Lead vocal», mentre i valori spettrali di quelle colonne sono compatibili con le etichette di colonna. Non ho potuto stabilire quale sia giusto.
- L'isolamento è imperfetto: il rientro fra microfoni rende la loudness effettiva di kick e snare diversa da quella della traccia — [De Man et al. 2014](https://archives.ismir.net/ismir2014/paper/000293.pdf) [FT parz.].
- Il kick mostra un obiettivo spettrale coerente: per molte feature spettrali gli ingegneri discordano su tutti gli strumenti tranne il kick — [De Man et al. 2014](https://archives.ismir.net/ismir2014/paper/000293.pdf) [FT parz.].
- La letteratura tecnica vuole basse frequenze, voce solista e snare al centro; nello studio il basso è significativamente più centrale del mix nella banda bassa — [De Man et al. 2014](https://archives.ismir.net/ismir2014/paper/000293.pdf) [FT parz.].

**Mascheramento e misure**

- In un test su strumenti a coppie, i soggetti hanno preferito gli strumenti che agiscono sul panorama delle due tracce, rispetto a EQ specchiata e separazione di banda — [Wakefield & Dewey 2015](https://aes2.org/publications/elibrary-page/?id=17765) [AM].
- Un sistema automatico di EQ riduce il mascheramento percepito e oggettivo, ma le misure oggettive esistenti sono poco adatte a quantificare il mascheramento percepito in audio multitraccia — [Hafezi & Reiss 2015](https://aes2.org/publications/elibrary-page/?id=17637) [AM].
- Un articolo di divulgazione colloca l'energia di kick e basso fra 20 e 160 Hz e descrive la competizione fra kick e basso — [iZotope](https://www.izotope.com/community/blog/how-to-mix-kick-and-bass) [AM, non peer reviewed].

**Rumore rosa e spettro di riferimento**

- La tecnica di bilanciamento contro rumore rosa viene presentata come rapida e «fairly accurate», ma senza misure, confronti o test alla cieca; l'autore ammette che sub kick e hi-hat possono essere coperti dal rumore — [Bazil, Sound on Sound, 2014](https://www.soundonsound.com/techniques/mixing-pink-noise-reference) [FT*].
- Non ho trovato alcuno studio peer reviewed che testi il metodo; le altre fonti trovate sono articoli di pratica ([Anderton](https://craiganderton.org/improve-your-mixes-with-pink-noise-really/), [Owsinski](https://bobbyowsinskiblog.com/2-methods-for-creating-quick-mixes-with-pink-noise/), AM).
- Su 772 singoli al numero uno dal 1950 al 2010, gli spettri a lungo termine decadono di circa 5 dB per ottava fra 100 e 4000 Hz, diventano più ripidi in alto e hanno un taglio intorno a 60 Hz — [Pestana et al. 2013](https://joshreiss.github.io/documents/2013/Pestana%20Ma%20Reiss%20-%20AES135.pdf) [FT parz.].
- Lo spettro medio dipende dalla prominenza percussiva: più percussioni alzano relativamente bassi e acuti, e le differenze fra generi sarebbero in gran parte un effetto collaterale di ciò — [Elowsson & Friberg 2017](https://aes2.org/publications/elibrary-page/?id=18638) [AM].

**Curve isofoniche**

- ISO 226 è la curva citata dall'esperto e nel riscontro H4; il repository la usa per spiegare perché un kick a 50–140 Hz sembra meno forte a parità di livello elettrico — [ascolto, H4](../../../../docs/m2_4_human_listening_2026-10-28.md) [FT]. Il sito ISO ha risposto 403: la norma non è stata letta e non ne riporto valori numerici.

### Inferences

- **Confronto sul kick.** Con De Man, il kick è circa 10 LU sotto la voce (se vale il testo), un divario simile ai 7–9 dB misurati in CYMATICA. Non è però lo stesso confronto: RMS contro LU, batteria acustica contro kick sintetico breve, corpus non elettronico, rientri. Il dato non smentisce l'esperto: dice che la correzione del solo livello non è garantita. Conviene confrontare attacco, durata e banda del kick con WAV di riferimento.
- **Rumore rosa.** Equal energy per octave corrisponde a −3 dB per ottava, mentre Pestana misura circa −5. Il metodo può servire come partenza d'orecchio per il titolare, non come criterio. Nel nostro caso il suo limite noto (kick coperto dal rumore) tocca proprio il ruolo in discussione.
- **Sequenza consigliata.** Prima la pendenza dello spettro medio confrontata con un riferimento di genere, poi loudness per stem, infine A/B d'ascolto. Tutto offline.
- **Registro ≠ banda spettrale.** Corpo e Vettore hanno parziali ben oltre la fondamentale. Il controllo simbolico sulla fondamentale è necessario, non sufficiente: la tabella offline deve includere lo spettro di ciascun ruolo.

**Regole candidate**

| ID | Regola | Calcolo simbolico | Tipo | Base |
|---|---|---|---:|---|
| R-REG-1 | Bande di registro per funzione, con sovrapposizione limitata fra dominante e supporto | Quota di tempo in cui il dominante sta fra le voci di Corpo o sotto la voce alta | S; H solo se lo stato non dichiara melodia interna | T, C, P |
| R-REG-2 | Spaziatura minima crescente verso il grave | Distanza in Hz fra voci ≥ quota di ERB o CB, tabulata per MIDI | H per intervalli stretti molto gravi; S altrove | E, C |
| R-REG-3 | Ordine di registro coerente con funzione: beat e basso in basso, riempimento in mezzo, melodia sopra, salvo eccezione dichiarata | Mediana di registro per ruolo | S | C, stilistico |
| R-REG-4 | Evitare collisioni di Pulso e basso di Corpo sullo stesso attacco se l'intervallo è stretto | Coppie gravi sul medesimo passo; lo sweep 140→50 Hz attraversa il basso di Corpo | S | P, T |
| R-REG-5 | Pass rate di mascheramento offline: confronto in banda per coppia di ruoli | Solo O; usare misure come indizio, non come verdetto | O | P, E debole |
| R-REG-6 | Spettro complessivo confrontato con riferimento di genere | Pendenza 100 Hz–4 kHz dello spettro medio, offline | O | C |
| R-REG-7 | Ruoli gravi e lead centrati; niente lateralizzazione stabile non dichiarata | Side/Mid per stem offline; simbolico solo se il pan è un parametro | O | C, P |
| R-REG-8 | Livelli relativi tarati su stem, mai dal planner | Loudness per stem, offline | O | P |

### Gaps

- Moore, *An Introduction to the Psychology of Hearing*, Izhaki, *Mixing Audio*, e Senior, *Mixing Secrets*, **non sono stati letti**: li ho visti solo nella bibliografia di De Man et al. 2014. Le ricerche mirate hanno trovato soltanto blog. Nessuna ripartizione di frequenze attribuibile a loro è riportata qui.
- Il mascheramento verso l'alto da parte di basse frequenze è emerso solo da una sintesi di ricerca senza attribuzione affidabile a una singola fonte: non lo cito come fatto verificato.
- Non ho trovato loudness per stem pubblicate su corpus di musica elettronica. Il confronto col kick resta debole.
- Non ho valori tabellari di ISO 226 verificati.
- Il testo integrale di Elowsson & Friberg non è stato letto: non riporto la pendenza.

## Timbro e consonanza: spettri FM inarmonici e rapporti che danno spettri armonici

### Takeaway

Il problema della Trama non è l'inarmonicità in sé, ma la vicinanza a un rapporto intero. Con m/c = 1,88 i parziali formano coppie distanti 0,12·c, ruvide per costruzione, e secondo il modello la gerarchia di consonanza fra le altezze si appiattisce. Rapporti come 1:√2 sono altrettanto inarmonici ma molto meno ruvidi. Un rapporto scelto da una lista chiusa è un intervento piccolo e misurabile.

### Cited Findings

- Con c/m = N1/N2 interi, gli spettri sono armonici e, a fattori comuni rimossi, la fondamentale è f0 = c/N1 = m/N2. Se N2 = 1 ci sono tutte le armoniche, se N2 è pari solo le dispari, se N2 = 3 manca ogni terza — [Chowning 1973](https://www.aes.org/e-lib/browse.cfm?elib=1954) [FT, copia in [fessler/umich](https://web.eecs.umich.edu/%7Efessler/course/100/misc/chowning-73-tso.pdf); URL AES non verificato].
- «Inharmonic spectra will result from ratios of irrational numbers»; le componenti riflesse cadono fra le altre. Chowning ottiene campane e tamburi con c/m = 1/1,4 e indica per i suoni percussivi spettro non armonico ed evoluzione dal complesso al semplice — [Chowning 1973](https://web.eecs.umich.edu/%7Efessler/course/100/misc/chowning-73-tso.pdf) [FT].
- Aggiungere una piccola costante alla modulante (0,5 Hz nell'esempio) fa sì che le componenti riflesse non coincidano con le superiori, producendo un battimento lento utile — [Chowning 1973](https://web.eecs.umich.edu/%7Efessler/course/100/misc/chowning-73-tso.pdf) [FT].
- Plomp & Levelt attribuiscono all'armonicità dei parziali la coincidenza che tiene basse le dissonanze; una deviazione avrebbe l'effetto opposto, una delle ragioni per cui «instruments with inharmonic partials are not used to produce musical chords» — [Plomp & Levelt 1965](https://doi.org/10.1121/1.1909741) [FT parz.].
- Con spettri inarmonici i massimi di consonanza si spostano su rapporti stabiliti dallo spettro; scale gamelan e thailandesi sembrano riflettere gli spettri degli strumenti, e Sethares le spiega con l'interferenza — [Harrison & Pearce 2020](https://doi.org/10.1037/rev0000169) [FT*].
- Il modello di dissonanza di Sethares, sommato sulle coppie di parziali: `min(a1,a2)·(5·e^(−3,51·s·Δf) − 5·e^(−5,75·s·Δf))`, con `s = 0,24/(0,0207·fmin + 18,96)` — [Sethares, Computing sensory dissonance](https://sethares.engr.wisc.edu/comprog.html) [FT*]. L'articolo del 1993 è una scansione senza testo e non l'ho letto.
- Fra i modelli di consonanza, alcuni partono da insiemi di classi di altezza (Milne 2013, Terhardt, Parncutt 1988), altri richiedono altezze (Stolzenburg 2015) o spettri (Parncutt 1989) — [Harrison & Pearce 2020](https://doi.org/10.1037/rev0000169) [FT*]; l'articolo rilascia il pacchetto R `incon`.

**Dal codice del progetto**

- Il rapporto è `1 + 1,76·timbreParam`, e il planner passa `timbreParam = 0,50` alla Trama, quindi m/c = 1,88. L'indice di picco è `min(Carson, 4)·clamp(t + 0,25)`, cioè 3,0 per queste altezze, e scala con l'inviluppo — [voice_allocator.cpp](../../../../engine/audio/voice_allocator.cpp), [music_planner.cpp](../../../../engine/audio/music_planner.cpp), [synth_dsp.h](../../../../engine/audio/synth_dsp.h) [FT, righe pertinenti].
- L'esperto ha giudicato la percussione FM «un po' dissonante» — [ascolto, §1](../../../../docs/m2_4_human_listening_2026-10-28.md) [FT].

**Misura mia su spettri teorici (modello di Sethares; steady state, indice fisso)**

- Parziali per m/c = 1,88 e I = 3 (relativi a c, ampiezza > 0,03): 0,88; 1; 2,76; 2,88; 4,64; 4,76; 6,52; 6,64; 8,4; 8,52; 10,4. Ogni coppia è distante |2c − m| = 0,12·c: 47 Hz per G4, 70 Hz per D5, 106 Hz per A5, 126 Hz per C6.
- 1/1,88 = 25/47: formalmente razionale, con f0 = c/25 secondo la formula di Chowning. In pratica è inarmonico.

| m/c | Inarmonico? | Autodissonanza, D5, I = 1,5 | Idem, I = 3 |
|---|---|---:|---:|
| 1 | no | 0,000 | 0,001 |
| 2 | no, solo dispari | 0,000 | 0,000 |
| 3 | no | 0,005 | 0,051 |
| 1,5 (c:m = 2:3) | no | 0,012 | 0,079 |
| 1,4 | di fatto sì | 0,004 | 0,039 |
| √2 | sì | 0,005 | 0,042 |
| 2,76 | sì | 0,013 | 0,094 |
| 1,76 | sì | 0,167 | 0,482 |
| **1,88 (attuale)** | sì | **0,508** | **0,848** |

Per confronto, una saw a 10 armoniche su D5 vale 0,028.

- Scansione del rapporto: zona peggiore intorno a m/c 1,8–2,2 esclusi i valori esatti (0,66 a 1,96 e a 2,04, contro 0 a 2,00); zona stretta intorno a 1,08; zone quiete 1,24–1,48 e ≥ 2,4. Sono t = 0 (1:1), t ≈ 0,284 (1,5), t ≈ 0,568 (2,0).
- Dissonanza aggiunta da una nota Trama (I = 1,5) sopra la diade Corpo D3 + A4:

| Nota Trama | m/c = 1 | m/c = 2 | m/c = 1,88 |
|---|---:|---:|---:|
| A5 (quinta della diade) | 0,08 | 0,08 | 0,88 |
| D5 | 0,27 | 0,30 | 1,15 |
| C6 | 0,56 | 0,36 | 1,14 |
| E5 | 0,44 | 0,47 | 1,00 |
| F5 | 0,88 | 0,51 | 0,94 |

- Intervalli fra due note Trama a 440 Hz, I = 1,5: con 1:1 → P5 0,04, M3 0,12, TT 0,21, m2 0,96, P8 0,00. Con 1:1,88 → P5 1,30, M3 1,73, TT 1,34, m2 3,88, P8 1,02. Con 1:2 → P5 0,08, M3 0,18, TT 0,07, m2 1,41.

### Inferences

- **La ragione del giudizio «dissonante» è plausibile e indipendente dall'accordo.** Il modello dà ruvidità in una singola nota. Cambiare armonia non la risolve. È un riscontro del modello, non una prova percettiva, che va confermata all'ascolto.
- **Con 1,88 la scelta dell'altezza pesa poco.** L'aumento di dissonanza sulla diade va da 0,88 a 1,15 su tutte le note provate; con 1:1 va da 0,08 a 0,88. Un direttore che sceglie note consonanti per la Trama guadagnerebbe poco finché il rapporto resta questo. **Conviene quindi correggere prima il rapporto, poi le regole di altezza della Trama.**
- **1,88 è il caso inverso di Chowning.** Lui scosta di 0,5 Hz per ottenere un battimento lento; qui lo scarto dal rapporto 2 vale decine di Hz e cade nella zona di ruvidità (Plomp & Levelt).
- **1:2 non è gratis.** Con le sole armoniche dispari il tritono risulta consonante quanto la quinta. Per una Trama che deve sostenere la gerarchia tonale, 1:1 è il più «tonale»; 1:2 è più cavo; 2:3 e 1:3 più colorati.
- **Inarmonicità come colore dichiarato.** I rapporti 1:√2 o 1:1,4 restano utili per note brevi e percussive, e danno un'identità timbrica senza la ruvidità di 1,88. Questo riduce il rischio di perdere il carattere «FM» della Trama. La qualità artistica va però giudicata all'ascolto.
- **Vincolo di progetto.** Il planner può usare una tabella calcolata offline dallo spettro nominale di ogni ruolo, indicizzata per altezze e profilo timbrico. Al momento della pianificazione è un dato simbolico, senza analisi audio, purché versionata.

**Regole candidate**

| ID | Regola | Calcolo simbolico | Tipo | Base |
|---|---|---|---:|---|
| R-TIM-1 | `timbreParam` della Trama scelto da lista chiusa di rapporti; niente rapporti entro la zona vicina a un intero piccolo | Appartenenza alla lista; zona proibita da tabella offline | H | T, misura mia, da validare |
| R-TIM-2 | Rapporto legato al contesto: armonico quando la Trama porta altezze, inarmonico solo per note brevi o in tensione dichiarata | Regola di stato; durata della nota | S | T |
| R-TIM-3 | Indice e rapporto si progettano insieme: il rapporto ha effetto proporzionale alla banda | Tabella `(rapporto, indice) → costo` | S | T |
| R-TIM-4 | Tabella di interferenza per coppie (ruolo, MIDI) e (ruolo, MIDI), generata offline e versionata | Consultazione nel planner; R-HAR-4 la usa | S | T, E |
| R-TIM-5 | Timbri dei ruoli che si sovrappongono devono differire | Offline: distanza spettrale per coppia; nel planner solo profilo | O | E parziale |
| R-TIM-6 | Non inseguire scale dedicate allo spettro: il progetto resta su altezze temperate e sceglie lo spettro adatto | Decisione di progetto | S | A |

### Gaps

- Il risultato su 1,88 è un calcolo di modello: un solo meccanismo, l'interferenza, senza armonicità, inviluppo o durata. Le note reali durano 107–125 ms e l'indice decade. Ascolto o renderer offline devono confermare l'effetto.
- Non ho letto Sethares 1993 né *Tuning, Timbre, Spectrum, Scale*: ho usato solo il codice pubblicato dall'autore.
- Parncutt è noto solo tramite Harrison & Pearce.
- Il codice non ha un parametro separato per rapporto e indice: `timbreParam` li muove insieme, quindi cambiare rapporto cambia anche l'indice. Serve una decisione di progetto.
- Non ho trovato studi percettivi su FM a rapporto quasi intero in contesto armonico.

## Quali regole hanno supporto sperimentale e quali sono convenzioni di genere?

### Takeaway

Pochi fatti sono sperimentali e robusti: interferenza legata alla banda critica, peggioramento in basso, inverted-U della sincope in condizioni specifiche, limite di tracciamento per linee omogenee, ruolo del grave nel timing. Quasi tutto il resto è corpus di un genere o convenzione. Il planner deve tenere come duri solo gli invarianti cheap e reversibili; le preferenze di genere vanno come punteggi versionati.

### Cited Findings

Riepilogo delle evidenze già citate nelle sezioni precedenti, con la classe di supporto.

| Affermazione | Supporto | Vale in | Forza | Fonte |
|---|---|---|---|---|
| Interferenza massima a ¼ di banda critica, peggiore in basso | Sperimentale, analisi di partiture | Indipendente dal genere, per parziali note | Alta | [Plomp & Levelt 1965](https://doi.org/10.1121/1.1909741) FT parz.; [Harrison & Pearce 2020](https://doi.org/10.1037/rev0000169) FT* |
| Consonanza composita (interferenza, armonicità, familiarità) | Rianalisi su oltre 500 partecipanti, corpora > 100 000 brani | Occidente; familiarità culturale | Alta | [Harrison & Pearce 2020](https://doi.org/10.1037/rev0000169) FT* |
| Sincope media ottimale | Sperimentale | Loop di batteria funk con hi-hat stabile, valutazioni soggettive | Media | [Witek 2014](https://doi.org/10.1371/journal.pone.0094446) FT* |
| Spostamento di fase con forte sincope | Sperimentale | Ritmi di laboratorio | Media | [Fitch & Rosenfeld 2007](https://peeref.com/works/67049890) AM |
| Timing dominato dal grave | Sperimentale | Toni di pianoforte; effetto piccolo | Media | [Hove 2014](https://doi.org/10.1073/pnas.1402039111) FT* |
| Limite di 3 voci | Sperimentale | Voci omogenee; non generalizzabile | Media | [Siedenburg 2021](https://doi.org/10.3389/fpsyg.2021.608684) FT* |
| Vantaggio timbrico | Sperimentale | Giovani normoudenti | Media | [Siedenburg 2021](https://doi.org/10.3389/fpsyg.2021.608684) FT* |
| Ancoraggio melodico | Sperimentale | Tonale, contesti di laboratorio | Media, non letto | [Bharucha 1984](https://doi.org/10.1016/0010-0285(84)90018-5) SEC/AM |
| LHL misura la complessità | Correlazionale | Giudizi senza metro esplicito | Media | [Smith & Honing 2006](https://www.mcg.uva.nl/mcg-2023/papers/smith-honing-2006.pdf) FT |
| Melodia sincopata anticipatoria, unità ~250 ms | Corpus | Voce rock 1950–1990 | Media, per quel genere | [Tan 2019](https://doi.org/10.1525/MP.2019.36.4.353) FT parz. |
| Melodia divorziata dall'armonia | Analisi teorica | Rock | Stilistica | [Reed 2022](https://mtosmt.org/ojs/index.php/mto/article/view/876/194) FT* |
| Strati funzionali, *novelty* | Teoria analitica | Pop e rock; synth pop | Stilistica | [Lavengood 2020](https://www.mtosmt.org/issues/mto.20.26.3/mto.20.26.3.lavengood.pdf) FT parz. |
| Dissonanza di spostamento | Teoria analitica | EDM | Stilistica | [Butler 2001](https://www.mtosmt.org/issues/mto.01.7.6/mto.01.7.6.butler_essay.html) FT* |
| Specie: forte consonante, debole di passaggio | Convenzione pedagogica | Polifonia storica | Stilistica | [Counterpoint](https://en.wikipedia.org/wiki/Counterpoint) SEC |
| Backbeat come riferimento | Pratica, un esperto, modelli di beat tracking | Pop, rock, funk | Debole | [Goto 1995](https://staff.aist.go.jp/m.goto/PAPER/ICMC95/node1.html) AM; [ascolto §3ter](../../../../docs/m2_4_human_listening_2026-10-28.md) FT |
| Pendenza di circa −5 dB/ottava | Corpus | Numeri uno 1950–2010 | Media | [Pestana 2013](https://joshreiss.github.io/documents/2013/Pestana%20Ma%20Reiss%20-%20AES135.pdf) FT parz. |
| Bilanciamento col rumore rosa | Solo pratica | Nessuna prova | Debole | [Bazil 2014](https://www.soundonsound.com/techniques/mixing-pink-noise-reference) FT* |

Conflitti da tenere visibili.

- **Dissonanza metrica:** la lettura psicologica (troppa sincope disturba) e la prassi EDM (emancipazione della dissonanza metrica) non sono in contraddizione se conta l'attesa stabilita, ma non lo prova nessuna fonte letta.
- **Tensione:** Witek lega l'eccesso alla perdita del metro, mentre Keller & Schubert e Temperley 1999 riportano poca instabilità o tensione inaffidabile nelle sincopi — riportati da [Fiorin & Delfitto 2024](https://doi.org/10.3389/fpsyg.2024.1304485) [SEC].
- **Armonicità:** Yeh tratta le note estranee come costo, Nobile e Temperley come struttura legittima.
- **Densità:** Huron pone il limite a tre, Siedenburg lo mette in dubbio per timbri diversi.
- **Kick:** l'ascolto dice troppo basso, De Man mostra divari simili in un corpus diverso.

### Inferences

**Proposta di ripartizione per il direttore**

- **Vincoli duri candidati, tutti economici e reversibili:** R-MET-1, R-MET-2 in stato ancorato, R-MET-9, R-HAR-1 solo sul tempo 1 al cambio d'accordo, R-HAR-2 per note tenute, R-ROL-1, R-REG-2 nell'estremo grave, R-TIM-1.
- **Preferenze:** tutte le altre, con pesi espliciti e versionati.
- **Offline:** livelli, panorama, mascheramento, spettro complessivo e distanza timbrica.
- **Non sono vietate dalle regole:** spostamento metrico, dissonanza, melodia al basso, stratificazione. Sono ammesse quando dichiarate nello stato di frase. Questo risponde al rischio di varietà fittizia e mantiene la distinzione fra gestione della tensione e divieto della dissonanza.

**Ordine d'intervento suggerito (da confermare con evidenza)**

1. Rapporto FM della Trama: piccolo, isolabile, con l'effetto modellato più forte.
2. Voicing di Corpo su accordi con note contraddette: rimuove il tritono senza toccare i motivi.
3. Registro di Vettore in A′ e rapporto con la voce alta di Corpo.
4. Stato metrico esplicito: rende intenzionali rotazione e backbeat.
5. Livello del kick: dopo le misure per stem, perché cambia il metro percepito.

**Esperimenti minimi di conferma**

- A/B Trama: 1,88, 2,0, 1,0, √2, stesse note e stesso livello in loudness.
- A/B àncora, a più livelli di kick: kick su 1, backbeat su 2 e 4, `2,6,10,14` tal quale, con compito di battere il tempo per misurare la fase scelta.
- Un solo cambiamento per pacchetto, come chiede la proposta M2.5.

### Gaps

- Nessuna delle regole è stata validata su musica sintetica a otto voci.
- Tutti i pesi e le soglie sono proposte. Il rapporto corretto per la Trama, la lunghezza dei cicli di spostamento e le bande di sincope devono uscire da pilot registrati.
- I giudizi disponibili provengono da un solo esperto, senza condizioni controllate.
- Mancano corpus simbolici di EDM paragonabili al corpus RS, su cui calibrare SQ e ASQ per questo genere.

### Registro delle fonti e di ciò che non è stato letto

| Fonte | URL o DOI | Livello |
|---|---|---|
| Chowning 1973 | [PDF](https://web.eecs.umich.edu/%7Efessler/course/100/misc/chowning-73-tso.pdf) | FT |
| Smith & Honing 2006 | [PDF](https://www.mcg.uva.nl/mcg-2023/papers/smith-honing-2006.pdf) | FT |
| Tan, Lustig & Temperley 2019 | [DOI](https://doi.org/10.1525/MP.2019.36.4.353), [copia PDF](https://www.jurispro.com/files/articles/anticipatorysyncopationinrockacorpusstudymusicperception364353370_8745.pdf) | FT parz. |
| Temperley & de Clercq 2013 | [PDF](https://www.midside.com/publications/temperley_declercq_2013.pdf) | FT parz. |
| Plomp & Levelt 1965 | [DOI](https://doi.org/10.1121/1.1909741), [PDF](https://pure.mpg.de/rest/items/item_66382_5/component/file_468040/content) | FT parz. |
| Pestana et al. 2013 | [PDF](https://joshreiss.github.io/documents/2013/Pestana%20Ma%20Reiss%20-%20AES135.pdf) | FT parz. |
| De Man et al. 2014 | [PDF](https://archives.ismir.net/ismir2014/paper/000293.pdf) | FT parz. |
| Yeh et al. 2021 | [arXiv](https://arxiv.org/abs/2001.02360) | FT parz. |
| Lavengood 2020 | [PDF](https://www.mtosmt.org/issues/mto.20.26.3/mto.20.26.3.lavengood.pdf) | FT parz. |
| Witek et al. 2014, PLOS ONE, e correzione | [DOI](https://doi.org/10.1371/journal.pone.0094446), [correzione](https://doi.org/10.1371/journal.pone.0139409) | FT* |
| Hove et al. 2014 | [DOI](https://doi.org/10.1073/pnas.1402039111) (PMC4104866; pagina PNAS 403) | FT* |
| Harrison & Pearce 2020 | [DOI](https://doi.org/10.1037/rev0000169), non verificato; testo via Europe PMC, PMC7032667 | FT* parziale |
| Siedenburg et al. 2021 | [DOI](https://doi.org/10.3389/fpsyg.2021.608684) (PMC8079728) | FT* |
| Fiorin & Delfitto 2024 | [DOI](https://doi.org/10.3389/fpsyg.2024.1304485) | FT* parziale |
| Reed 2022 | [MTO](https://mtosmt.org/ojs/index.php/mto/article/view/876/194) | FT* |
| Butler 2001 | [MTO](https://www.mtosmt.org/issues/mto.01.7.6/mto.01.7.6.butler_essay.html) | FT* |
| Bazil 2014, Sound on Sound | [URL](https://www.soundonsound.com/techniques/mixing-pink-noise-reference) | FT* |
| Sethares, codice | [URL](https://sethares.engr.wisc.edu/comprog.html) | FT* |
| Counterpoint, Wikipedia | [URL](https://en.wikipedia.org/wiki/Counterpoint) | FT*, fonte terziaria |
| Witek et al. 2014, Music Perception | [DOI](https://doi.org/10.1525/mp.2014.32.2.201) | AM |
| Fitch & Rosenfeld 2007 | [DOI](https://doi.org/10.1525/mp.2007.25.1.43), [scheda](https://peeref.com/works/67049890) | AM |
| Tal et al. 2017 | [DOI](https://doi.org/10.1523/JNEUROSCI.2500-16.2017) | AM |
| Huron 1989 | [DOI](https://doi.org/10.2307/40285438) | AM; numeri SEC |
| Huron 2001 | [DOI](https://doi.org/10.1525/mp.2001.19.1.1) | AM |
| Huron 2008 | [DOI](https://doi.org/10.18061/1811/31695) | AM |
| Moore & Gockel 2012 | [DOI](https://doi.org/10.1098/rstb.2011.0355) | AM |
| Goto & Muraoka 1995 | [URL](https://staff.aist.go.jp/m.goto/PAPER/ICMC95/node1.html) | AM |
| Wakefield & Dewey 2015 | [URL](https://aes2.org/publications/elibrary-page/?id=17765) | AM |
| Hafezi & Reiss 2015 | [URL](https://aes2.org/publications/elibrary-page/?id=17637) | AM |
| Elowsson & Friberg 2017 | [URL](https://aes2.org/publications/elibrary-page/?id=18638) | AM |
| Articoli di pratica (Anderton, Owsinski, iZotope) | [Anderton](https://craiganderton.org/improve-your-mixes-with-pink-noise-really/), [Owsinski](https://bobbyowsinskiblog.com/2-methods-for-creating-quick-mixes-with-pink-noise/), [iZotope](https://www.izotope.com/community/blog/how-to-mix-kick-and-bass) | AM, non peer reviewed |
| Glasberg & Moore 1990, formula ERB | [CCRMA](https://ccrma.stanford.edu/~jos/sasp/Equivalent_Rectangular_Bandwidth.html) | SEC/AM |
| Longuet-Higgins & Lee 1984 | tramite Smith & Honing | SEC |
| Povel & Essens 1985 | [DOI](https://doi.org/10.2307/40285311), non verificato | SEC/AM |
| Lerdahl & Jackendoff 1983 | [Wikipedia](https://en.wikipedia.org/wiki/Generative_theory_of_tonal_music) | SEC |
| London 2002 | [scheda](https://sonicfield.org/library/cognitive-constraints-on-metric-systems-some-observations-and-hypotheses) | SEC/AM |
| Temperley 2007, Nobile 2015 | tramite Reed 2022 | SEC |
| Allan Moore 2012 | tramite Lavengood 2020 | SEC |
| Bharucha 1984 | [DOI](https://doi.org/10.1016/0010-0285(84)90018-5), non verificato | SEC/AM |
| Wright & Bregman 1987 | [DOI](https://doi.org/10.1080/07494468708567054) | SEC |
| Duinker 2017 su Butler 2006 | [PDF](https://creaa.unistra.fr/websites/gream/Activites/Euromac_2017_-_Postprint_-_Extended_abstract_-_DUINKER_Ben.pdf) | SEC/AM |
| Tymoczko 2006 | [DOI](https://doi.org/10.1126/science.1126287) | SEC |
| Temperley 1999, Keller & Schubert 2011, Chapin et al. 2010 | tramite Fiorin & Delfitto 2024 | SEC |

**Non letti o non raggiungibili.**

- *Libri non letti:* Huron, *Voice Leading* (2016); Bregman, *Auditory Scene Analysis*; London, *Hearing in Time*; Lerdahl & Jackendoff; Moore, *An Introduction to the Psychology of Hearing*; Izhaki, *Mixing Audio*; Senior, *Mixing Secrets*; Butler, *Unlocking the Groove*; Sethares, *Tuning, Timbre, Spectrum, Scale*.
- *Norma non letta:* ISO 226, sito ISO con risposta 403.
- *Accesso negato o fallito:* copia di Fitch & Rosenfeld su uvic.ca (connessione rifiutata); pagina PNAS di Hove (403); Springer per Povel & Essens (reindirizzamento al login); PMC per Moore & Gockel e Harrison & Pearce (verifica del browser, non aggirata per quella pagina; per Harrison & Pearce ho usato Europe PMC); Europe PMC per Moore & Gockel (errore 500); Royal Society (403); CORE (403); kb.osu.edu per Huron 2008 (403); un articolo Music & Science del 2026 di argomento non determinato ([DOI](https://doi.org/10.1177/20592043261466428), 403); Open Music Theory (403 e connessione rifiutata).
- *File illeggibile:* Sethares 1993, PDF scansionato senza strato di testo e strumento di rendering non disponibile.
- *DOI riportati a memoria e non verificati:* Povel & Essens 1985, Harrison & Pearce 2020, Bharucha 1984.
