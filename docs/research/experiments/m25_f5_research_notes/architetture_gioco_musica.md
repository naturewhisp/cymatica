# Architetture gioco/musica: chi decide cosa e a quale scala temporale

Note che estendono `docs/research/M2_4_Inc5_Entry_Research.md` §5 (G1–G24). I risultati di quelle fonti non sono ripetuti: compaiono solo come riferimento (es. «vedi G4»). Le nuove fonti hanno ID A1–A44; il registro completo e le pagine non lette sono in fondo alla sezione 5.

**Livelli di verifica.** FT: testo letto da me direttamente (testo della pagina nel browser o output di `pdftotext`). FT\*: testo letto tramite un estrattore automatico, che restituisce una sintesi. AM: solo abstract o metadati. SEC: nota solo tramite fonte secondaria o riassunto di ricerca. «Parziale» indica la porzione effettivamente letta. I brevetti descrivono ciò che rivendicano, non necessariamente il prodotto uscito.

**Citazioni.** Tutto è parafrasato, salvo il termine di Morasky indicato in 1. Nomi di API e di opzioni sono identificatori, non citazioni.

## 1. Come i sistemi documentati dividono le decisioni fra logica di gioco e logica musicale?

### Takeaway

Lo schema dominante, da iMUSE (1991) a Hades II e Total War: il gioco esprime un intento e la logica musicale lo esegue al primo punto autorizzato, scelto in anticipo da chi compone. In mezzo c'è spesso un mediatore in codice o dati, con coda, priorità e validità. Il caso più vicino a un «direttore musicale» è Total War: WARHAMMER 2; la separazione più esplicita fra due direttori è Left 4 Dead.

### Cited Findings

**Sintesi per sistema**

| Sistema | Decide il gioco | Decide la logica musicale | Fissa i confini |
|---|---|---|---|
| iMUSE | cosa cambiare, abilitando hook o accodando comandi; può anche saltare subito | esegue al prossimo punto di decisione | compositore ([A1, FT\*](https://patents.google.com/patent/US5315057)) |
| Halo | `start`, `start_alt`, `stop`, volume, durata massima | sequenza in/loop/out, scelta pesata dei loop | sound designer ([A7, FT\*](https://halo.bungie.org/misc/gdc.2002.music)) |
| Left 4 Dead | AI Director: popolazione di minacce | Music Director separato: regole sull'ambiente vicino al giocatore | compositore e audio designer ([A3, FT/SEC](https://left4dead.fandom.com/wiki/Developer_Commentary_(Left_4_Dead))) |
| Dead Space | stato e paura totale emessa nei dintorni | mix continuo di 4 strati | sound designer ([A8, FT\*](https://www.originalsoundversion.com/dead-space-sound-design-in-space-no-one-can-hear-interns-scream-they-are-dead-interview/)) |
| Total War: WARHAMMER 2 | eventi e condizioni | coda prioritaria in codice, regole Wwise | composizione a blocchi e marker ([A9, FT](https://www.audiokinetic.com/en/community/blog/total-war-warhammer-2-how-we-construct-and-systemically-use-our-music/)) |
| Metal: Hellsinger | livello di prestazione, cambio d'atto per luogo o script | crossfade fra tier, attesa del punto di transizione | audio designer ([A11, FT\*](https://patents.google.com/patent/US11358065)) |
| Hades / Hades II | avanzamento di sezione via parametro | logica FMOD su regioni e quantizzazione | autore dell'evento FMOD ([A13, FT\*](https://gameplay.co/hades-game-music-sound-design-darren-korb-supergiant-games/); [A14, FT\*](https://sgg-modding.github.io/Hades2ModWiki/docs/creating-mods/audio/adding-dynamic-music)) |

**iMUSE (brevetto US 5315057, Land e McConnell, depositato il 25/11/1991, concesso il 24/5/1994)**

- Il brevetto separa il sistema dirigente (il gioco) dal sound driver, che naviga un database compositivo: MIDI più messaggi condizionali in formato SysEx ([A1, FT\*](https://patents.google.com/patent/US5315057)).
- Il compositore colloca i punti di decisione, che formano un albero. Un hook esegue la sua azione (salto, trasposizione, volume, attivazione di parte) se il suo id coincide col valore impostato dal gioco, poi si disabilita ([A1, FT\*](https://patents.google.com/patent/US5315057)).
- I marker chiudono le sequenze di transizione ed eseguono i comandi accodati in precedenza dal gioco. È attivo solo il trigger in testa alla coda ([A1, FT\*](https://patents.google.com/patent/US5315057)).
- Esempio del brevetto: in un combattimento il gioco abilita una parte quando il lottatore perde terreno e la parte entra fra due frasi. A fine scontro abilita i salti per l'esito; il sequencer salta all'hook di fine battuta, suona la transizione e il marker avvia la musica di vittoria o sconfitta ([A1, FT\*](https://patents.google.com/patent/US5315057)).
- Un salto diretto del gioco è invece immediato: il gioco può scavalcare i punti d'autore ([A1, FT\*](https://patents.google.com/patent/US5315057)).

**Left 4 Dead (AI Director e Music Director)**

- L'AI Director stima l'intensità di ogni sopravvissuto, ne segue il massimo sui quattro e rimuove le minacce principali quando è troppo alta. Il ciclo è Build Up, Sustain Peak, Peak Fade, Relax ([A2, FT parziale](https://steamcdn-a.akamaihd.net/apps/valve/2009/ai_systems_of_l4d_mike_booth.pdf)).
- L'intensità sale con danni, incapacitazione, cadute e morte di infetti vicini; decade verso zero, ma non mentre gli infetti ingaggiano il sopravvissuto. Il sistema modifica il ritmo, non l'ampiezza della difficoltà; gli scontri con i boss ne restano fuori ([A2, FT parziale](https://steamcdn-a.akamaihd.net/apps/valve/2009/ai_systems_of_l4d_mike_booth.pdf)).
- Nel testo estratto delle slide di Booth la parola «music» non compare ([A2, FT parziale](https://steamcdn-a.akamaihd.net/apps/valve/2009/ai_systems_of_l4d_mike_booth.pdf)).
- Nel commento degli sviluppatori, Larkin descrive un music director che gira accanto all'AI Director e segue l'esperienza del giocatore, non il suo stato emotivo. Il motore musicale è multitraccia lato client, unico per giocatore, ascoltabile dagli spettatori ([A3, FT/SEC](https://left4dead.fandom.com/wiki/Developer_Commentary_(Left_4_Dead))).
- Morasky: un sistema semplice esamina l'ambiente immediato del giocatore e applica regole reattive e scalari a musica e volume. Lo stesso sistema governa il mix dinamico ([A3, FT/SEC](https://left4dead.fandom.com/wiki/Developer_Commentary_(Left_4_Dead))).
- Morasky descrive poi un motore con priorità e un «audio director» basato su IA che tiene il mix nel suo intervallo ([A5, FT\*](https://www.originalsoundversion.com/one-musician-to-another-mike-morasky-interview/)).
- Van Buren: tutta la musica usa una sola scala ibrida, così quasi ogni brano si accosta agli altri ([A3, FT/SEC](https://left4dead.fandom.com/wiki/Developer_Commentary_(Left_4_Dead))). Una pagina wiki del 2009, anonima, riporta brani di 5–30 secondi ([A6, FT\*](https://pcg.wikidot.com/left4dead-music)).

**Total War: WARHAMMER 2 (2017, articolo del 2018)**

- Il gioco ha due sistemi musicali distinti, campagna e battaglia. Il modulo in codice mantiene una «Music Queue»: le variabili di ogni stato sono definite nei dati, tracciate a runtime, e la coda gestisce condizioni simultanee e durata di validità ([A9, FT](https://www.audiokinetic.com/en/community/blog/total-war-warhammer-2-how-we-construct-and-systemically-use-our-music/)).
- In campagna gli eventi hanno priorità. Un evento più prioritario cambia cue solo al punto di transizione del brano uscente o alla sua fine naturale. Ogni tipo di cue ha un timer di validità, un cooldown e un'opzione di transizione istantanea ([A9, FT](https://www.audiokinetic.com/en/community/blog/total-war-warhammer-2-how-we-construct-and-systemically-use-our-music/)).
- Per non aggiornare lo stato a ogni frame, il sistema attende marker nel brano. È la musica a dare il momento in cui la coda viene letta ([A9, FT](https://www.audiokinetic.com/en/community/blog/total-war-warhammer-2-how-we-construct-and-systemically-use-our-music/)).
- In battaglia i tipi di cue sono Reveal, Threat, March, Skirmish, Dynamic Action e stinger di esito; l'ordine segue azioni ed eventi. Le condizioni usano soglie su forza dell'esercito, percentuale di unità in movimento e unità in combattimento ([A9, FT](https://www.audiokinetic.com/en/community/blog/total-war-warhammer-2-how-we-construct-and-systemically-use-our-music/)).
- La regola compositiva è parte dell'architettura: radici limitate a tre, modulazione di una terza minore, tempi per tipo di cue, niente tempi variabili, metri definiti, blocchi di 8 battute. Il sistema traccia la radice e sceglie la transizione adatta ([A9, FT](https://www.audiokinetic.com/en/community/blog/total-war-warhammer-2-how-we-construct-and-systemically-use-our-music/)).

**Metal: Hellsinger (brevetto Funcom US 11358065, concesso il 14/6/2022)**

- Ogni brano è diviso in atti: loop autonomi, spesso di 1–2 minuti, ciascuno con una traccia per ognuno dei cinque tier di prestazione. L'atto cambia per azione del giocatore (nuova stanza, evento scriptato), non per tempo o musica ([A11, FT\*](https://patents.google.com/patent/US11358065)).
- Il designer audio colloca i punti di transizione; il gioco, se il trigger cade fra due punti, aspetta il successivo. Prima del nuovo atto suona un segmento di 1–4 battute ([A11, FT\*](https://patents.google.com/patent/US11358065)).
- Il cambio di tier avviene dopo un'attesa casuale di 1–3 secondi, con crossfade di circa 2 secondi ([A11, FT\*](https://patents.google.com/patent/US11358065)). L'intervista a Goldfarb conferma cinque gradazioni, voce al livello massimo, ritardo e smoothing contro i cambi bruschi ([A12, FT\*](https://gamedeveloper.com/design/shredding-for-satan-how-i-metal-hellsinger-i-designed-fps-harmonies-in-hell)).

**Hades e Hades II**

- Korb: ogni bioma ha una playlist (4 brani Tartaro, 3 Asfodelo, 3 Elisio), con un'introduzione fissa a ogni run. Le sezioni avanzano tramite marker su confini di battuta; batteria e basso suonano in combattimento; ai boss si passa a una sezione rock a stem completi; la vittoria attiva una coda conclusiva; gli stem di chitarra e basso si combinano in modo semi-casuale fra le stanze. Il controllo è in FMOD Studio ([A13, FT\*](https://gameplay.co/hades-game-music-sound-design-darren-korb-supergiant-games/)).
- Hades II (guida della comunità di modding, non Supergiant): il gioco chiama `SetMusicSection`, che imposta il parametro `Section`. L'evento FMOD decide cicli, salti e momento consentito ([A14, FT\*](https://sgg-modding.github.io/Hades2ModWiki/docs/creating-mods/audio/adding-dynamic-music)).

**Middleware: dove risiede la decisione**

- Wwise: la regola di transizione fissa dove esce la sorgente (`Immediate`, `Next Grid`, `Next Bar`, `Next Beat`, `Next Cue`, `Next Custom Cue`, `Exit Cue`; predefinito `Exit Cue`) e dove entra la destinazione (`Entry Cue`, stesso istante, cue casuale, `Last Exit Position`; predefinito `Entry Cue`) ([A16, FT parziale](https://www.audiokinetic.com/en/public-library/2025.1.12_9287/?source=Help&id=transitions_tab_music_objects)).
- Wwise: gli stinger sono segmenti sovrapposti chiamati da Trigger, con punto d'ingresso proprio (predefinito `Immediate`) e blocco di ripetizione in secondi; le chiamate nel blocco vengono scartate ([A17, FT parziale](https://www.audiokinetic.com/en/public-library/2025.1.12_9287/?source=Help&id=stingers_tab_music_objects)).
- FMOD: la logica vive nell'evento. Marker e regioni hanno condizioni su parametri e probabilità, priorità per posizione verticale, e formano «logic point» che sono alberi di decisione. Solo regioni di transizione e magnet regions si quantizzano; senza tempo marker valgono 120 BPM e 4/4 ([A22, FT](https://www.fmod.com/docs/2.03/studio/authoring-events.html)).
- DirectMusic (documentazione archiviata) aveva già flag di confine (griglia, beat, misura, marker, fine segmento), segmenti primari, secondari e di controllo, e un flag che compone e suona un segmento di transizione da un template ([A24, FT\*](https://learn.microsoft.com/en-us/previous-versions/ms808244(v=msdn.10))). ADX2 usa blocchi con divisioni interne come punti di transizione ([A25, FT\*](https://blog.criware.com/index.php/2016/06/14/interactive-music-with-adx2/)).

**Sistemi accademici**

- Il sistema di Hutchings e McCormack (estende G19) è un pacchetto separato che riceve messaggi dal gioco: grafo di concetti e affetti ad attivazione diffusa, più agenti con ruoli Harmony, Melody e Percussive Rhythm ([A28, FT parziale](https://arxiv.org/pdf/1907.01154)).
- L'agente armonico non scrive note: produce una matrice di risorse per altezza e suddivisione. Il confronto di confidenza fra agente armonico e primo agente melodico stabilisce il leader. Ogni nota consuma risorse, e questo tiene lontani gli altri agenti melodici dagli scontri. Il ritmo percussivo raddoppia la linea melodica più grave. Sotto una soglia di idoneità un agente può tacere ([A28, FT parziale](https://arxiv.org/pdf/1907.01154)).
- FATE (Aspromallis e Gold) separa un motore di predizione da un motore musicale a grammatica stocastica ([A29, FT parziale](https://discovery.ucl.ac.uk/id/eprint/1505747/1/Aspromallis_Gold_CR_.pdf)).

### Inferences

- La divisione in due fasi (intento del gioco, esecuzione al confine autorizzato) è il confine naturale fra Pacing Director e direttore musicale: il primo chiede, il secondo sceglie quando e come.
- Total War: WARHAMMER 2 è il precedente più vicino a un direttore musicale: un modulo distinto da gameplay e contenuti, con coda, validità, cooldown e memoria dello stato armonico.
- La separazione di Left 4 Dead è per indipendenza: due processi paralleli con ingressi diversi. In CYMATICA il gameplay dipende dalla musica, quindi serve un contratto fra i due direttori, non due osservatori indipendenti.
- Nei titoli usciti un «ruolo» è uno stem o una parte da attivare. Fra le fonti lette, solo A28 coordina ruoli con una semantica musicale, e non è un prodotto commerciale.

### Gaps

- **DOOM (2016):** nessuna descrizione primaria scritta trovata. Si ha solo l'abstract del talk di Gordon ([A42, AM](https://gdcvault.com/play/1024068/-DOOM-Behind-the)) e l'analisi di un recensore che attribuisce la scelta dei frammenti al codice del gioco, senza citare id Software ([A43, FT\*](https://everythingisnoise.net/features/sound-test-doom-2016/)). Una mappa stati-musica non può essere affermata.
- **Left 4 Dead:** il commento è letto in trascrizione di fan, non nel gioco. Non è documentato quali variabili legga il Music Director né se condivida dati con l'AI Director.
- **Hellsinger:** nella parte letta il brevetto non è collegato al gioco in modo esplicito; il legame è mia deduzione da assegnatario, inventori e coincidenza dei tratti. Il prodotto uscito può differire.
- **Hades 1:** non è verificato che usi lo stesso meccanismo `SetMusicSection` descritto per Hades II.
- **Thumper e Sayonara Wild Hearts:** vedi sezione 3.

## 2. A quali scale temporali si decide, e quali decisioni aspettano i confini musicali?

### Takeaway

Le fonti mostrano tre bande: controllo a 20–30 ms, che non è musicale; commit musicale fra beat e 8 battute; forma da sezione a run. Le decisioni strutturali aspettano sempre un confine, salvo eccezioni dichiarate e mascherate. Il pacing di gioco vive su decine di secondi, quindi più lento di una battuta e di solito paragonabile a una frase o a una sezione.

### Cited Findings

| Scala | Decisione | Sistema e fonte |
|---|---|---|
| 5, 10 o 20 ms | periodo del mixer, secondo piattaforma | FMOD ([A21, FT](https://www.fmod.com/docs/2.03/api/studio-api-threads.html)) |
| 20 ms | elaborazione Studio asincrona | FMOD ([A21, FT](https://www.fmod.com/docs/2.03/api/studio-api-threads.html)) |
| 30 ms | aggiornamento del grafo di contesto | Hutchings ([A28, FT parziale](https://arxiv.org/pdf/1907.01154)) |
| Ogni frame | scarto dell'input dal beat | Hellsinger ([A11, FT\*](https://patents.google.com/patent/US11358065)) |
| Nota | colore e traiettoria dei proiettili | AudioInSpace ([A27, FT parziale](https://www.antoniosliapis.com/papers/audio_in_space.pdf)) |
| Beat | animazioni e movimento dei nemici, durate in beat | Hellsinger ([A11, FT\*](https://patents.google.com/patent/US11358065)) |
| Battuta | ingresso e uscita di parti; avanzamento di sezione | iMUSE ([A1, FT\*](https://patents.google.com/patent/US5315057)); Hades ([A13, FT\*](https://gameplay.co/hades-game-music-sound-design-darren-korb-supergiant-games/)) |
| 1–3 s + 2 s | attesa e crossfade di tier | Hellsinger ([A11, FT\*](https://patents.google.com/patent/US11358065)) |
| 2 battute | orizzonte di generazione degli agenti | Hutchings ([A28, FT parziale](https://arxiv.org/pdf/1907.01154)) |
| 4 battute | soglia di ETA per avviare la cadenza; spawn di nemici | FATE ([A29, FT parziale](https://discovery.ucl.ac.uk/id/eprint/1505747/1/Aspromallis_Gold_CR_.pdf)); Audioverdrive ([A26, FT](https://www.kmjn.org/publications/Audioverdrive_ICMC13.pdf)) |
| 8 battute | blocco di ingresso e uscita | Total War ([A9, FT](https://www.audiokinetic.com/en/community/blog/total-war-warhammer-2-how-we-construct-and-systemically-use-our-music/)) |
| 12 battute | decisioni di alto livello alle battute 1, 5, 7, 9, 11 | FATE ([A29, FT parziale](https://discovery.ucl.ac.uk/id/eprint/1505747/1/Aspromallis_Gold_CR_.pdf)) |
| 32 battute | fasi di sopravvivenza | Audioverdrive ([A26, FT](https://www.kmjn.org/publications/Audioverdrive_ICMC13.pdf)) |
| 3–5 s; 30–45 s | Sustain Peak; Relax | Left 4 Dead, lato gioco ([A2, FT parziale](https://steamcdn-a.akamaihd.net/apps/valve/2009/ai_systems_of_l4d_mike_booth.pdf)) |
| 90–180 s | intervallo casuale dei mob, difficoltà Normal | Left 4 Dead, lato gioco ([A2, FT parziale](https://steamcdn-a.akamaihd.net/apps/valve/2009/ai_systems_of_l4d_mike_booth.pdf)) |
| 1–2 min | atto come loop | Hellsinger ([A11, FT\*](https://patents.google.com/patent/US11358065)) |
| Bioma, run | playlist per bioma; intro fissa a ogni run | Hades ([A13, FT\*](https://gameplay.co/hades-game-music-sound-design-darren-korb-supergiant-games/)) |

- **Peak Fade aspetta una pausa.** Anche il lato gioco ha un suo confine: la fase non cede a Relax finché non c'è una pausa naturale nell'azione ([A2, FT parziale](https://steamcdn-a.akamaihd.net/apps/valve/2009/ai_systems_of_l4d_mike_booth.pdf)).
- **Aspettano un confine:** cambio di cue in Total War ([A9, FT](https://www.audiokinetic.com/en/community/blog/total-war-warhammer-2-how-we-construct-and-systemically-use-our-music/)), atto in Hellsinger ([A11, FT\*](https://patents.google.com/patent/US11358065)), sezione in Hades II ([A14, FT\*](https://sgg-modding.github.io/Hades2ModWiki/docs/creating-mods/audio/adding-dynamic-music)), hook e marker in iMUSE ([A1, FT\*](https://patents.google.com/patent/US5315057)).
- **Non aspettano:** i cambi verticali sincronizzati. Hell Is Us passa fra versioni integrali dello stesso brano con uscita `Immediate` e `Same Time as Playing Segment`, con crossfade di 10 secondi; serve materiale esportato con precisione al campione ([A10, FT](https://www.audiokinetic.com/en/community/blog/from-constraint-to-creativity-the-music-design-of-hell-is-us/)). Lo stesso vale per i salti diretti di iMUSE ([A1, FT\*](https://patents.google.com/patent/US5315057)).
- **Metrica irregolare:** Hell Is Us usa `Next Custom Cue` perché con metri irregolari la battuta non basta a identificare la fine di frase ([A10, FT](https://www.audiokinetic.com/en/community/blog/from-constraint-to-creativity-the-music-design-of-hell-is-us/)).
- **Uscita anticipata:** la musica di marcia di Total War ha due uscite. Un marker `Out` precedente permette di entrare nell'azione sul battere del culmine; altrimenti il pezzo chiude e torna a Threat ([A9, FT](https://www.audiokinetic.com/en/community/blog/total-war-warhammer-2-how-we-construct-and-systemically-use-our-music/)).
- **Quantizzazione e probabilità:** in FMOD una condizione di probabilità su regione quantizzata si valuta a ogni punto di quantizzazione ([A22, FT](https://www.fmod.com/docs/2.03/studio/authoring-events.html)).

### Inferences

- Stime mie, in 4/4: un blocco di 8 battute a 60–75 BPM dura 25,6–32 s, cioè l'ordine del Relax di Left 4 Dead. Il pacing di campagna e il blocco musicale possono allinearsi; le durate della fonte non indicano però che sia stato fatto apposta.
- Il 20–30 ms non va confuso con una decisione musicale. In CYMATICA il Pacing Director può aggiornarsi a tick, ma il direttore musicale dovrebbe leggerne l'output solo ai confini del piano. È una proposta, non una decisione accettata.
- Nessun sistema reattivo letto possiede un piano esplicito a livello di run. Quelli con forma lunga la ottengono da materiale d'autore (Hades, Audioverdrive, Sayonara Wild Hearts).

### Gaps

- Nessuna fonte misura la latenza percepita fra evento e cambio musicale.
- Nessuna fonte riporta quanto spesso il Music Director di Left 4 Dead valuti le regole.
- Total War non indica se i timer di validità siano in secondi o in tempo musicale.

## 3. Il segnale gioco-musica è piccolo o ricco? La musica può guidare il gameplay, e chi ha l'autorità?

### Takeaway

Al confine di esecuzione i titoli usciti usano pochi valori enumerati (3 comandi in Halo, una sezione in Hades II, 5 tier in Hellsinger), ma a monte possono calcolarli da eventi ricchi: Total War elenca 14 situazioni dinamiche come esempi, più priorità e validità. La musica guida il gameplay in tre modi: come clock, come sorgente di eventi, come partitura del livello. L'autorità strutturale resta quasi sempre al gioco; solo Audioverdrive la cede davvero.

### Cited Findings

**Larghezza del segnale**

- Halo: tre comandi per i level designer ([A7, FT\*](https://halo.bungie.org/misc/gdc.2002.music)).
- Hades II: il parametro discreto `Section`, stesso meccanismo degli stem ([A14, FT\*](https://sgg-modding.github.io/Hades2ModWiki/docs/creating-mods/audio/adding-dynamic-music)).
- Hellsinger: un misuratore di tier con soglie di moltiplicatore 1x, 2x, 4x, 8x, 16x, più trigger d'atto ([A11, FT\*](https://patents.google.com/patent/US11358065)).
- Dead Space: un valore continuo e spaziale. Gli emettitori di paura sono piazzati nel mondo o legati ai nemici, e guidano anche ambiente, riverbero e mix ([A8, FT\*](https://www.originalsoundversion.com/dead-space-sound-design-in-space-no-one-can-hear-interns-scream-they-are-dead-interview/)).
- Total War: eventi tipizzati e 14 esempi di stato dinamico; la validità varia per evento (carica breve, morte di un eroe lunga, perché l'umore persiste) ([A9, FT](https://www.audiokinetic.com/en/community/blog/total-war-warhammer-2-how-we-construct-and-systemically-use-our-music/)).
- Hutchings: il segnale più ricco, un grafo di oggetti, ambienti e sei categorie affettive inviato via OSC ([A28, FT parziale](https://arxiv.org/pdf/1907.01154)).
- FATE: un'interfaccia di forma con ETA, avvicinamento e arrivo, ridotta ai segnali `ENDING`, `RECOVERY` e `GOAL_REACHED`, quest'ultimo irreversibile ([A29, FT parziale](https://discovery.ucl.ac.uk/id/eprint/1505747/1/Aspromallis_Gold_CR_.pdf)).
- PreGLAM stima valenza, arousal e tensione di uno spettatore e cerca di prevedere gli eventi futuri ([A33, AM](https://doi.org/10.1109/TG.2023.3287732)); la pagina del progetto lo descrive come modello appraisal che reagisce a eventi di gioco ([A33b, FT\*](https://www.metacreation.net/projects/preglam-the-predictive-gameplay-based-layered-affect-model)).
- Holtar e colleghi (2013) indicano le transizioni di stato discrete, introdotte da iMUSE, come modalità ancora dominante ([A26, FT](https://www.kmjn.org/publications/Audioverdrive_ICMC13.pdf)).

**La musica come clock**

- Hellsinger non analizza l'audio: calcola i beat dal tempo fisso e dalla posizione in campioni. La finestra tipica è 0,3 s (0,4 e 0,2 secondo difficoltà), e si sposta per compensare la latenza misurata. Gli errori fuori tempo non puniscono direttamente, ma non riempiono il misuratore ([A11, FT\*](https://patents.google.com/patent/US11358065)).
- Conflitto fra fonti: nell'intervista a Goldfarb sparare fuori tempo peggiora la prestazione ([A12, FT\*](https://gamedeveloper.com/design/shredding-for-satan-how-i-metal-hellsinger-i-designed-fps-harmonies-in-hell)), mentre il brevetto dice che non è penalizzato direttamente ([A11, FT\*](https://patents.google.com/patent/US11358065)). Il brevetto può non coincidere col gioco uscito.
- Un BPM più alto accelera tiro nemico, cadenza del giocatore e beatmatching ([A12, FT\*](https://gamedeveloper.com/design/shredding-for-satan-how-i-metal-hellsinger-i-designed-fps-harmonies-in-hell)). Per G6, G7 e G8 vedi il repository.

**La musica come sorgente di eventi**

- Audioverdrive mantiene separati i due sistemi ma li accoppia in entrambe le direzioni. I parametri sono float, continui o discreti, collegati via OSC da mappature JavaScript scritte dal composer ([A26, FT](https://www.kmjn.org/publications/Audioverdrive_ICMC13.pdf)).
- Kick e rullante fanno sparare i nemici, il clap li fa caricare, le note del pad definiscono l'altezza del terreno, un altro strumento genera i nemici a inizio periodo di quattro battute. Senza OSC il tasso di spawn è zero. Al composer sono esposte persino vittoria e sconfitta ([A26, FT](https://www.kmjn.org/publications/Audioverdrive_ICMC13.pdf)).
- La forma è esplicita: quattro fasi lineari in circa ABAB, in corrispondenza 1:1 con la progressione di gioco, con obiettivi che fanno avanzare la musica ([A26, FT](https://www.kmjn.org/publications/Audioverdrive_ICMC13.pdf)).
- AudioInSpace: colore e traiettoria dei proiettili si ricalcolano a ogni nota; una nuova nota parte quando un proiettile colpisce o quando finisce la nota corrente. È un proof of concept, senza valutazione con giocatori ([A27, FT parziale](https://www.antoniosliapis.com/papers/audio_in_space.pdf)).

**La musica come partitura del livello**

- Sayonara Wild Hearts: ogni livello è un brano, e un'animazione master definisce eventi, camera, cambi visivi e posizioni dei personaggi sulla strada ([A39, FT\*](https://www.simogo.com/sayonara-wild-hearts/)). La musica ha preceduto la direzione visiva ([A40, FT\*](https://developer.apple.com/news/?id=33kvkagk)). Composizione e livelli sarebbero stati iterati in parallelo ([A41, SEC](https://www.npr.org/2020/09/14/908365152/sayonara-wild-hearts-pop-music-lockdown-depression-recovery)).
- Thumper: gli autori hanno costruito uno strumento per scrivere insieme gameplay e audio; FMOD gestiva l'audio a runtime; le collisioni sono trattate come eventi musicali ([A36, FT\*](https://www.gamedeveloper.com/audio/road-to-the-igf-drool-s-i-thumper-i-)). Gibson definisce generativa la musica in gioco, senza spiegare come ([A38, FT\*](https://briangibson.bandcamp.com/album/thumper)).

**Autorità**

- In iMUSE la musica informa: il gioco può interrogare posizione, stato delle parti e coda, e adattare la propria attività. Nella parte letta non c'è callback, solo interrogazione ([A1, FT\*](https://patents.google.com/patent/US5315057)).
- In Hellsinger il clock viene dall'audio, ma il brevetto esclude tempo e musica come causa del cambio d'atto: decide il progresso del giocatore ([A11, FT\*](https://patents.google.com/patent/US11358065)).
- In Total War la musica decide solo quando il proprio stato viene aggiornato (marker), mai lo stato del gioco ([A9, FT](https://www.audiokinetic.com/en/community/blog/total-war-warhammer-2-how-we-construct-and-systemically-use-our-music/)).
- In Audioverdrive nessun livello è subordinato all'altro, per scelta dichiarata ([A26, FT](https://www.kmjn.org/publications/Audioverdrive_ICMC13.pdf)).

### Inferences

- Mie stime: una finestra di 0,3 s a 130 BPM copre circa il 65% del periodo di beat (0,46 s). Il giudizio di Hellsinger è quindi tollerante; il valore non è trasferibile a CYMATICA, dove il timing riguarda altro.
- La forma più difendibile per CYMATICA è un'interfaccia stretta ma non povera: intento enumerato, priorità, validità ed eventuale ETA. Un solo scalare di intensità non basta a esprimere la differenza fra «carica» e «lutto» che Total War modella con validità diverse.
- Conviene distinguere tre autorità: clock (musicale canonico), struttura (coordinatore del gioco, che accetta revisione del piano ed epoch) e sicurezza (validatore e guardia runtime, secondo AGENTS §3). L'esposizione di vittoria e sconfitta al composer, come in Audioverdrive, non è compatibile con quest'ultima.
- CYMATICA è già nel caso «musica come sorgente di eventi»; le mappature dirette di Audioverdrive mostrano però che il legame deve restare interpretabile dal giocatore.

### Gaps

- **Thumper:** nessun testo pubblico letto descrive il sequencer. Il talk GDC 2017 esiste solo come video e non è stato visionato.
- **Sayonara Wild Hearts:** nessuna fonte tecnica su come il tempo musicale governi l'animazione. Anche le recensioni trovate nei risultati di ricerca non concordano su quanto sia stretto il sincronismo.
- **Hellsinger:** motore audio non confermato; talk Nordic Game non recuperato (HTTP 404).
- Nessuna fonte letta descrive un sistema in cui la musica rifiuti una richiesta del gioco.

## 4. Quali design sono compatibili con determinismo e replay?

### Takeaway

I modelli a intento più commit su confine sono compatibili, purché confine, coda e casualità siano calcolati nel sim e registrati nella traccia. Sono invece inadatti come autorità i middleware, che decidono su thread propri legati al mix, e ogni feedback dall'audio reale al gioco. Le partiture d'autore sono deterministiche ma non adattive. G4 resta l'unico replay deterministico documentato di sistema musicale procedurale trovato.

### Cited Findings

- **Lockstep deterministico:** si trasmettono solo gli input. Stato iniziale, input e passo fisso devono dare lo stesso risultato al bit; ogni divergenza persiste e cresce; si suggerisce un checksum dello stato a ogni frame ([A35, FT\*](https://gafferongames.com/post/deterministic_lockstep/)).
- **FMOD asincrono:** i comandi Studio sono accodati e inviati a fine `Studio::System::update`; l'elaborazione avviene ogni 20 ms, innescata dal mixer. In modalità sincrona i comandi girano nell'update, ma il mix può intervenire in qualunque momento e Studio programma un blocco dopo ([A21, FT](https://www.fmod.com/docs/2.03/api/studio-api-threads.html)).
- **Casualità nei middleware:** `FMOD_ADVANCEDSETTINGS` ha un `randomSeed` per il generatore interno ([A23, FT parziale](https://www.fmod.com/docs/2.03/api/core-api-system.html)). Wwise espone `SetRandomSeed`, `SetOfflineRendering` e `SetOfflineRenderingFrameTime` ([A20, AM, sole firme](https://www.audiokinetic.com/en/public-library/2025.1.12_9287/?source=SDK&id=namespace_a_k_1_1_sound_engine.html)). Le pagine lette non garantiscono che un seed renda riproducibili le scelte musicali.
- **Funzionalità casuali:** FMOD applica condizioni di probabilità ai marker ([A22, FT](https://www.fmod.com/docs/2.03/studio/authoring-events.html)); Wwise ha `Random Cue` e `Random Custom Cue` come punti d'ingresso ([A16, FT parziale](https://www.audiokinetic.com/en/public-library/2025.1.12_9287/?source=Help&id=transitions_tab_music_objects)). Hellsinger usa un'attesa casuale di 1–3 s ([A11, FT\*](https://patents.google.com/patent/US11358065)).
- **Tempo reale nell'accademia:** Hutchings aggiorna ogni 30 ms e fa decadere le attivazioni per secondo ([A28, FT parziale](https://arxiv.org/pdf/1907.01154)). FATE lavora su una media mobile esponenziale della velocità, ma le produzioni della grammatica avvengono in tempo musicale ([A29, FT parziale](https://discovery.ucl.ac.uk/id/eprint/1505747/1/Aspromallis_Gold_CR_.pdf)).
- **Lato client:** il sistema musicale di Left 4 Dead è per giocatore e lato client ([A3, FT/SEC](https://left4dead.fandom.com/wiki/Developer_Commentary_(Left_4_Dead))).
- **Trasporto non deterministico:** Audioverdrive passa OSC su rete wireless fra iPad e computer ([A26, FT](https://www.kmjn.org/publications/Audioverdrive_ICMC13.pdf)).
- **Livelli d'autore:** l'animazione master di Sayonara Wild Hearts fissa ogni evento ([A39, FT\*](https://www.simogo.com/sayonara-wild-hearts/)).
- **Render offline:** Platinum avrebbe usato il rendering offline di Wwise con frame fisso di 1/60 s per un proprio strumento ([A44, SEC](https://www.audiokinetic.com/en/community/blog/music-render-offline-rendering-tool/)).

### Inferences

| Design | Giudizio | Motivo | Condizione per CYMATICA |
|---|---|---|---|
| Hook e trigger (iMUSE) | compatibile | la scelta dipende da posizione e condizioni | posizione dal tempo musicale canonico, non dall'audio reale |
| Coda con marker (Total War) | compatibile se riscritto | la coda è codice puro; i marker vengono dall'audio | marker derivati dal piano, validità in tick o battute |
| Tier con ritardo (Hellsinger) | parziale | ritardo casuale e controllo per frame | stream casuale isolato, giudizio in tick, decisione in traccia |
| Director paralleli (Left 4 Dead) | compatibile solo per decorazione | fuori dallo stato autoritativo | inadatto se la musica genera gameplay |
| Mix continuo (Dead Space, slew rate) | compatibile | funzione del sim | calcolo a passo fisso, nessuna autorità sugli eventi |
| Decisioni nel middleware | inadatto come autorità | thread e mix fuori dalla traccia | usare il motore solo come esecutore |
| Grafo a 30 ms (Hutchings) | non così com'è | tempo reale, decadimenti per secondo | riformulare in tick; il modello può restare, il tempo no |
| Note che guidano i proiettili (AudioInSpace) | da evitare così | la nota dipende dai colpi e cambia il pericolo | ogni pattern deve passare dal validatore |
| Partitura d'autore (Sayonara) | compatibile | tutto è fissato | nessuna adattività |
| Correzione del clock da audio (G7) | incompatibile | l'audio modifica il tempo autoritativo | in conflitto col passo fisso autoritativo di AGENTS §3 |

La stessa regola vale per tutte le righe: il seed da solo non riproduce una run adattiva. Ciò che riproduce è la traccia delle decisioni accettate, come già fissato in AGENTS §3.

### Gaps

- Nessun titolo letto documenta replay di musica adattiva, salvo G4.
- Non ho verificato se `randomSeed` o `SetRandomSeed` coprano le decisioni di transizione.
- Un riassunto di ricerca citava un thread della community su `SetRandomSeed` senza effetto apparente. Non ho potuto stabilire quale fosse, quindi non lo uso.

## 5. Quali fallimenti o limiti sono documentati?

### Takeaway

I problemi ricorrono: attesa del confine, oscillazione dello stato, esplosione delle regole, ripetizione, collisioni armoniche e di mix. Le soluzioni documentate sono quasi tutte vincoli compositivi o regole semplici, non maggiore intelligenza. L'avvertimento più netto viene da Left 4 Dead: più complessità produce più casualità, non più intenzione.

### Cited Findings

**Lag e momento sbagliato**

- Total War ha provato cue di transizione fra mix: creavano troppo ritardo e richiedevano troppo materiale. Ha adottato transizioni immediate coperte da un riser sul battere ([A9, FT](https://www.audiokinetic.com/en/community/blog/total-war-warhammer-2-how-we-construct-and-systemically-use-our-music/)).
- Hellsinger ha scartato sia il cambio al beat successivo (poteva cadere sul movimento sbagliato e sembrare un salto nel brano) sia quello a fine battuta (funzionava, ma arrivava inatteso): da qui il segmento di 1–4 battute ([A11, FT\*](https://patents.google.com/patent/US11358065)).
- Wwise esce per default a `Exit Cue` ([A16, FT parziale](https://www.audiokinetic.com/en/public-library/2025.1.12_9287/?source=Help&id=transitions_tab_music_objects)): senza regole specifiche, il ritardo è il comportamento predefinito.
- Un cambio istantaneo è tollerato quando è mascherato: in Total War coincide con cambio visivo, voce e un forte effetto sonoro ([A9, FT](https://www.audiokinetic.com/en/community/blog/total-war-warhammer-2-how-we-construct-and-systemically-use-our-music/)).
- Un cambio brusco può anche essere voluto: in The Wind Waker l'ingresso in combattimento è disgiunto in metro, altezza e volume, e questo segnala il pericolo ([A31, FT\*](https://mtosmt.org/issues/mto.19.25.3/mto.19.25.3.medina.gray.html)).
- Il modulo Wwise sulle transizioni usa una curva asimmetrica: entrata rapida in combattimento, uscita lenta ([A18, FT parziale](https://www.audiokinetic.com/en/courses/wwise201/?source=wwise201&id=smoothing_transition_decisions_configuring_an_rtpc_values_to_control_switch)).

**Oscillazione**

- Il corso Wwise cita il caso di un mostro incontrato e sconfitto in due secondi, con passaggio avanti e indietro fra esplorazione e combattimento. La cura è un parametro con slew rate ([A18, FT parziale](https://www.audiokinetic.com/en/courses/wwise201/?source=wwise201&id=smoothing_transition_decisions_configuring_an_rtpc_values_to_control_switch)).
- Total War usa timer di validità e cooldown ([A9, FT](https://www.audiokinetic.com/en/community/blog/total-war-warhammer-2-how-we-construct-and-systemically-use-our-music/)); Hellsinger un ritardo di 1–3 s ([A11, FT\*](https://patents.google.com/patent/US11358065)).
- FATE tratta il falso allarme: `RECOVERY` riporta la musica nella forma, anche molte volte, finché l'obiettivo non è raggiunto ([A29, FT parziale](https://discovery.ucl.ac.uk/id/eprint/1505747/1/Aspromallis_Gold_CR_.pdf)).

**Esplosione delle regole**

- Total War ha 121 regole fra brani d'azione e marker, con 3 o più varianti di cue per cultura e radice ([A9, FT](https://www.audiokinetic.com/en/community/blog/total-war-warhammer-2-how-we-construct-and-systemically-use-our-music/)).
- La matrice Wwise deve coprire ogni coppia sorgente-destinazione, gestita come lista di eccezioni ordinata ([A19, FT\*](http://gamesounddesign.com/making-interactive-music-for-games-part-three.html)).
- Hell Is Us ha dovuto creare un altro insieme di regole per il giocatore che torna indietro ([A10, FT](https://www.audiokinetic.com/en/community/blog/from-constraint-to-creativity-the-music-design-of-hell-is-us/)).
- Total War ha dovuto aggiungere al proprio Wwise un filtro di stato per avere tre regole sulla stessa coppia, poi integrato nelle versioni successive ([A9, FT](https://www.audiokinetic.com/en/community/blog/total-war-warhammer-2-how-we-construct-and-systemically-use-our-music/)).

**Sovra-progettazione e perdita di senso**

- Morasky mirava alla «planned serendipity»: regole che alzino la probabilità di felici coincidenze e abbassino quella di errori. Regole troppo progettate tolgono la sorpresa; troppa complessità produce casualità e momenti scadenti ([A3, FT/SEC](https://left4dead.fandom.com/wiki/Developer_Commentary_(Left_4_Dead))).
- Audioverdrive: lo spazio aperto di mappature non dà struttura; serve un metodo sopra il framework, e mappature complesse rischiano di diventare illeggibili per il giocatore. Alcune idee di gioco vanno scartate per l'effetto musicale, e viceversa ([A26, FT](https://www.kmjn.org/publications/Audioverdrive_ICMC13.pdf)).
- Hutchings: la qualità musicale resta sotto quella di un compositore esperto, per ammissione degli autori. Il riconoscimento dei concetti associati è stato inferiore alla colonna originale: Zelda 53% contro 70%, StarCraft 41% contro 59% ([A28, FT parziale](https://arxiv.org/pdf/1907.01154)).
- **Precisazione su G19.** Con 17 partecipanti per gruppo, il confronto fra gruppi non è significativo a 0,05: immersione p = 0,121 e 0,053; correlazione p = 0,057 e 0,055. L'effetto è piccolo. Il test di Wilcoxon è significativo sull'effetto percepito della musica ([A28, FT parziale](https://arxiv.org/pdf/1907.01154)). La formula «immersione superiore» di G19, nota solo da AM, va quindi qualificata.

**Ripetizione, armonia, mix**

- Halo evita la saturazione con la scarsità: oltre 80 minuti su 20–70 ore e molto silenzio, più loop pesati ([A7, FT\*](https://halo.bungie.org/misc/gdc.2002.music)).
- Total War etichetta le parti di brano già usate per non riproporre lo stesso materiale negli stati dinamici ([A9, FT](https://www.audiokinetic.com/en/community/blog/total-war-warhammer-2-how-we-construct-and-systemically-use-our-music/)).
- Hades distribuisce circa 2,5 ore di musica su centinaia di ore possibili con strati e sezioni ([A13, FT\*](https://gameplay.co/hades-game-music-sound-design-darren-korb-supergiant-games/)).
- La compatibilità è scritta in partitura: radici limitate in Total War, scala unica in Left 4 Dead ([A9, FT](https://www.audiokinetic.com/en/community/blog/total-war-warhammer-2-how-we-construct-and-systemically-use-our-music/); [A3, FT/SEC](https://left4dead.fandom.com/wiki/Developer_Commentary_(Left_4_Dead))).
- Medina-Gray misura la «smoothness» di una giuntura separatamente per metro, timbro, altezza, volume e bruschezza, senza punteggio unico, anche in forma probabilistica su tutte le giunture possibili ([A31, FT\*](https://mtosmt.org/issues/mto.19.25.3/mto.19.25.3.medina.gray.html)).
- Le mix alternative di Total War tolgono percussioni gravi durante gli incantesimi per lasciare spazio agli effetti ([A9, FT](https://www.audiokinetic.com/en/community/blog/total-war-warhammer-2-how-we-construct-and-systemically-use-our-music/)). Hellsinger modula un beat pre-sincronizzato con fade di 8–10 ms invece di innescare un suono all'input, per mascherare la latenza ([A11, FT\*](https://patents.google.com/patent/US11358065)).

**Informazione e anticipazione**

- In Left 4 Dead i segnali musicali aiutano a giocare e, nei playtest, a distinguere gli eventi ([A3, FT/SEC](https://left4dead.fandom.com/wiki/Developer_Commentary_(Left_4_Dead))).
- Hell Is Us impone il contrario: la musica non deve suggerire nulla e cambia solo dopo che il giocatore ha assimilato l'informazione ([A10, FT](https://www.audiokinetic.com/en/community/blog/from-constraint-to-creativity-the-music-design-of-hell-is-us/)).
- Hutchings dichiara che il suo sistema non conosce gli eventi futuri e rimanda al foreshadowing ([A28, FT parziale](https://arxiv.org/pdf/1907.01154)). Nello studio di Scirea e colleghi (N = 30) il foreshadowing migliora la coerenza percepita, e quello falso aumenta il divertimento ([A32, AM](https://www.kmjn.org/publications/Foreshadowing_AM14-abstract.html)).

### Inferences

- CYMATICA dovrebbe confrontare prima regole semplici più vincoli di scrittura, e aggiungere semantica solo se misurabile. È coerente con il criterio di autenticità del progetto: una musica che ricorda, sviluppa, sorprende e ritorna, senza varietà fittizia.
- Il piano a orizzonte è un vantaggio reale sui sistemi reattivi: permette ETA, cadenze e foreshadowing, che Hutchings non può fare e FATE deve stimare. Il dato di Scirea è però solo AM e piccolo.
- L'equivalente deterministico di slew rate e cooldown è un'isteresi in battute o tick, registrata in traccia.
- Le giunture si prestano a un controllo per aspetti, come in Medina-Gray; la soluzione del riser di Total War suggerisce di testare anche transizioni coperte, non solo quantizzate.

### Gaps

- La tesi di Aspromallis (2020), con FATE preferito a un crossfade di riferimento, è nota solo da un riassunto di ricerca: URL non accertato, non usata come evidenza.
- La survey di Plut e Pasquier è solo AM; il divario accademia-industria nella tesi di Plut non è verificato.
- Nessuna fonte industriale pubblica dati di playtest sul lag musicale. Le soluzioni sono riferite come scelte di team.
- Red Dead Redemption (stem a 130 BPM in la minore) è noto solo da riassunti di ricerca; l'articolo CDM letto non contiene dettagli tecnici. Non usato come evidenza.
- Il sistema del remake di Dead Space (Intensity Director) è noto solo da riassunti di ricerca.

### Registro delle fonti

| ID | Fonte | URL o DOI | Verifica |
|---|---|---|---|
| A1 | Land, McConnell, brevetto US 5315057, 1994 | [Google Patents](https://patents.google.com/patent/US5315057) | FT\*, caratteri 0–200.000 di 240.053 |
| A2 | M. Booth, *The AI Systems of Left 4 Dead*, Valve, 2009 | [PDF](https://steamcdn-a.akamaihd.net/apps/valve/2009/ai_systems_of_l4d_mike_booth.pdf) | FT parziale: pacing e mob; resto solo ricerca per parole |
| A3 | Commento sviluppatori Left 4 Dead, trascrizione di fan | [Fandom](https://left4dead.fandom.com/wiki/Developer_Commentary_(Left_4_Dead)) | FT sui nodi musicali; SEC rispetto al gioco |
| A4 | Commento sviluppatori Left 4 Dead 2, trascrizione di fan | [Fandom](https://left4dead.fandom.com/wiki/Developer_Commentary_(Left_4_Dead_2)) | FT parziale, voci filtrate; SEC. Nessun dettaglio nuovo sul Music Director, non citata sopra |
| A5 | B. Dyck, intervista a Morasky, Original Sound Version, 2012 | [Link](https://www.originalsoundversion.com/one-musician-to-another-mike-morasky-interview/) | FT\* |
| A6 | «Is the music of Left4Dead procedural?», PCG Wiki, 2009 | [Link](https://pcg.wikidot.com/left4dead-music) | FT\*, fonte debole |
| A7 | M. O'Donnell, *Producing Audio for Halo*, GDC 2002 | [Link](https://halo.bungie.org/misc/gdc.2002.music) | FT\* |
| A8 | J. Napolitano, intervista a Veca su Dead Space, 2008 | [Link](https://www.originalsoundversion.com/dead-space-sound-design-in-space-no-one-can-hear-interns-scream-they-are-dead-interview/) | FT\* |
| A9 | J. Melham, R. Beddow, Total War: WARHAMMER 2, Audiokinetic, 2018 | [Link](https://www.audiokinetic.com/en/community/blog/total-war-warhammer-2-how-we-construct-and-systemically-use-our-music/) | FT |
| A10 | A. Vachon, Hell Is Us, Audiokinetic, 2026 | [Link](https://www.audiokinetic.com/en/community/blog/from-constraint-to-creativity-the-music-design-of-hell-is-us/) | FT |
| A11 | Goldfarb et al., brevetto US 11358065 B2, Funcom Oslo, 2022 | [Google Patents](https://patents.google.com/patent/US11358065) | FT\*, testo intero in due passaggi |
| A12 | J. Couture, intervista a Goldfarb, Game Developer, 2022 | [Link](https://gamedeveloper.com/design/shredding-for-satan-how-i-metal-hellsinger-i-designed-fps-harmonies-in-hell) | FT\* |
| A13 | J. Parkinson, *The Sound of Hades*, podcast Gameplay, 2020 | [Link](https://gameplay.co/hades-game-music-sound-design-darren-korb-supergiant-games/) | FT\* |
| A14 | SGG-Modding, guida alla musica dinamica di Hades II | [Link](https://sgg-modding.github.io/Hades2ModWiki/docs/creating-mods/audio/adding-dynamic-music) | FT\*, comunità |
| A15 | T. Caldas, Everything Is Noise, 2019 | [Link](https://everythingisnoise.net/?p=50582) | FT\*. Analisi dell'autore in accesso anticipato, superata da A13, non citata sopra |
| A16 | Audiokinetic, Wwise Help 2025.1.12, Transitions tab | [Link](https://www.audiokinetic.com/en/public-library/2025.1.12_9287/?source=Help&id=transitions_tab_music_objects) | FT parziale, opzioni sorgente e destinazione |
| A17 | Audiokinetic, Wwise Help 2025.1.12, Stingers tab | [Link](https://www.audiokinetic.com/en/public-library/2025.1.12_9287/?source=Help&id=stingers_tab_music_objects) | FT parziale |
| A18 | Audiokinetic, Wwise-201, modulo 16 | [Link](https://www.audiokinetic.com/en/courses/wwise201/?source=wwise201&id=smoothing_transition_decisions_configuring_an_rtpc_values_to_control_switch) | FT parziale, circa 4.500 di 6.647 caratteri |
| A19 | L.-X. Buffoni, *Making Interactive Music For Video Games Part 3*, senza data | [Link](http://gamesounddesign.com/making-interactive-music-for-games-part-three.html) | FT\*, terza parte |
| A20 | Audiokinetic, Wwise SDK, namespace SoundEngine | [Link](https://www.audiokinetic.com/en/public-library/2025.1.12_9287/?source=SDK&id=namespace_a_k_1_1_sound_engine.html) | AM, sole firme |
| A21 | Firelight, FMOD Engine 2.03, Studio API Threads | [Link](https://www.fmod.com/docs/2.03/api/studio-api-threads.html) | FT |
| A22 | Firelight, FMOD Studio 2.03, Authoring Events §4.11 | [Link](https://www.fmod.com/docs/2.03/studio/authoring-events.html) | FT, §4.11–4.11.3 |
| A23 | Firelight, FMOD Core API, System | [Link](https://www.fmod.com/docs/2.03/api/core-api-system.html) | FT parziale, voce `randomSeed` |
| A24 | Microsoft, DMUS_SEGF_FLAGS (archivio) | [Link](https://learn.microsoft.com/en-us/previous-versions/ms808244(v=msdn.10)) | FT\* |
| A25 | CRI Middleware, *Interactive Music with ADX2*, 2016 | [Link](https://blog.criware.com/index.php/2016/06/14/interactive-music-with-adx2/) | FT\* |
| A26 | Holtar, Nelson, Togelius, *Audioverdrive*, ICMC 2013 | [PDF](https://www.kmjn.org/publications/Audioverdrive_ICMC13.pdf) | FT |
| A27 | Hoover, Cachia, Liapis, Yannakakis, *AudioInSpace*, EvoMUSART 2015 | [PDF](https://www.antoniosliapis.com/papers/audio_in_space.pdf) | FT parziale, passi selezionati per parole chiave |
| A28 | Hutchings, McCormack, *Adaptive Music Composition for Games*, IEEE ToG 2020 | [arXiv](https://arxiv.org/pdf/1907.01154); [DOI](https://doi.org/10.1109/TG.2019.2921979) | FT parziale, sezioni VI–VIII. Estende G19 |
| A29 | Aspromallis, Gold, *Form-Aware, Real-Time Adaptive Music Generation*, SMC 2016 | [PDF](https://discovery.ucl.ac.uk/id/eprint/1505747/1/Aspromallis_Gold_CR_.pdf) | FT parziale, sezioni 1–3 e conclusioni |
| A30 | *Making of Red Dead Redemption*, Create Digital Music, 2010 | [Link](https://test.cdm.link/?p=12454) | FT\*; nessun dettaglio tecnico, non usata come evidenza |
| A31 | E. Medina-Gray, *Analyzing Modular Smoothness in Video Game Music*, MTO 25.3, 2019 | [Link](https://mtosmt.org/issues/mto.19.25.3/mto.19.25.3.medina.gray.html); [DOI](https://doi.org/10.30535/mto.25.3.2) | FT\*, caratteri 0–100.000 di 157.305 |
| A32 | Scirea, Cheong, Nelson, Bae, foreshadowing, Audio Mostly 2014 | [Abstract](https://www.kmjn.org/publications/Foreshadowing_AM14-abstract.html); [DOI](https://doi.org/10.1145/2636879.2636889) | AM |
| A33 | Plut, Pasquier, Ens, Bougueng Tchemeube, *PreGLAM*, IEEE ToG 2024 | [DOI](https://doi.org/10.1109/TG.2023.3287732) | AM |
| A33b | Metacreation Lab, pagina PreGLAM | [Link](https://www.metacreation.net/projects/preglam-the-predictive-gameplay-based-layered-affect-model) | FT\* |
| A34 | Plut, Pasquier, *Generative music in video games*, Entertainment Computing 33, 2020 | [Scheda](https://www.x-mol.com/paper/1307980426999205888) | AM, da riassunto di ricerca; DOI non verificato, non citata sopra |
| A35 | G. Fiedler, *Deterministic Lockstep*, 2014 | [Link](https://gafferongames.com/post/deterministic_lockstep/) | FT\* |
| A36 | *Road to the IGF: Drool's Thumper*, Game Developer, 2015 | [Link](https://www.gamedeveloper.com/audio/road-to-the-igf-drool-s-i-thumper-i-) | FT\* |
| A37 | L. LeRay, *Q&A: The rhythm violence of Thumper*, 2015 | [Link](https://gamedeveloper.com/audio/q-a-the-rhythm-violence-of-i-thumper-i-) | FT\*; conferma FMOD e motore proprio, non citata sopra |
| A38 | B. Gibson, pagina Bandcamp di Thumper | [Link](https://briangibson.bandcamp.com/album/thumper) | FT\*, una frase |
| A39 | Simogo, pagina di Sayonara Wild Hearts | [Link](https://www.simogo.com/sayonara-wild-hearts/) | FT\* |
| A40 | Apple, *Behind the Design: Sayonara Wild Hearts*, 2020 | [Link](https://developer.apple.com/news/?id=33kvkagk) | FT\* |
| A41 | NPR, articolo su Sayonara Wild Hearts, 2020 | [Link](https://www.npr.org/2020/09/14/908365152/sayonara-wild-hearts-pop-music-lockdown-depression-recovery) | SEC, solo riassunto di ricerca |
| A42 | M. Gordon, *'DOOM': Behind the Music*, GDC 2017 | [GDC Vault](https://gdcvault.com/play/1024068/-DOOM-Behind-the) | AM |
| A43 | D. Rodriguez, *Sound Test: DOOM (2016)*, 2018 | [Link](https://everythingisnoise.net/features/sound-test-doom-2016/) | FT\*, analisi dell'autore |
| A44 | PlatinumGames, *Music Render*, Audiokinetic | [Link](https://www.audiokinetic.com/en/community/blog/music-render-offline-rendering-tool/) | SEC, solo riassunto di ricerca |

### Pagine non lette o non recuperabili

- **Rip & Tear (studio su DOOM):** figshare ha risposto 403. [Link](https://figshare.com/articles/Rip_Tear_Deconstructing_the_Technological_and_Musical_Composition_of_Mick_Gordon_s_Score_for_DOOM_2016_/5526751)
- **Tesi di Plut (SFU):** il PDF è protetto da una pagina anti-bot Anubis. Non ho cercato di aggirarla. [Link](https://summit.sfu.ca/_flysystem/fedora/2022-11/etd21917.pdf)
- **NPR su Sayonara Wild Hearts:** due timeout da 60 s, quindi resta SEC.
- **Talk GDC di Gordon e di Flury:** nessun testo; video non visionati. Vault riservato ai membri per DOOM.
- **Talk Nordic Game su Hellsinger:** HTTP 404.
- **web.archive.org:** non raggiungibile con lo strumento di estrazione.
- **Parti non lette:** ultimi 40.053 caratteri di A1, ultimi 57.305 di A31, sezioni I–V di A28, resto di A2, A16, A17 e A29.
- **Pagine indicate nei risultati ma non aperte:** Game Informer e Destructoid su Dead Space, Fangoria sul remake.

**Modalità d'accesso.** Audiokinetic (WebFetch: 403), Fandom (WebFetch: 402) e documentazione FMOD (applicazione JavaScript, WebFetch vuoto) sono stati letti nel pannello browser dell'app, con il testo della pagina. A29 è stato scaricato con `curl` dal repository UCL dopo un 403 di WebFetch sull'host di staging; il paper è in accesso aperto. Nessuna modifica al repository: il file è sotto `artifacts/`, ignorato da git.
