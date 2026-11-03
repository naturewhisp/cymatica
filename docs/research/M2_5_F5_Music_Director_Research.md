# M2.5 — ricerca R-M25-7: un direttore sottile lega i quattro ruoli

- **Data:** 2026-11-04 (fonti consultate il 2026-11-03)
- **Origine:** richiesta del titolare del 2026-11-03: stato dell'arte per F5 ([ADR-0007](../adr/0007-m2-5-entry-decisions.md), R-M25-7), con la sua idea che il direttore garantisca la coerenza fra i quattro ruoli.
- **Status:** ricerca **non normativa**. Ogni indicazione è una proposta da decidere ai checkpoint; nessuna regola, soglia o peso è validato.
- **Materiale:** [note di ricerca](experiments/m25_f5_research_notes/README.md) e [script con output](experiments/m25_ensemble_baseline/README.md).
- **Verifica:** controllo a campione nel paragrafo [Verifica del dossier](#verifica-del-dossier-2026-11-04).

Il direttore musicale di CYMATICA dovrebbe essere uno strato sottile e deterministico, non un compositore: una funzione pura che a ogni confine autorizzato legge la richiesta del Pacing Director e lo stato della forma, poi scrive un contratto d'insieme che i quattro ruoli leggono mentre vengono generati. È una proposta da decidere ai checkpoint di [ADR-0007](../adr/0007-m2-5-entry-decisions.md), non una decisione. I sistemi documentati convergono su due fasi: il gioco esprime un intento, la logica musicale lo esegue al primo confine autorizzato. Quelli generativi convergono su un piano condiviso sottile con parti che leggono uno stato comune, ma nessuno studio letto confronta direttore e agenti, né ordini di generazione diversi, a parità di materiale. Per questo il dossier propone di generare i ruoli in sequenza con lettura, in ordine di dipendenza, con otto vincoli duri economici, preferenze versionate e mix solo offline. Lo sviluppo autentico si riconosce perché dipende dall'ordine: ciò che non cambia rimescolando le frasi non è sviluppo. Sulla ripetizione nessuna fonte dà una soglia in secondi, né una misura durante un gioco d'azione. Sul piano: una parte piccola del contratto in M2.5.3 e M2.5.4, se CP-2 e CP-3 la autorizzano; l'attuazione di §16.3–§16.4 a M6 come deliverable proprio, distinto dal CIE Director, con prova a M7.

## Le sei risposte e la forza dell'evidenza che le regge

| Domanda | Risposta proposta | Forza dell'evidenza | Dove si decide |
|---|---|---|---|
| 1. Chi dirige cosa, a quale scala | Il gioco chiede, il direttore sceglie quando e come. Contratto a frase, memoria e forma a sezione e run. Clock canonico, sicurezza al validatore | Descrittiva: brevetti, articoli di studio, documentazione. Nessuna misura di latenza percepita | CP-3, M6 |
| 2. Direttore o agenti | Piano condiviso sottile più ruoli che leggono uno stato comune. In un thread unico sono la stessa computazione | Nessun confronto controllato | CP-3 |
| 3. Sequenza o congiunta | Sequenza con lettura, in ordine di dipendenza. Candidati per ruolo e revisione limitata solo se un A/B li giustifica | Indiretta: due confronti su modelli appresi e un'ablazione. Nessuna prova d'ascolto sull'ordine | CP-3 |
| 4. Regole d'insieme | Otto vincoli duri, il resto preferenze versionate o controlli offline. Dissonanza e spostamento ammessi se dichiarati | Sperimentale per interferenza e poco altro. Il resto è corpus o convenzione | CP-2, CP-3 |
| 5. Sviluppo o varietà fittizia | Sviluppo è derivato, cumulativo, dipendente dall'ordine e ripreso in seguito. Si verifica con rimescolamento, fixture avverse, ablazione e prove fittizie d'ascolto | Proposta nostra. Nessuna metrica validata in letteratura | CP-3, M2.5.5 |
| 6. Dopo quanto pesa la ripetizione | Nessuna soglia pubblicata. L'esperto segnala il calo dopo il secondo loop. In gioco il dato manca | Laboratorio su unità separate, più uno studio in gioco senza condizione a bassa ripetizione | CP-1, M7 |

## Come leggere il dossier: tre piani, cinque livelli

Questo documento è ricerca non normativa per R-M25-7 e la questione F5. Non modifica specifica, ADR, contratti o golden. Ogni **Proposta** resta da decidere ai checkpoint di ADR-0007.

Ogni sezione separa tre piani. **Fonti** riporta ciò che le fonti lette affermano o misurano. **Misure nostre** riporta calcoli sul piano, sul codice o su modelli, eseguiti da noi. **Inferenze** riporta letture nostre. Un modello non è una prova percettiva: Sethares, LHL, E0 e CTnCTR descrivono spettri e simboli, non ciò che una persona sente. L'agente non ha ascoltato l'audio.

I livelli di verifica sono quelli delle note e non sono mai alzati. **FT**: testo letto direttamente. **FT-parz**: come FT, ma solo nelle parti citate. **FT\***: testo letto tramite estrattore automatico; nella nota sul coordinamento indica anche PDF estratti con `pdftotext`. **AM**: solo abstract o metadati. **SEC**: nota solo da fonte secondaria. Quando due note danno livelli diversi per la stessa fonte, il testo usa quello della nota che ha letto il dato e l'appendice A li riporta entrambi.

La base già presente è [`M2_4_Inc5_Entry_Research.md`](M2_4_Inc5_Entry_Research.md) §5 (G1–G24). Il confronto fra metodi compositivi di [`M2_4_Deep_Research_Reviewed.md`](M2_4_Deep_Research_Reviewed.md) non è ripetuto: qui la domanda è come si coordinano le parti, dato un metodo. Fonti consultate il 2026-11-03.

## Il gioco chiede, la musica sceglie il quando: due fasi da iMUSE a Hades II

**Fonti.** Lo schema dominante divide la decisione in due fasi. Il gioco esprime un intento; la logica musicale lo esegue al primo punto autorizzato, scelto in anticipo da chi compone.

| Sistema | Decide il gioco | Decide la logica musicale | Fissa i confini | Fonte |
|---|---|---|---|---|
| iMUSE | cosa cambiare, abilitando hook o accodando comandi; può anche saltare subito | esegue al prossimo punto di decisione | compositore | [Land e McConnell, brevetto, FT\*](https://patents.google.com/patent/US5315057) |
| Halo | `start`, `start_alt`, `stop`, volume, durata massima | sequenza in, loop, out; scelta pesata dei loop | sound designer | [O'Donnell 2002, FT\*](https://halo.bungie.org/misc/gdc.2002.music) |
| Left 4 Dead | AI Director: popolazione di minacce | Music Director separato: regole sull'ambiente vicino al giocatore | compositore e audio designer | [Commento sviluppatori, trascrizione di fan, FT/SEC](https://left4dead.fandom.com/wiki/Developer_Commentary_%28Left_4_Dead%29) |
| Total War: WARHAMMER 2 | eventi e condizioni | coda prioritaria in codice, regole Wwise | blocchi e marker | [Melham e Beddow 2018, FT](https://www.audiokinetic.com/en/community/blog/total-war-warhammer-2-how-we-construct-and-systemically-use-our-music/) |
| Metal: Hellsinger | prestazione, cambio d'atto per luogo o script | crossfade fra tier, attesa del punto di transizione | audio designer | [Goldfarb et al., brevetto, FT\*](https://patents.google.com/patent/US11358065) |
| Hades e Hades II | avanzamento di sezione via parametro | logica FMOD su regioni e quantizzazione | autore dell'evento FMOD | [Parkinson 2020, FT\*](https://gameplay.co/hades-game-music-sound-design-darren-korb-supergiant-games/); [guida di modding, FT\*](https://sgg-modding.github.io/Hades2ModWiki/docs/creating-mods/audio/adding-dynamic-music) |

Il legame fra il brevetto e il gioco Hellsinger è una deduzione della nota, e il prodotto uscito può differire. La guida di Hades II è della comunità, non di Supergiant.

Total War è il caso più vicino a un direttore musicale. Un modulo in codice mantiene una coda con priorità, validità e cooldown, legge lo stato solo ai marker del brano e traccia la radice corrente per scegliere la transizione ([Melham e Beddow 2018, FT](https://www.audiokinetic.com/en/community/blog/total-war-warhammer-2-how-we-construct-and-systemically-use-our-music/)). Left 4 Dead offre la separazione più esplicita fra due direttori: lavorano in parallelo con ingressi diversi. Nelle slide di Booth sull'AI Director la parola «music» non compare ([Booth 2009, FT-parz](https://steamcdn-a.akamaihd.net/apps/valve/2009/ai_systems_of_l4d_mike_booth.pdf)).

Il segnale è piccolo al confine di esecuzione, ricco a monte. Halo ha tre comandi, Hades II un parametro `Section`, Hellsinger cinque tier. Total War elenca invece 14 situazioni dinamiche, con validità diverse per evento: breve per una carica, lunga per la morte di un eroe, perché l'umore persiste ([FT](https://www.audiokinetic.com/en/community/blog/total-war-warhammer-2-how-we-construct-and-systemically-use-our-music/)).

L'autorità si divide. Hellsinger calcola i beat dal tempo fisso e dalla posizione in campioni, ma il cambio d'atto dipende dal progresso del giocatore ([FT\*](https://patents.google.com/patent/US11358065)). In Total War la musica decide solo quando aggiornare il proprio stato. Audioverdrive rinuncia alla subordinazione per scelta dichiarata, ed espone al composer persino vittoria e sconfitta ([Holtar et al. 2013, FT](https://www.kmjn.org/publications/Audioverdrive_ICMC13.pdf)). **Nessuna fonte letta descrive un sistema in cui la musica rifiuti una richiesta del gioco.**

I problemi ricorrono: ritardo, oscillazione, esplosione delle regole. Total War ha scartato i cue di transizione perché troppo lenti e usa transizioni immediate coperte da un riser; conta 121 regole fra brani d'azione e marker ([FT](https://www.audiokinetic.com/en/community/blog/total-war-warhammer-2-how-we-construct-and-systemically-use-our-music/)). Hellsinger ha scartato il cambio al beat e quello a fine battuta, e inserisce un segmento di 1–4 battute ([FT\*](https://patents.google.com/patent/US11358065)). Il corso Wwise cura il rimbalzo di stato con uno slew rate ([Wwise-201, FT-parz](https://www.audiokinetic.com/en/courses/wwise201/?source=wwise201&id=smoothing_transition_decisions_configuring_an_rtpc_values_to_control_switch)). L'avvertimento più netto è di Morasky: regole troppo progettate tolgono la sorpresa, e troppa complessità produce casualità e momenti scadenti ([FT/SEC](https://left4dead.fandom.com/wiki/Developer_Commentary_%28Left_4_Dead%29)).

Fra i sistemi accademici la separazione è esplicita. Hutchings e McCormack costruiscono un pacchetto separato che riceve messaggi dal gioco ([FT\*](https://arxiv.org/abs/1907.01154)). FATE separa un motore di predizione da un motore musicale a grammatica stocastica, e riduce l'interfaccia di forma a ETA e ai segnali `ENDING`, `RECOVERY` e `GOAL_REACHED`. `RECOVERY` riporta la musica nella forma dopo un falso allarme ([Aspromallis e Gold 2016, FT-parz](https://discovery.ucl.ac.uk/id/eprint/1505747/1/Aspromallis_Gold_CR_.pdf)).

Sulle scale temporali le fonti mostrano tre bande: controllo a 20–30 ms, che non è musicale; commit musicale fra beat e 8 battute; forma da sezione a run. Le decisioni strutturali aspettano un confine. Fanno eccezione i cambi verticali sincronizzati, come in [Hell Is Us, FT](https://www.audiokinetic.com/en/community/blog/from-constraint-to-creativity-the-music-design-of-hell-is-us/), e i cambi mascherati. Il lato gioco di Left 4 Dead lavora a 3–5 s (Sustain Peak), 30–45 s (Relax) e 90–180 s (mob a difficoltà Normal) ([Booth 2009, FT-parz](https://steamcdn-a.akamaihd.net/apps/valve/2009/ai_systems_of_l4d_mike_booth.pdf)).

Sul replay: il lockstep richiede stessi stato iniziale, input e passo fisso ([Fiedler 2014, FT\*](https://gafferongames.com/post/deterministic_lockstep/)). FMOD elabora i comandi Studio in modo asincrono ogni 20 ms, legato al mixer ([FMOD, FT](https://www.fmod.com/docs/2.03/api/studio-api-threads.html)). Mini Motorways (G4) resta l'unico replay deterministico documentato di un sistema musicale procedurale fra quelli trovati ([G4, FT\*](https://www.gamedeveloper.com/design/turning-road-noise-into-music-mini-motorways)).

**Misure nostre.** Scale temporali della fixture e confronto con le bande delle fonti. Le durate sono calcoli in 4/4 a 120 e 140 BPM.

| Banda | Valori nelle fonti | Durata in CYMATICA | Proposta |
|---|---|---|---|
| Controllo, non musicale | 20 ms FMOD; 30 ms [Hutchings, FT\*](https://arxiv.org/abs/1907.01154) | tick di simulazione | il Pacing aggiorna, il direttore non legge |
| Commit | battuta; 4 battute in [FATE, FT-parz](https://discovery.ucl.ac.uk/id/eprint/1505747/1/Aspromallis_Gold_CR_.pdf) | frase di 4 battute: 8 s e 6,9 s | contratto d'insieme |
| Iperfrase | 8 battute in Total War | 8 battute: 16 s e 13,7 s; 16 battute: 32 s e 27,4 s | maschera dei ruoli, ciclo metrico, debito di contrasto |
| Sezione | 32 battute in Audioverdrive; atti da 1–2 minuti in Hellsinger | 32 battute: 64 s e 54,9 s | area armonica, archetipo, tempo |
| Run | playlist per bioma in Hades | 10 minuti: circa 9,4 sezioni da 64 s | grammatica con memoria |

Un Relax di Left 4 Dead equivale a circa 15–22 battute a 120 BPM. La scala è simile a quella di una sezione breve, ma nessuna fonte dice che un allineamento del genere sia voluto.

**Inferenze.** La separazione di Left 4 Dead è per indipendenza. In CYMATICA il gameplay dipende dalla musica, quindi serve un contratto fra i due direttori, non due osservatori. Un solo scalare di intensità non esprime la differenza fra carica e lutto. Nessun sistema reattivo letto ha un piano esplicito a livello di run: chi ha forma lunga la ottiene da materiale d'autore. Il piano a orizzonte è perciò un vantaggio reale di CYMATICA, perché consente ETA e cadenze preparate. Conviene distinguere tre autorità: clock (trasporto musicale canonico), struttura (coordinatore, che accetta revisione del piano ed epoch) e sicurezza (validatore e guardia runtime). Le decisioni nel middleware restano fuori dalla traccia, quindi sono inadatte come autorità. La correzione del clock dall'audio, come in [BPM (G7), FT](https://www.audiokinetic.com/en/blog/syncing-music-and-gameplay-in-bpm), confligge con il passo fisso autoritativo di AGENTS §3. Coda e marker sono compatibili se riscritti su tick e battute del piano.

**Proposta (CP-3, M6).** Il direttore non è un blocco nuovo. La [specifica §16.2](../../CYMATICA_Specifica_Agentica_Sviluppo.md) ha già Run Form Planner, Section/Phrase Planner e Four-Role Orchestrator: il direttore coincide con i primi due, più il contratto consegnato al terzo. La specifica fornisce anche le interfacce. Il §9.2 usa l'energia musicale prevista nelle condizioni di transizione, il §9.4 separa `musicalTension` e `gameplayPressure`, il §16.10 elenca le regioni mutabili.

| Direzione | Contenuto proposto | Base |
|---|---|---|
| Pacing verso direttore | Intento enumerato, priorità, validità in frasi, ETA facoltativa. Tocca solo le regioni mutabili del §16.10 | Total War, FATE, specifica §16.10 |
| Direttore verso Pacing | Previsione di energia e tensione per le prossime frasi; punti di recovery e climax consentiti | specifica §9.2, §9.4, §16.3 |
| Direttore verso ruoli | Contratto d'insieme di frase: stato metrico e àncora, dominante e supporti, maschera attiva, bande di densità e tensione, modo armonico, operazione sul motivo con genitore, budget di novità | spec. §16.4, §16.7; DESIGN §8.5; sezioni seguenti |
| Direttore verso traccia | Ragione della scelta, alternative scartate, richieste scadute, budget esauriti | AGENTS §3; specifica M6, explanation trace |

«Subordinato» significa, in questa proposta, che il direttore può differire una richiesta entro la sua validità ma non può rifiutarla oltre il limite in frasi del recovery debt (§9.3), e non scavalca mai il validatore. Una richiesta scaduta è una decisione registrata. Questa regola non ha precedenti nelle fonti lette. Il lavoro del direttore è fisso per confine: stato a capacità fissa, valori interi, scelte da tabelle chiuse, flusso casuale isolato. Il replay consuma le decisioni accettate, perché il seed da solo non riproduce una run adattiva (AGENTS §3). Il contratto resta interno al planner, perché la specifica §16.12 non ammette nuovi identificatori pubblici in `MusicIntentEvent` senza revisione di D-M2-02.

## Direttore o agenti è una falsa alternativa in un planner a thread singolo

**Fonti.** Nei sistemi che coordinano parti generate dallo stesso programma, l'«ascolto» è quasi sempre simbolico. L'audio serve per l'ingresso di un musicista o per il timbro.

| Sistema | Chi coordina | Come «ascoltano» le parti | Valutazione | Fonte |
|---|---|---|---|---|
| Hutchings e McCormack | Agente armonia e agenti melodia; guida dinamica per confidenza | Matrice 12 × suddivisioni; ogni nota consuma risorse | 34 persone; non isola il coordinamento | [FT\*; FT-parz nell'altra nota](https://arxiv.org/abs/1907.01154) |
| Kinetic Engine v1–v3 | Da direttore pieno a direttore ridotto, poi intenzioni trasmesse | Messaggi con riscontro | Artistica, degli autori | [Eigenfeldt e Pasquier 2009, FT\*](http://www.sfu.ca/~eigenfel/A%20Realtime%20Generative%20Music%20System.pdf) |
| Coming Together | Negoziazione | Stato simbolico; audio per il timbro | Nessuno studio nel testo letto | [Eigenfeldt 2010, FT\*](http://www.sfu.ca/~eigenfel/ComingTogether_Beauty&Truth.pdf) |
| Musebot | Conductor come snodo OSC | Stati e intenzioni | Nessuno studio controllato nel testo letto | [Eigenfeldt 2016, FT\*](https://zenodo.org/records/4285186/files/Eigenfeldt_Musebots.pdf) |
| Swarm Music | Nessuno: stigmergia | Eventi MIDI come attrattori | Descrittiva | [Blackwell e Young 2004, FT\*](https://eprints-gro.gold.ac.uk/106/1/OSYoung04.pdf) |
| Reflexive Looper | Griglia di accordi condivisa | Nessun ascolto reciproco; audio solo del musicista | Nessuno studio controllato nel testo letto | [Marchini et al. 2017, FT\*](https://www.francoispachet.fr/wp-content/uploads/2021/01/marchini-17nime.pdf) |

La traiettoria di Kinetic Engine è istruttiva. La v1, gerarchica, è stata ritirata perché difficile da estendere. La v2 riduce il direttore a tempo, metro, impulso e valutazione sfumata della densità, ma a lungo andare risulta omogenea perché le regole sono statiche. La v3 trasmette intenzioni e perde reattività. Gli autori concludono proponendo un terzo livello simile al direttore della v1 ([FT\*](http://www.sfu.ca/~eigenfel/A%20Realtime%20Generative%20Music%20System.pdf)). È la riflessione di un compositore sul proprio lavoro, non un esperimento.

Hutchings è il sistema con la valutazione più vicina a un gioco, ma confronta il sistema con la colonna sonora originale, non due forme di coordinamento. Il survey di [Tatar e Pasquier 2019, AM](https://doi.org/10.1080/09298215.2018.1511736) censisce 78 sistemi, ma il testo integrale non è stato ottenuto, quindi nessuna sua conclusione è citabile.

**Misure nostre.** Verifica sul [codice](../../engine/audio/music_planner.cpp), righe 1014–1042. Trama salta gli step di Pulso e prende le altezze dalla voicing della battuta. Vettore legge solo il motivo e la scala. La frase «i ruoli non si sentono» descrive quindi il codice attuale, non un vincolo strutturale. La premessa di «accordo e scala condivisi» vale per Corpo e Trama; **Vettore condivide solo la scala**.

**Inferenze.** In un planner a thread singolo, direttore e agenti sono due modi di schedulare la stessa computazione. Un ruolo è una funzione, e ascoltare significa leggere eventi simbolici già prodotti, senza latenza. Il modello ricorrente non è né puro direttore né puro agente: un piano condiviso sottile con parti autonome. Lo si ritrova in Kinetic v2, Musebot, Reflexive Looper e Hutchings. Il limite del solo simbolico è il timbro: nulla dice sul mascheramento. Assegnare a ogni ruolo un profilo spettrale statico è un'ipotesi nostra, non un risultato pubblicato.

**Proposta (CP-3).** Il direttore pubblica globali e non giudica la musica. Decide il ruolo guida per dichiarazione, usando `dominantRole`, invece della confidenza di una rete neurale. Il silenzio di una parte è un esito legittimo, come in Hutchings, e libera voci nel budget di 8. Sottile riguarda l'accoppiamento, non la memoria: il direttore può conservare stato di dimensione fissa. La lezione di Kinetic v2 è che le regole devono cambiare con lo stato della forma, altrimenti il risultato si omogeneizza. Restano escluse la negoziazione asincrona, gli ensemble su rete e ogni modalità con analisi dell'audio.

## La sequenza con lettura costa 24 valutazioni contro 1296, ma va riparata

**Fonti.** Solo due studi confrontano direttamente progetti di coordinamento, entrambi su modelli appresi.

| Confronto | Risultato | Limite | Fonte |
|---|---|---|---|
| MuseGAN: generatori indipendenti, un generatore, ibrido | Distanza tonale fra tracce minore con intenzione condivisa. Armonia, ascoltatori pro: 2,31, 2,66, 2,92 su 5. Ritmo quasi invariato | Rock, modelli appresi, nessun test statistico nelle sezioni lette | [Dong et al. 2018, FT\*](https://arxiv.org/abs/1709.06298) |
| Coconet: revisione Gibbs contro passaggio ordinato | NLL 0,52 contro 0,74–1,09. 96 confronti a coppie, χ²(2) = 12,23, p < 0,001 | Bach. Gli autori lo attribuiscono a condizionali mal modellate | [Huang et al. 2017, FT\*](https://arxiv.org/abs/1903.07227) |
| BachBot: sequenziale puro | Discriminazione al 51%. Il completamento con parti date riesce peggio della composizione libera | Bach, ricerca greedy, ambiente web | [Liang et al. 2017, FT\*](https://archives.ismir.net/ismir2017/paper/000156.pdf) |
| Whole-Song: cascata di quattro livelli | 57 persone in doppio cieco: meglio sulla struttura, senza degrado | Cambiano anche i modelli | [Wang et al. 2024, FT\*](https://arxiv.org/abs/2405.09901) |
| MetaCompose: ablazione delle parti | Accompagnamento casuale, piacevolezza p = 5,00e-35. Melodia casuale in tonalità, p = 5,47e-2. 298 partecipanti | Tonale generico, parti sostituite dal caso, nessun confronto fra ordini | [Scirea et al. 2016, FT\*](https://researcher.itu.dk/ws/files/82418836/scirea_evomusart16.pdf) |
| Anders e Miranda: ordine nei solutori | Tre ordini di grandezza fra ordini diversi; l'ordine ottimale dipende dal problema | Efficienza, non ascolto; misure di Anders 2007 SEC | [Anders e Miranda 2011, FT\*](https://doi.org/10.1145/1978802.1978809) |

**Nessuno studio d'ascolto letto confronta un ordine di generazione con un altro.** Gli ordini dichiarati seguono il genere: armonia prima per corali e sistemi tonali, batteria prima in [GESMI, FT\*](http://www.sfu.ca/~eigenfel/Eigenfeldt%20ICCC%2013.pdf), melodia prima in [Song From PI, FT\*](https://arxiv.org/abs/1611.03477). Nei sistemi gerarchici la forma viene sempre prima. La soluzione dominante per i conflitti è filtrare con pochi vincoli duri e ordinare con preferenze, perché le regole musicali sono perlopiù linee guida ([Anders e Miranda 2011, FT\*](https://doi.org/10.1145/1978802.1978809)).

**Misure nostre.** Con 6 opzioni per ruolo il prodotto congiunto vale 6⁴ = 1296 combinazioni, una sequenza greedy ne valuta 24. Nel [codice](../../engine/audio/music_planner.cpp), righe 925–938, tutti i parametri di ruolo dipendono dallo stesso indice k: i 6 candidati sono 6 punti di un unico filo. Un motivo non si può abbinare al ritmo di Trama di un altro candidato. Anche la scelta fra `VoiceLed` e `RootFifth` di Corpo dipende dalla parità di k. Il costo reale sul planner non è stato misurato.

**Inferenze.** La regola che regge la tabella è informativa, non estetica: una parte va generata dopo quelle che deve leggere. Conviene separare due griglie. Sulla griglia temporale, GESMI va da batteria a basso a synth. Sulla griglia delle altezze, armonia e contesto vengono prima. Questa lettura concilia «armonia prima» e «ritmo prima», ma è nostra.

Il motivo di Vettore è fisso per identità, quindi funziona come un *cantus firmus*: si stabilisce prima e si adatta per ultimo. L'ablazione di MetaCompose suggerisce che Vettore solo-scala sia già accettabile su alcuni criteri, ma paghi su armonia e casualità percepita. L'estensione è prudente, perché quello studio sostituisce le parti con il caso. BachBot avverte invece che riempire in modo greedy attorno a una parte fissa può peggiorare: servono più candidati.

Il modello ibrido di MuseGAN è l'analogo più vicino all'architettura attuale: un vettore condiviso, il piano, più vettori privati, i ruoli. Incoraggia a mantenere ruoli separati con intenzione comune, ma non prova nulla su un planner a regole. Il vantaggio di Coconet nasce da condizionali deboli, quindi non si trasferisce come tale. Si trasferisce l'idea della riscrittura: un passaggio che corregge scelte già fatte.

Il risparmio combinatorio non basta da solo a escludere il congiunto. Anche 1296 è un budget logico fisso, e il planner lavora fuori dalla callback. La sequenza si preferisce per dipendenze e spiegabilità: ogni scelta ha una causa leggibile nella traccia. Il suo prezzo è la miopia, perché una scelta iniziale non si ripara: è ciò che una revisione limitata dovrebbe correggere. Un termine di punteggio verticale, da solo, non risolve: se nessun candidato è buono, il punteggio sceglie il meno peggio. Il coordinamento deve agire durante la generazione.

**Proposta (CP-3).** Ordine da verificare: forma, frase, armonia e identità del motivo, griglia di Pulso, Corpo, Trama, adattamento di Vettore, controllo verticale finale limitato. Coincide con l'ordine di [§16.2](../../CYMATICA_Specifica_Agentica_Sviluppo.md), che pone Harmony e Motif prima di Rhythm e Orchestrator. La tabella è una valutazione nostra: nessuna fonte misura il proprio sistema in termini di determinismo o replay.

| Progetto | Limite di lavoro | Valido entro il budget? | Collocazione proposta |
|---|---|---|---|
| A. Piano condiviso, ruoli indipendenti (attuale) | C × B × S | sì | baseline |
| B. Sequenziale con lettura, ordine fisso | C × somma dei ruoli | sì, col silenzio come ripiego | primo passo |
| D. Candidati per ruolo, fascio ridotto | L × K × R | sì | dopo B, se un A/B lo giustifica |
| C. Matrice condivisa a consumo, interi | 12 × S per frase, ricerca finita | sì, col silenzio come ripiego | candidato per M6 |
| G. Revisione a K passaggi fissi | K × R × C | sì, se riparte da una soluzione valida | facoltativo; nessuna fonte musicale la valuta |
| E. Programmazione dinamica sugli step | P × K² | sì, se ogni posizione ha un candidato | riserva |
| H, I, J. Backtracking, ricerca locale, evoluzione | budget fisso | no: serve un fallback | solo con fallback registrato |
| L, M, O. Negoziazione asincrona, rete, analisi audio | illimitato o vietato | no | escluse |

Legenda: C candidati, B battute, S step, L ampiezza del fascio, K candidati per ruolo, R ruoli, P posizioni.

Per il replay, ogni fase di ruolo dovrebbe avere il proprio slot in `deriveDecisionId` e il proprio spareggio. La traccia dovrebbe registrare la scelta per ruolo, non solo il candidato vincente. Un budget esaurito è una decisione registrata, non un effetto dell'orologio. Il parallelismo resta escluso. Un termine incrociato nel punteggio richiede una nuova versione esplicita del profilo, come chiede AGENTS §1.1. Target a banda, non a minimo: in MuseGAN i modelli migliori scendono sotto i dati reali in distanza tonale, e in [Yeh et al. 2021, FT-parz](https://arxiv.org/abs/2001.02360) i metodi più armonici risultano troppo conservativi.

## Otto vincoli duri candidati: il resto è punteggio o controllo offline

Le regole sono tutte proposte. Ogni soglia è un valore iniziale da versionare e validare. Base di evidenza: E sperimentale, C corpus, T teoria o modello, P pratica di mixaggio, A scelta d'autore.

| Area | Vincoli duri candidati | Preferenze | Offline | Base |
|---|---|---|---|---|
| Riferimento metrico | **R-MET-1** stato esplicito per frase: `ancorato`, `spostato`, `ritorno`. **R-MET-2** in `ancorato`, un ruolo-àncora articola il tempo 1 e la fase 0 è la migliore o pari. **R-MET-9** nessuna suddivisione sotto 100 ms | Punteggio di fase pesato; banda di sincope legata alla tensione; almeno un ruolo rigido; ciclo esposizione, spostamento, ritorno; sincope anticipatoria; unità vicina a 250 ms | — | T, A, E parziale, C |
| Dominante e supporto | **R-ROL-1** un dominante per frase, che guida la generazione | Almeno due segni di salienza; ruolo sopra il dominante solo a bassa densità melodica; massimo 3 flussi con contorno indipendente; ingressi in voce esterna; pieno organico solo nei punti forti; opportunità rara | — | A, E su voci omogenee |
| Consonanza | **R-HAR-1** nota dell'accordo sul tempo 1 al cambio d'accordo, salvo figura dichiarata. **R-HAR-2** nota estranea tenuta almeno un quarto: etichetta e risoluzione entro 2 semitoni | Banda di armonicità; costo di interferenza da tabella; riparazione a costo minimo; voice leading; stratificazione dichiarata; limite a unisoni e ottave | — | T, C, E indiretta |
| Registro e mascheramento | **R-REG-2** spaziatura minima nell'estremo grave | Bande e ordine di registro per funzione; collisioni fra Pulso e basso di Corpo | Mascheramento per coppia, pendenza spettrale, immagine stereo, livelli per stem | E, C, P |
| Timbro | **R-TIM-1** lista chiusa di rapporti FM per la Trama | Rapporto legato al contesto; indice e rapporto progettati insieme; tabella di interferenza offline e versionata | Distanza timbrica fra ruoli | T, misura nostra |

Spostamento metrico, dissonanza, melodia al basso e stratificazione non sono vietati. Sono ammessi quando dichiarati nello stato di frase. Questa è la differenza fra gestione della tensione e divieto della dissonanza che la [proposta M2.5 §8](M2_5_Entry_Music_Quality_Proposal.md) chiede di mantenere.

### Riferimento metrico: l'àncora può stare fuori dal kick, se è dichiarata

**Fonti.** In [Witek et al. 2014, FT\*](https://doi.org/10.1371/journal.pone.0094446) (66 partecipanti, 50 drum break) le risposte seguono una U rovesciata: R² = 0,347 per il desiderio di muoversi e 0,427 per il piacere. La sincope stava però solo in kick e snare, sopra un hi-hat costante. La dose non è trasferibile senza quel riferimento. [Butler 2001, FT\*](https://www.mtosmt.org/issues/mto.01.7.6/mto.01.7.6.butler_essay.html) descrive nella EDM il passaggio del ruolo metrico da un livello a un altro: un kick sui controtempi può far sentire il beat ribaltato. Con toni simultanei, l'anticipo del grave sposta il tapping più di quello dell'acuto, ma di poco: 14 contro 11 ms ([Hove et al. 2014, FT\*](https://doi.org/10.1073/pnas.1402039111)). Il metro può sostenersi internamente, anche senza energia alla frequenza del pulse ([Tal et al. 2017, AM](https://doi.org/10.1523/JNEUROSCI.2500-16.2017)). La sincope rock è soprattutto anticipatoria, con unità di spostamento ottimale di circa un quarto di secondo ([Tan et al. 2019, FT-parz](https://doi.org/10.1525/MP.2019.36.4.353)).

**Misure nostre.** LHL è ciclico, E0 conta i tick silenziosi di un clock di quarti per fase, da [Povel e Essens, SEC/AM, DOI non verificato](https://doi.org/10.2307/40285311). Griglia di 16 passi, fixture del seed 1337. La misura LHL segue [Smith e Honing 2006, FT](https://www.mcg.uva.nl/mcg-2023/papers/smith-honing-2006.pdf), dove la variabile validata è la complessità percepita, non il groove. A 140 BPM il sedicesimo vale 107 ms, quindi R-MET-9 fissa a 150 BPM il limite per i ruoli in sedicesimi.

| Pattern | LHL | E0, fasi 0/1/2/3 |
|---|---:|---|
| Pulso A1 `2,5,7,10,13,15` | 11 | 4/2/2/2 |
| Pulso B e A′ `2,6,10,14` | 7 | 4/4/0/4 |
| `2,6,10,14` più backbeat `4,12` | 5 | 2/4/0/4 |
| `2,6,10,14` più kick su `0` | 4 | 3/4/0/4 |
| Vettore `0,4,8,12`, tutte le sezioni | 0 | 0/4/4/4 |
| Kick `dritto` dell'esperto più backbeat | 2 | 0/2/4/2 |

**Inferenze.** `2,6,10,14` non è storto in sé: è un four-on-the-floor sfasato di un ottavo, con clock perfetto in fase 2. Con Vettore sui quarti le fasi 0 e 2 pareggiano, e a rompere la parità sono il cambio d'accordo sul passo 0 e il contorno. Questo spiega perché il metro regge oggi, e perché alzare il kick può ribaltarlo. **Nel modello a soli onset il backbeat da solo non riporta la fase 0.** La proposta §3ter dell'esperto non è quindi trasferibile senza prova: il suo kick `storto` contiene già i passi 4 e 12, quello del generatore no. Se funziona, funziona per schema appreso e timbro distinto, che E0 non vede. La disposizione di CYMATICA, con lead rigido e kick sincopato, inverte quella del rock analizzato: nessuna fonte la vieta, ma va scelta.

### Consonanza: il problema non è la nota estranea, ma chi la decide

**Fonti.** La consonanza è composita: interferenza, armonicità e familiarità predicono i dati di Bowling con r = 0,88 ([Harrison e Pearce 2020, FT\*, DOI non verificato](https://doi.org/10.1037/rev0000169)). La massima dissonanza tonale cade a circa il 25% della banda critica ([Plomp e Levelt 1965, FT-parz](https://doi.org/10.1121/1.1909741)). Su musica umana, [Yeh et al. 2021, FT-parz](https://arxiv.org/abs/2001.02360) riporta CTnCTR 0,74 e PCS 1,42. Nel rock la melodia può seguire un quadro proprio: Nobile distingue fra l'altro un *hierarchy divorce*, in cui la melodia resta orientata sulla tonica e gli accordi cambiano a un livello meno profondo ([Reed 2022, FT\*, Nobile SEC](https://mtosmt.org/ojs/index.php/mto/article/view/876/194)). Il principio di ancoraggio melodico di Bharucha è noto solo da estratti ([SEC/AM, DOI non verificato](https://doi.org/10.1016/0010-0285%2884%2990018-5)).

**Misure nostre.** Vettore rispetto alle triadi inferite dalla fondamentale di Corpo: Dm–G–Am–Dm in A e A′, Gm–E♭–F–Gm in B. I valori sono ricalcolati da noi sul [CSV della fixture](experiments/cacp_methods/fixture_intents_seed1337.csv). Coincidono con le note entro 0,02, salvo la riga B (vedi correzioni).

| Sezione | Note d'accordo, tutte | Tempi 1 e 3 | Tempi 2 e 4 | CTnCTR | PCS |
|---|---:|---:|---:|---:|---:|
| A | 0,56 | 0,75 | 0,38 | 0,63 | 1,06 |
| B | 0,50 | **0,50** | 0,50 | 0,81 | 1,19 |
| A′ | 0,56 | 0,75 | 0,38 | 0,63 | 1,00 |
| Riferimento umano, Yeh | — | — | — | 0,74 | 1,42 |

Il confronto con Yeh è indicativo: accordi per battuta, dataset pop diverso. Su tempi 2 e 4, in A e A′ quasi nessuna nota estranea prosegue per grado (CTnCTR 0,39 e 0,38 contro un tasso di note d'accordo di 0,38). In B molte sì (0,75 contro 0,50). Il tritono Fa contro Si dura 8,0 s e la settima maggiore Mib contro Re 6,9 s, in [`role_clashes`](experiments/m25_ensemble_baseline/role_clashes.output.txt). Valgono 3,1% e 6,2% del tempo di sovrapposizione Corpo–Vettore, e circa 6% e 13% sui soli tempi 2 e 4.

**Una hard rule non è neutra.** Vettore non è nota dell'accordo su **8 tempi 1 con cambio d'accordo su 25, in ciascuna sezione**: su G e Am in A, su F in B, su Am in A′. Le figure `A F D E`, `D F A G` e `G D C D` proseguono tutte per salto, quindi non risolvono entro 2 semitoni. In `clash_bars` Corpo usa già fondamentale e quinta in B, mentre in A suona G–B e A–C.

**Inferenze.** Il tritono è un problema di diade, non solo di melodia: con G + D in Corpo sparirebbe senza toccare il motivo. Con Mib + Sib il Re di Vettore resta settima maggiore, e lì serve adattare il grado o dichiarare la figura. L'ostinato non è illegittimo. Una lettura da *hierarchy divorce* è possibile se Vettore resta orientato sulla tonica. Oggi però Corpo mette la terza in alto proprio dove la melodia la contraddice. R-HAR-1 e R-HAR-2 costringono a una scelta che sta sopra la frase: o il motivo cede, o Corpo cede, o si dichiara la stratificazione. È esattamente il tipo di decisione che spetta al direttore. Il [README delle misure](experiments/m25_ensemble_baseline/README.md) ricorda che nessuna di queste dissonanze è stata segnalata dall'esperto: la motivazione è l'indicazione del titolare sulla coerenza fra ruoli, non un difetto sentito.

**Proposta.** Riparazione a costo minimo (R-HAR-5): prima la diade di Corpo, poi il grado di Vettore, infine lo scarto del candidato. La prima mossa non richiede un nuovo tipo di voicing: `RootFifth` esiste già, legata però alla parità di k.

### Dominante e supporto: Vettore non è mai la voce superiore in A e A′

**Fonti.** DESIGN §8.5 chiede di distinguere canale dominante, supporto, atmosfera e opportunità. Moore divide la tessitura pop in beat esplicito, basso, melodia e riempimento. Lavengood aggiunge un livello *novelty*, melodico ma con altra funzione, usato con parsimonia ([Lavengood 2020, FT-parz, Moore SEC](https://www.mtosmt.org/issues/mto.20.26.3/mto.20.26.3.lavengood.pdf)). Gli errori nel seguire le voci di Bach salgono da circa 10% con tre voci a circa 50% con quattro. Con timbri diversi, nei giovani normoudenti, la prestazione a quattro voci è circa 72% contro 56%, e gli autori concludono che il limite vale per miscele omogenee ([Siedenburg et al. 2021, FT\*, Huron SEC](https://doi.org/10.3389/fpsyg.2021.608684)).

**Misure nostre.** Note di Vettore rispetto alla voce alta di Corpo.

| Sezione | Sopra | Uguale | Sotto | Registro di Vettore | Registro della Trama |
|---|---:|---:|---:|---|---|
| A | 0 | 15 | 109 | D4–A4 | D5–C6 |
| B | 70 | 38 | 15 | C4–G4 | G4–D5 |
| A′ | 0 | 0 | 127 | D3–A3 | D5–C6 |

Nel [codice](../../engine/audio/music_planner.h), riga 193, `dominantRole` ha default Vettore ma non guida la generazione. Corpo e Vettore sono entrambi saw filtrate, quindi i due ruoli che si sovrappongono sono anche i più simili di timbro.

**Inferenze.** Il ruolo d'atmosfera occupa il registro più acuto, e il ruolo dichiarato dominante ha perso ogni vantaggio di registro in A′. Può essere una scelta (melodia al basso), ma oggi non è dichiarata. Il limite di Huron non si applica direttamente: Pulso e Corpo sostenuto richiedono meno tracciamento di una linea melodica. La soglia deve quindi contare i flussi con contorno indipendente, non i ruoli attivi. Definire «contorno indipendente» resta una scelta di progetto. Mappatura proposta di §8.5: dominante è il ruolo con informazione nuova nella frase, supporto il riempimento o il basso, atmosfera la Trama senza contorno memorizzabile, opportunità una figura rara.

### Registro e mascheramento: il planner controlla la spaziatura, il livello resta offline

**Fonti.** Le disposizioni accordali occidentali evitano piccoli intervalli nel grave ([Harrison e Pearce 2020, FT\*](https://doi.org/10.1037/rev0000169); [Plomp e Levelt 1965, FT-parz](https://doi.org/10.1121/1.1909741)). Nei mix misurati da [De Man et al. 2014, FT-parz](https://archives.ismir.net/ismir2014/paper/000293.pdf) il kick sta a −13,15 ± 3,89 LU. Il testo dà la voce a −2,7 LU, ma la tabella la colloca altrove: l'incoerenza è interna alla fonte e non risolta. I singoli al numero uno decadono di circa 5 dB per ottava fra 100 e 4000 Hz ([Pestana et al. 2013, FT-parz](https://joshreiss.github.io/documents/2013/Pestana%20Ma%20Reiss%20-%20AES135.pdf)). Il bilanciamento contro rumore rosa è presentato senza misure né test alla cieca ([Bazil 2014, FT\*](https://www.soundonsound.com/techniques/mixing-pink-noise-reference)), e la nota non ha trovato studi peer reviewed che lo verifichino.

**Inferenze.** Il divario del kick di 7–9 dB in CYMATICA non è anomalo in sé rispetto a De Man, ma il confronto è debole: RMS contro LU, batteria acustica contro kick sintetico breve, rientri fra microfoni. Non smentisce l'esperto. Dice che alzare il solo livello può non bastare, e che contano anche attacco, durata e banda. Il rumore rosa vale −3 dB per ottava, mentre Pestana misura circa −5: può servire come partenza d'orecchio, non come criterio. Il registro nominale non è la banda spettrale, perché Corpo e Vettore hanno parziali ben oltre la fondamentale. Sequenza proposta, tutta offline: pendenza dello spettro medio, loudness per stem, A/B.

### Timbro: correggere il rapporto FM della Trama prima di scegliere le sue note

**Fonti.** Con c/m interi gli spettri sono armonici. Chowning ottiene campane con c/m = 1/1,4, e usa uno scarto di 0,5 Hz per un battimento lento ([Chowning 1973, FT](https://web.eecs.umich.edu/%7Efessler/course/100/misc/chowning-73-tso.pdf)). Parziali non armonici perdono la coincidenza che tiene bassa la dissonanza ([Plomp e Levelt 1965, FT-parz](https://doi.org/10.1121/1.1909741)).

**Misure nostre, su modello.** Modello di [Sethares, FT\*](https://sethares.engr.wisc.edu/comprog.html), spettri teorici stazionari, D5. Con m/c = 1,88 ogni coppia di parziali dista 0,12·c: 47 Hz su G4, 126 Hz su C6.

| m/c | Inarmonico? | Autodissonanza, I = 1,5 | I = 3 |
|---|---|---:|---:|
| 1 | no | 0,000 | 0,001 |
| 2 | no, solo dispari | 0,000 | 0,000 |
| √2 | sì | 0,005 | 0,042 |
| 1,76 | sì | 0,167 | 0,482 |
| **1,88, attuale** | sì | **0,508** | **0,848** |
| Saw a 10 armoniche, per confronto | no | 0,028 | — |

L'aumento di dissonanza dato da una nota Trama sopra la diade D3 + A4 va da 0,88 a 1,15 con 1,88, e da 0,08 a 0,88 con 1:1.

Verifica sul [codice](../../engine/audio/voice_allocator.cpp), riga 597: il rapporto è `1 + 1,76·timbreParam`, e il planner passa 0,50. Cambiare la costante 1,76 lascia invariati piano e trace. Alle altezze attuali lascia invariato anche l'indice di picco a 3,0, perché il limite di Carson resta saturato a 4. Questo restringe la lacuna della nota: rapporto e indice si muovono insieme solo se si cambia `timbreParam`.

**Inferenze.** Il modello ha un solo meccanismo, l'interferenza, e ignora armonicità, inviluppo e durata. Le note reali durano 107–125 ms e l'indice decade. Il risultato è plausibile rispetto al giudizio «un po' dissonante» dell'esperto, ma resta da confermare all'ascolto. Con 1,88 la scelta dell'altezza pesa poco, quindi conviene correggere prima il rapporto. 1:2 non è gratis, perché con le sole armoniche dispari il tritono risulta consonante quanto la quinta. 1:√2 conserva un carattere FM con poca ruvidità solo da sola: sopra la diade D3 + A4 lo stesso modello le assegna l'aumento più alto, 1,08–1,39 contro 0,88–1,15 di 1,88, quindi non è un candidato sicuro. Il planner può usare una tabella calcolata offline dallo spettro nominale, che al momento della pianificazione è dato simbolico: nessuna analisi audio.

**Ordine d'intervento.** La nota propone rapporto FM, voicing di Corpo, registro di Vettore, stato metrico, livello del kick. ADR-0007 mette invece il mix prima della struttura, perché F1 si giudica solo a kick corretto. Il dossier segue ADR-0007, che è più stringente sul metro: stem in M2.5.1, kick e rapporto FM in M2.5.2, stato metrico in M2.5.3, voicing e registro in M2.5.4. Il kick resta dopo le misure per stem, come chiede la nota.

**Esperimenti minimi.** A/B della Trama con rapporti 1,88, 2,0, 1,0 e √2, stesse note e stessa loudness. A/B dell'àncora a più livelli di kick: kick sul tempo 1, backbeat, `2,6,10,14` tal quale, con compito di battere il tempo per misurare la fase scelta. Un cambiamento per pacchetto, secondo D-M2-19.

## Lo sviluppo dipende dall'ordine: ciò che il rimescolamento non vede è varietà fittizia

**Fonti.** La letteratura non offre una metrica validata dello sviluppo. Una rassegna del 2025 scrive, riferendosi a forma e ripetizione: «specific metrics to measure these higher-level features have not yet been proposed» ([Lerch et al. 2025, FT-parz](https://doi.org/10.1145/3769106)).

Le basi percettive indicano che la parentela immediata è di superficie. Ascoltatori con e senza formazione classificano 34 sottosezioni tematiche per tempo, tessitura, registro e contorno ([McAdams et al. 2004, FT-parz](https://mcgill.ca/mpcl/files/mpcl/mcadams_2004_musperc_1.pdf)). La dimensione a bassa complessità domina il giudizio di similarità ([Ens et al. 2017, FT-parz](https://archives.ismir.net/ismir2017/paper/000079.pdf)). Sulla forma lunga l'evidenza è modesta. Con chunk di circa 6 s rimescolati, i partecipanti rilevano solo il 17% dei collegamenti alterati nel pop e circa il 13% nella contemporanea, ma giudicano il rimescolato meno coerente ([Lalitte e Bigand 2006, FT-parz](https://lead.ube.fr/wp-content/uploads/2023/09/000296-music-in-the-moment-revisiting-the-effect-of-large-scale-structures.pdf)). Information content ed entropia hanno il legame più solido con giudizi umani, ma misurano l'aspettativa nota per nota ([Pearce 2018, FT\*](https://pmc.ncbi.nlm.nih.gov/articles/PMC6849749/); [Gold et al. 2019, FT\*](https://pmc.ncbi.nlm.nih.gov/articles/PMC6867811/)). Il piacere dipende dal contesto d'incertezza ([Cheung et al. 2019, AM](https://doi.org/10.1016/j.cub.2019.09.067)).

Sull'intenzionalità, Ventura osserva che in un sistema stocastico la novità è quasi certa ma non voluta, e distingue la conoscenza nel filtro da quella nel processo: solo la seconda spiega perché un artefatto è nato ([Ventura 2016, FT-parz](http://www.computationalcreativity.net/iccc2016/wp-content/uploads/2016/01/Mere-Generation.pdf)). Sul gaming: valutare con una metrica che fa parte del progetto del sistema è un errore noto ([Yang e Lerch 2020, FT-parz](https://doi.org/10.1007/s00521-018-3849-7)), e [Lerch et al. 2025, FT-parz](https://doi.org/10.1145/3769106) richiama la legge di Goodhart. Il test di Turing regge solo in condizioni spesso assenti negli esperimenti piccoli. Il CAT classico si basa su più giudici esperti indipendenti, in media poco più di 10 ([Baer e McKool 2014, FT-parz; Amabile SEC](https://www.rider.edu/sites/default/files/2024-05/GoldStandard.pdf)), quindi non si applica a due persone.

**Misure nostre.** Calcoli della nota sul binomiale esatto, unilaterale, α = 0,05, con prove assunte indipendenti, quindi ottimistici. Con probabilità di risposta corretta 0,75, 12 prove danno potenza 0,39 e 24 prove 0,77. Nel test del segno servono **almeno 6 clip tutte concordi** per p < 0,05 bilaterale. Dopo 9 su 12 corrette, con prior uniforme, l'intervallo credibile al 95% è 0,46–0,91: compatibile col caso. Frasi medie della fixture: 7,6 s, vicine ai chunk di Lalitte e Bigand.

**Inferenze.** Sviluppo implica tre proprietà distinte: derivazione, cumulatività e direzione. Ogni statistica funzione del solo insieme delle frasi resta invariata permutandole: entropia globale, conteggio di materiali distinti, distanza media. Non può quindi distinguere sviluppo e materiale rimescolato. Un generatore che alza queste misure senza intenzione musicale è esattamente la varietà fittizia.

Una variazione «imparentata e nuova» deve conservare un invariante udibile e cambiare qualcosa di udibile. Il ritmo di Pulso, Trama e Vettore è fisso per 64 battute. Da Ens deriva un'ipotesi verificabile: cambiare il ritmo pesa più che cambiare le altezze, quindi una variazione melodica a ritmo fisso potrebbe restare poco percepita. Il «nuovo suono» chiesto dall'esperto diventa sviluppo solo se assume una funzione: se ritorna o trasforma un materiale. Dentro il budget di 8 voci, due per ruolo, significa ri-orchestrare o cambiare timbro in un ruolo esistente. Una voce aggiuntiva richiederebbe la revisione di D-M2-03.

**Proposta: definizioni operative (CP-3).** Il criterio del titolare, «ricordi, sviluppi, sorprenda e ritorni», diventa una definizione controllabile. Una variazione è **sviluppo** se è derivata, cumulativa, dipendente dall'ordine e ripresa in seguito. È **autentica** se tutto ciò resta rilevabile da un ascoltatore che non conosce la verità sigillata. È **fittizia** se sparisce sotto permutazione o ablazione, oppure se cambia solo la metrica.

| Criterio | Definizione | Prova simbolica | Prova d'ascolto |
|---|---|---|---|
| Ricorda | Una frase ha un genitore precedente, una trasformazione nominata e un invariante udibile | Recupero cieco del genitore dagli eventi, contro controllo rimescolato | Vecchio o nuovo, con esche; classificazione libera |
| Sviluppa | La trasformazione parte dallo stadio precedente, non sempre dall'originale; l'ordine conta | Profondità del lignaggio; variogramma fraseologico oltre il nullo | Originale contro rimescolato |
| Sorprende | Evento improbabile in contesto stabile, poi ripreso | Sorpresa con contesto; calo di IC alla ripresa | Punti di sorpresa segnati senza imbeccate |
| Ritorna | Riapparizione riconoscibile dopo un allontanamento reale | Picco di similarità a lag lungo dopo una valle | Tocco al ritorno, confini sigillati |
| Varietà fittizia | Differenza senza conseguenza, indipendente dall'ordine, o solo metrica | Nessun riuso; stessa misura dopo permutazione | ABX al caso, oppure rilevata ma non motivabile |

Le misure sono proposte nostre, non validate. Vanno calcolate per ruolo e riportate come vettori, mai come punteggio unico. Il variogramma V(k) è la distanza media fra frasi a lag k, con la Levenshtein già esistente: il rumore lo dà piatto, il loop a zeri periodici, lo sviluppo crescente entro la sezione, il ritorno con una caduta. Altre candidate: consequenzialità (quota di materiale nuovo che ricompare), apprendibilità, curva del vocabolario, forza del ritorno. Su 24 frasi i dati sono pochi: vanno lette come diagnosi.

Prima di usarle vanno provate su sei fixture: loop letterale, rumore nella scala, sviluppo reale rimescolato, trasposizione casuale per frase, ornamenti casuali, controllo positivo scritto da una persona. Una metrica che non le ordina correttamente è un cavallo nel senso di [Sturm 2016, AM](https://arxiv.org/abs/1606.03044). Una misura usata nello scoring del planner non vale più come prova. Ipotesi e soglie si fissano prima dell'ascolto. Nessuna soglia è proposta qui.

Variazione guidata dallo stato. Lo stato comprende registro dei motivi, carico per ruolo, posizione nella forma e novità recente, tutto a capacità fissa e in interi. Tre meccanismi deterministici completano lo schema. Il **budget di novità** limita quante dimensioni portatrici di forma cambiano insieme. Il **debito di contrasto** cresce con le ripetizioni e si estingue con un contrasto. Il **consumo obbligato** impone il riuso di un elemento nuovo entro N frasi. L'espressione «novelty budget» non è stata trovata nella letteratura musicale: i meccanismi sono nostri, e solo il contrasto ha un appiglio in [System & Contrast, AM](https://hal-univ-tlse3.archives-ouvertes.fr/INRIA/hal-01188244). Il seed deve agire sopra i pareggi: **lo stato decide quando, il seed sceglie quale percorso fra alternative d'autore**. Oggi i 64 seed danno un solo contenuto ([Teorema D](../m2_procedural_evidence.md)). L'anti-ripetizione evita un difetto ma non costruisce sviluppo: va usata come vincolo, non come motore. Il rischio speculare è in [MorpheuS, FT-parz](https://arxiv.org/pdf/1812.04832): pattern troppo vincolanti riconducono al modello.

Batteria d'ascolto proposta. La descrizione libera viene prima delle scale, per non suggerire i termini.

| Test | Domanda | Procedura | Limite |
|---|---|---|---|
| B0 Controlli | Quanto è rumoroso il giudizio? | Coppie identiche e duplicati nascosti in ogni sessione | Da riportare sempre |
| B1 Rilevazione | Il cambiamento si sente? | ABX su 8–16 battute, 16–24 prove | Non dice nulla sul valore |
| B2 Ritorno | Il materiale viene ricordato? | Vecchio o nuovo su A, A′ ed esca dello stesso stile | Servono motivi diversi come esche |
| B3 Ordinamento libero | La parentela sentita coincide col lignaggio? | Raggruppare 10–15 frammenti e descrivere | Il confronto immediato privilegia la superficie |
| B4 Segmentazione | Le sezioni sono udibili? | Tocco a ogni fine sezione e ritorno | Non prova la funzione del confine |
| B5 Ordine | L'ordine conta? | Originale contro rimescolato, primo e ultimo chunk fissi, almeno 6 clip | Un nullo informa poco; serve materiale nuovo per ogni coppia |
| B6 Ablazione | Il componente serve? | Completo contro versione senza, un componente alla volta | Richiede prima B1 |
| B7 Attenzione divisa | Regge in gioco? | Stesso giudizio con un compito o in gioco | Rilevante per M7 |

L'ablazione è la prova finale, a quattro esiti.

| Esito | Significato |
|---|---|
| Inerte | Il fingerprint non cambia: variazione nulla a livello simbolico |
| Attivo, non rilevato | Il contenuto cambia, l'ABX non distingue: da rivedere, non è prova di inaudibilità |
| Rilevato, non motivato | Si sente, ma non cambia ordine, ritorno o giudizio: candidato a varietà fittizia |
| Rilevato e motivato | L'ascoltatore lo descrive e la descrizione concorda con la verità sigillata: massimo verdetto con due ascoltatori, valido per quelle persone e quelle clip |

Con due ascoltatori le clip sono l'unità replicabile e ogni persona si analizza da sola ([Smith e Little 2018, FT\*](https://pmc.ncbi.nlm.nih.gov/articles/PMC6267527/)). Coppie identiche e duplicati nascosti sono obbligatori, altrimenti una preferenza non si distingue dall'aspettativa di miglioramento. Esperto e non musicista non si sostituiscono: su poesie, le medie di esperti e novizi correlano solo 0,21 ([Kaufman et al. 2008, FT-parz](https://doi.org/10.1080/10400410802059929)). L'inferenza è asimmetrica: un esito positivo informa, uno nullo è debole. Queste misure possono falsificare la varietà fittizia, non certificare il valore artistico.

## Nessuna soglia pubblicata: all'ascolto la ripetizione pesa presto, in gioco non si sa

**Fonti.**

| Contesto | Risultato | Limite | Fonte |
|---|---|---|---|
| Ascolto focalizzato | Picco a 2–8 esposizioni; a 32 livello iniziale o sotto | Esposizioni separate, gradimento e non attenzione; estratti da 15 s in Szpunar | [Szpunar et al. 2004, FT; AM nell'altra nota](https://doi.org/10.1037/0278-7393.30.2.370); [Schellenberg et al. 2008, FT](https://doi.org/10.1080/02699930701350753) |
| Ascolto incidentale | Il gradimento sale fino a 32 | La musica va ignorata: non è il caso di CYMATICA | stesse fonti |
| Esposizione naturale, 4 settimane | Crescita senza calo, N = 15 | Campione piccolo | [Madison e Schiölde 2017, FT\*](https://doi.org/10.3389/fnins.2017.00147) |
| Differenze individuali | 37% U rovesciata, 28% calo, 19% aumento, 16% U diritta | Meno della metà segue la curva media | [Hunter e Schellenberg 2011, FT](https://doi.org/10.1016/j.paid.2010.09.021) |
| Spaziatura | Un minuto di pausa annulla il calo: β −3,00 contro −0,58 n.s., N = 106 | Canzoni intere preferite, non loop | [Galak et al. 2011, FT\*](https://www.cambridge.org/core/product/E993A1FE1E30B988E71BE3D6A6726025/core-reader) |
| Gioco, 10 minuti per condizione | Il rendering espressivo non riduce la ripetizione percepita, N = 20 | Un brano, ogni giro identico, nessuna condizione a bassa ripetizione | [Worrall 2025, FT nelle parti lette](https://etheses.whiterose.ac.uk/id/eprint/38684/) |
| Gioco, gradimento | Musica sgradita abbassa l'immersione, musica scelta la alza | Labirinto poco esigente, campioni diversi | [Sanders e Cairns 2010, FT](https://www-users.york.ac.uk/~pc530/pubs/Sanders_HCI2010.pdf) |
| EDM, trance | Piacciono gli estremi, non il mutamento lieve: N = 20, estratti da 30 s | Preliminare, sede e anno non confermati | [Agres et al., FT](https://www.dorienherremans.com/sites/default/files/preprint_trance.pdf) |
| Prassi | Loop di 1–2 minuti; 3–5 ripetizioni a rischio, oltre 5 caso peggiore | Osservazione e conversazioni informali | [Martins 2012, FT\*](https://www.gamedeveloper.com/audio/rethinking-the-audio-loop-in-games) |
| Testimonianze | Bassa complessità, fatica prima. La causa può essere la frustrazione verso la scena | 11 intervistati scelti fra chi ritiene importante la musica | [Dutta 2020, FT nelle parti lette](https://eprints.hud.ac.uk/id/eprint/35234/) |

Dentro un brano la ripetizione può aumentare il piacere anche in stili ostici ([Margulis 2013, AM](https://doi.org/10.2190/EM.31.1.c)). Preferenza e forza della ripetizione sono però due fattori distinti ([Touizrar et al. 2021, FT\*](https://doi.org/10.3389/fpsyg.2021.673706)). Nella EDM il loop è un processo di aggiunta e sottrazione di strati su multipli di 4 battute ([Garcia 2005, FT\*](https://www.mtosmt.org/issues/mto.05.11.4/mto.05.11.4.garcia.html)), ma l'analisi è senza dati empirici. Lo studio più vicino al formato di CYMATICA confronta loop di 15 s e 2,5 minuti, con pochi risultati significativi ([McNamara e Arthur, FT\*](https://ccml.gtcmt.gatech.edu/projects/McNamara_Flow_2022.html)). Nessuna telemetria pubblica su mute o volume è stata trovata, e nessun dato su bullet hell o modalità infinite.

**Misure nostre.** A 120 BPM 4 battute durano 8 s. Due giri sono 16 s, quattro giri 32 s, dieci minuti 75 giri, cioè 3,3 passaggi della forma da 183 s. Il [registro del 2026-10-28](../m2_4_human_listening_2026-10-28.md) colloca il calo d'attenzione dopo il secondo loop per un ascoltatore medio e intorno al quarto per uno attento, e parla altrove di loop di 4 battute. Se è lo stesso loop, il calo cade a 16 e 32 s, ma il registro non fa esplicitamente questa equivalenza.

**Inferenze.** Il rapporto dell'esperto è coerente con un picco precoce in ascolto focalizzato su materiale generato. È coerenza, non conferma: un'esposizione di laboratorio è un'unità separata, non un giro contiguo. «Mai fastidiosa» e ripetitività 7 su 7 non si contraddicono, perché attenzione, ripetitività percepita e gradimento sono costrutti distinti. EDM e minimalismo non provano che 75 giri identici reggano: il tessuto cambia su scale superiori. Il dato utile è la gerarchia, non la ripetizione nuda.

L'attenuazione in gioco è a doppio taglio: se il carico protegge dalla noia, nasconde anche lo sviluppo. Conviene quindi puntare su macro-arrangiamento e sottrazione, udibili sotto carico, e non su micro-variazioni. Worrall sconsiglia di attendersi molto da queste ultime. Il trasferimento dall'ascolto incidentale è incerto, perché in CYMATICA il giocatore deve riconoscere il ritmo. Ipotesi non verificata: ascolto focalizzato sul ritmo, incidentale su armonia e melodia. Quale livello i giocatori sentano come «il loop», cellula o forma, è aperto. Il silenzio ha un sostegno convergente ma indiretto, e in CYMATICA è limitato dalla leggibilità: vanno distinti gli strati che segnalano da quelli che sviluppano. Una fatica non notata non equivale a musica riuscita.

**Proposta (CP-1, M7).** In M2.5 il giudizio isolato va trattato come misura di ascolto focalizzato: adatto a stereo, timbro, click, mix e forma udibile, non a una soglia per il gioco. Si registrano a parte ripetitività, attenzione, gradimento e fastidio, e si fa segnare il momento del calo per sezione. Nessuna soglia di ripetizione entra nel contratto. A M7, quando esisterà un prototipo giocabile: disegno entro i soggetti, sessioni da 10 minuti, pause, ordine bilanciato, musicisti e non musicisti separati. Vanno aggiunte le due condizioni che mancano in Worrall: bassa ripetizione e variazione davvero diversa a ogni giro. Un compito visivo-ritmico surrogato prima del gioco è possibile, ma solo come esplorazione.

## M2.5 riceve un contratto minimo, §16.3–§16.4 va a M6

Tutti gli interventi seguenti sono proposte. Alla data del dossier M2.5.1 non è autorizzata.

| Fase | Cosa entra (proposta) | Condizione |
|---|---|---|
| **M2.5.1** | Strumento offline di misura: fixture avverse, nullo per permutazione, variogramma; statistiche verticali su griglia intera. Stima del costo logico dei progetti B, D e G, senza integrarli. | Non tocca il segnale |
| **CP-1** | Estendere P-M25-03: coppie identiche, duplicati nascosti, costrutti separati, istante del calo, Q3 per sezione, descrizione libera prima delle scale, domande semplici per il secondo ascoltatore. Decidere se il rapporto FM entra in M2.5.2 | Decisione del titolare |
| **M2.5.2** | F4 con kick e A/B. A/B del rapporto FM tramite la costante 1,76: piano e trace invariati, golden PCM aggiornato | Serve estendere il perimetro, oggi limitato a costanti di bus e di stereo |
| **CP-2** | F1 si legge come stato metrico da dichiarare, non come difetto da eliminare. A/B dell'àncora a più livelli di kick, con tapping | Decisione del titolare |
| **M2.5.3** | F3 come previsto. Se CP-2 lo chiede, R-MET-1: rotazione di Pulso dichiarata, mai derivata da k | Golden di piano e trace |
| **CP-3** | Dove va F5: raccomandazione nella matrice sotto | Momento limite CP-4 |
| **M2.5.4** | Contratto minimo, un cambiamento per pacchetto, in quest'ordine: diade di Corpo sui punti contraddetti; lettura dell'accordo da parte di Vettore, con e senza, come prima ablazione; `dominantRole` che guida, con il registro di Vettore in A′; entrate e uscite dei ruoli; infine seed sopra i pareggi su una tabella chiusa d'autore | Solo se CP-3 lo autorizza, secondo D-M2-19 |
| **M2.5.5** | Pacchetto con seed nuovo; B0 controlli, B1 rilevazione, B4 segmentazione. Specificità solo con gruppi indipendenti, altrimenti `INSUFFICIENT_DATA` | Regole già in vigore |
| **M2.5.6** | Ascolto al gate con il protocollo fissato. Le regole d'insieme non diventano criteri mandatory | Regola D-M2-20 |

**Matrice per CP-3: dove va F5.**

| Opzione | A favore | Contro | Valutazione |
|---|---|---|---|
| A. Tutto F5 a M6 | Meno rischio, M2.5 più breve | Nessuna ablazione, e va modificato il rinvio 5e-07 di ADR-0006; contenuto unico, quindi specificità non misurabile; la richiesta dell'esperto resta senza risposta | Accettabile se i primi interventi bastano al livello 2 |
| **B. Contratto minimo in M2.5.4** | Prima prova diretta del coordinamento, stimoli per l'ablazione, base per il sidecar di M3 | Cambia piano, trace e golden. Rischio di varietà fittizia se il seed agisce prima dello sviluppo | **Raccomandata**, con tabella d'autore e A/B |
| C. §16.3–§16.4 completo in M2.5 | Copre tutto | D-M2-19 scarta la riscrittura; il Pacing Director non esiste ancora | Esclusa |

**Matrice per P-M25-06: quale milestone attua §16.3–§16.4.**

| Milestone | A favore | Contro | Valutazione |
|---|---|---|---|
| M3 | Il checkpoint di M3 deve già decidere un piano semantico o sidecar | Obbligo «senza adattamento». §16.4 dipende da novelty budget e pacing, che non esistono ancora | Decide la forma del contratto, non l'attuazione |
| **M6** | Ha `ExperienceTarget.novelty`, pacing state machine, novelty memory, selettore top-k, commit horizon, explanation trace, Pure Seed | Nessun deliverable musicale: va aggiunto a §32 con decisione esplicita | **Raccomandata** |
| M7 | Qui stanno i gate: sessione infinita di almeno 10 minuti, «run diverse mostrano diversità misurabile» | Costruire al gate toglie il margine: prova e costruzione coinciderebbero | Prova, non attuazione |
| M8 | Quality-diversity | Viene dopo il gate di M7 | No |

La raccomandazione rispetta ADR-0007: non amplia il CIE Director. §16.3–§16.4 diventa un deliverable proprio di M6, con un nome distinto, che usa gli stessi strumenti. Serve una modifica alla specifica §32, da decidere con ADR entro CP-4.

| Milestone | Cosa resta o arriva |
|---|---|
| M3 | Decidere se il contratto d'insieme diventa il sidecar di §16.13. Nessun nuovo campo pubblico. Decidere quali campi congelare prima che i pattern ne dipendano |
| M4 | Ogni pattern derivato dalla musica passa dal validatore. Direttore e regole d'insieme non autorizzano maggiore letalità |
| M5 | Leggibilità dei segnali con utenti reali; legame fra salienza musicale e letalità (§8.5); uso del foreshadowing |
| M6 | Form planner con grammatica e memoria; motif memory con lignaggio; budget di novità e debito di contrasto; interfaccia col Pacing; candidati per ruolo; matrice condivisa se giustificata |
| M7 | Prova in gioco della sezione precedente; 10 minuti, cioè circa 9 sezioni; diversità autentica fra run; valutazione di affaticamento |
| M8 | Descrittori e versioni di motivi, tensione e trasformazioni, separando copertura statistica e valore artistico |

## Correzioni: G19, G21, G22, la proposta M2.5 e sei incongruenze nelle note

**Correzioni a fonti e documenti del repository.** La [ricerca d'ingresso di Inc-5](M2_4_Inc5_Entry_Research.md) rimanda a questa tabella con un erratum, e la [proposta M2.5](M2_5_Entry_Music_Quality_Proposal.md) §8 è corretta.

| Voce | Testo attuale | Correzione proposta |
|---|---|---|
| **G19** ([riga 253](M2_4_Inc5_Entry_Research.md)) | Immersione riportata superiore alle colonne sonore originali in due giochi. Livello AM | I confronti fra gruppi, 17 persone ciascuno, non sono significativi a 0,05: immersione p = 0,121 e 0,053, correlazione p = 0,057 e 0,055. Il Wilcoxon è significativo sull'effetto percepito della musica. Il riconoscimento dei concetti cala: 70→53% e 59→41%. Gli autori ammettono una qualità inferiore a quella di un compositore esperto. Livello: FT\* o FT-parz sulle sezioni VI–VIII ([arXiv](https://arxiv.org/abs/1907.01154)) |
| **G21** (righe 255 e 278) | La variazione espressiva è meno percepibile durante il gioco che in ascolto isolato. Livello AM | Vale per la variazione espressiva fine e statica fra i giri, confrontata fra campioni diversi. Non dice nulla sul macro-arrangiamento né sulla tolleranza. Worrall è in contraddizione interna: abstract e §7.4.4 dicono che la variazione influenza significativamente la ripetizione percepita, i fattori di Bayes e §7.5 indicano assenza di beneficio. La variazione statica può persino aumentarla. Livello: FT su §2.2, §3.6, §7.3–7.6 ([tesi](https://etheses.whiterose.ac.uk/id/eprint/38684/)) |
| **G22** (righe 256 e 278) | I giocatori tollerano la ripetizione, che ha anche funzione di feedback; l'eccessiva familiarità riduce il piacere. Livello AM | Studio qualitativo: 11 intervistati, 10 trascritti, 7 al secondo colloquio, campionamento mirato di chi ritiene importante la musica. Tolleranza contestuale. L'autore avverte che la causa può essere la frustrazione verso la scena. Non è generalizzabile. Livello: FT nelle parti lette; Volumi II–III non letti ([tesi](https://eprints.hud.ac.uk/id/eprint/35234/)) |
| **Proposta M2.5 §8** | «Accordo e scala condivisi» fra i legami presenti | Vettore condivide solo la scala. L'accordo raggiunge Corpo e Trama |

**Incongruenze trovate nelle note.**

| Punto | Problema | Esito |
|---|---|---|
| Nota sulle regole, riga B: «Tempi 1 e 3 = 1,00» | Non riproducibile: la media con 0,48 darebbe 0,74, non 0,49. Lo script confronta posizioni in virgola mobile con `0.0` e `8.0`, e a 140 BPM la battuta non è un numero intero di campioni | **Ricalcolo nostro: 0,50 e 0,50**. Su tempi 2 e 4, CTnCTR 0,75 e non 0,80. La lettura qualitativa su B regge, ma il problema sui tempi forti di B è maggiore di quanto dicesse la nota |
| Chmiel e Schubert | 50 studi su 57 compatibili ([AM](https://doi.org/10.1177/0305735617697507)) in una nota; effetto Wundt in 15 su 57 (SEC, via Gold) nell'altra | Probabilmente criteri diversi, ma non verificato. Non usare nessuno dei due come forza della curva |
| Lunghezza del «loop» | Una nota assume 16 s per due giri, l'altra dice che la lunghezza non è definita | Il registro del 2026-10-28 parla di loop di 4 battute in un'altra riga. Equivalenza da confermare con l'esperto |
| Hutchings e McCormack | FT-parz in una nota, FT\* nell'altra | Convenzioni diverse sullo stesso testo: si riportano entrambi |
| Szpunar et al. 2004 | FT in una nota, AM nell'altra | I dati citati vengono dalla nota FT |
| Conflitto brevetto e intervista su Hellsinger | Il brevetto dice che sparare fuori tempo non punisce direttamente, l'intervista che peggiora la prestazione | Non risolto; [brevetto](https://patents.google.com/patent/US11358065) e [intervista, FT\*](https://gamedeveloper.com/design/shredding-for-satan-how-i-metal-hellsinger-i-designed-fps-harmonies-in-hell) restano distinti |

## Cosa il dossier non può dire: limiti e domande aperte

**Limiti.** Nessuna delle regole è stata validata su musica sintetica a otto voci. Le prove dirette sul coordinamento si riducono a due studi su modelli appresi, in Bach e nel rock. I test «Bach o computer» misurano l'imitazione di uno stile, non l'obiettivo di CYMATICA. Le misure nostre riguardano un solo brano, il seed 1337, perché il generatore ha diversità nulla. I giudizi umani vengono da un solo esperto, in condizioni non controllate, e nessun ascolto umano è alla cieca. I modelli (Sethares, LHL, E0) non sono prove percettive. La tabella di compatibilità col replay è analisi nostra, e nessun costo è stato misurato sul planner. Le ricerche non sono esaustive, e le tabelle numeriche delle fonti vengono da estrazione automatica. Gli script di calcolo sono strumenti temporanei, archiviati in [m25_ensemble_baseline](experiments/m25_ensemble_baseline/README.md).

**Domande aperte.**

| Domanda | Perché conta | Quando |
|---|---|---|
| Il direttore può rifiutare, o solo differire? | Nessuna fonte tratta il caso | ingresso M6 |
| Validità in frasi o in tempo? | Total War non lo dice; il tempo cambia fra sezioni | CP-3, M6 |
| Con il kick alzato, quale fase si sente? | Decide F1 e il valore di R-MET-2 | CP-2 |
| Il backbeat àncora davvero, e con quale timbro? | Il modello dice no, l'esperto sì per il suo schema. Pulso ha due voci, e un timbro nuovo ha un costo DSP | CP-2 |
| Quale rapporto FM? | Il modello indica una zona, l'orecchio deve scegliere | M2.5.2 |
| Quale periodo sentono come «loop»? | Cambia il confronto con la prassi | CP-1, M7 |
| Che cos'è un contorno indipendente? | Serve per R-ROL-4 | CP-3 |
| Chi scrive il controllo positivo, restando nel linguaggio CYMATICA? | È l'unico riferimento esterno | CP-3 |
| Come rimescolare in A–B–A′? | Il cambio di tempo limita il rimescolamento alle singole sezioni | M2.5.1 |
| M3–M5 si tarano su un brano solo? | Variare poi la musica richiede regressioni del validatore | ingresso M3 |
| Il foreshadowing aiuta? | [Scirea et al. 2014, AM](https://www.kmjn.org/publications/Foreshadowing_AM14-abstract.html) dice sì con N = 30; Hell Is Us vieta di anticipare; Left 4 Dead usa la musica come segnale | M5, M6 |
| Il giudizio isolato predice quello in gioco? | Nessuno studio stima la correlazione per la stessa musica | M7 |

## Conclusione

La domanda iniziale chiedeva quanto dovesse essere intelligente il direttore. Le note spostano la domanda: il rischio documentato è la complessità che si traveste da intenzione, mentre il rimedio documentato è un vincolo semplice, scritto in anticipo e verificabile. Un contratto sottile ha un vantaggio che un compositore centrale non ha: ogni campo si può togliere e misurare. Per questo l'ablazione non è solo una verifica, ma il criterio di progetto: un campo del contratto che non cambia nulla di rilevabile non dovrebbe esistere.

Il secondo spostamento riguarda l'ordine dei lavori. Le mosse più economiche non richiedono un direttore: una costante FM, una diade di Corpo già disponibile nel codice, uno stato metrico dichiarato. Conviene farle prima, perché un direttore costruito sopra una Trama che il modello dà ruvida a ogni altezza avrebbe poco su cui agire. Infine, poiché la letteratura non decide, il progetto può produrre un dato che le note non hanno trovato: stessa forma, stesso seed, con e senza lettura dell'accordo da parte di Vettore, in un sistema deterministico a regole. Vale per quelle persone e quelle clip, non per il pubblico, ma per CYMATICA è più diretta di tutto ciò che è stato letto.

## Verifica del dossier (2026-11-04)

Il dossier è una sintesi delle note. Prima di archiviarlo è stato controllato a campione, in modo indipendente dalla redazione.

- **Misure riprodotte.** Note d'accordo, CTnCTR e PCS, 8 tempi 1 su 25, dissonanza FM, potenza binomiale, LHL ed E0 coincidono con gli output degli script. Le durate sono state ricalcolate.
- **Codice e specifica.** Confermati `music_planner.cpp` righe 925–938 e 1014–1042, `dominantRole` mai letto, `voice_allocator.cpp` riga 597, il limite di Carson a 4 in `synth_dsp.h` (quindi indice 3,0 con `timbreParam` 0,50), la specifica §16.12 e il budget di voci in D-M2-03.
- **Fonti, sul testo estratto.** Confermati i valori citati di Hutchings e McCormack (tabella delle p), MuseGAN (armonia, ascoltatori pro, da zero), Coconet, MetaCompose, Hunter e Schellenberg, Lalitte e Bigand, De Man, Dutta (11 intervistati, 10 trascritti, campionamento mirato) e la citazione di Lerch et al. 2025.
- **Correzione apportata.** Il dossier presentava 1:√2 come rapporto a bassa ruvidità senza dire che, sopra la diade di Corpo, il modello gli assegna l'aumento maggiore. Il paragrafo sul timbro è corretto.
- **Non verificato.** Le altre fonti restano al livello dichiarato nelle note. Gli URL non sono stati riaperti uno per uno. Sezioni e tabelle non citate qui si basano sulle note.

## Appendice A. Registro delle fonti citate

Livelli come nelle note. L'ultima colonna indica la copertura o una riserva.

| Fonte | Livello | Copertura o riserva |
|---|---|---|
| [Land e McConnell, US 5315057](https://patents.google.com/patent/US5315057) | FT\* | Caratteri 0–200.000 di 240.053. Descrive il rivendicato, non il prodotto |
| [Booth 2009](https://steamcdn-a.akamaihd.net/apps/valve/2009/ai_systems_of_l4d_mike_booth.pdf) | FT-parz | Pacing e mob |
| [Commento Left 4 Dead](https://left4dead.fandom.com/wiki/Developer_Commentary_%28Left_4_Dead%29) | FT/SEC | Trascrizione di fan |
| [O'Donnell 2002](https://halo.bungie.org/misc/gdc.2002.music) | FT\* | |
| [Melham e Beddow 2018](https://www.audiokinetic.com/en/community/blog/total-war-warhammer-2-how-we-construct-and-systemically-use-our-music/) | FT | |
| [Vachon 2026](https://www.audiokinetic.com/en/community/blog/from-constraint-to-creativity-the-music-design-of-hell-is-us/) | FT | |
| [Goldfarb et al., US 11358065](https://patents.google.com/patent/US11358065) | FT\* | Legame col gioco dedotto dalla nota |
| [Couture 2022](https://gamedeveloper.com/design/shredding-for-satan-how-i-metal-hellsinger-i-designed-fps-harmonies-in-hell) | FT\* | |
| [Parkinson 2020](https://gameplay.co/hades-game-music-sound-design-darren-korb-supergiant-games/) | FT\* | |
| [Guida SGG-Modding](https://sgg-modding.github.io/Hades2ModWiki/docs/creating-mods/audio/adding-dynamic-music) | FT\* | Comunità |
| [Wwise-201, modulo 16](https://www.audiokinetic.com/en/courses/wwise201/?source=wwise201&id=smoothing_transition_decisions_configuring_an_rtpc_values_to_control_switch) | FT-parz | Circa 4.500 di 6.647 caratteri |
| [FMOD, Studio API Threads](https://www.fmod.com/docs/2.03/api/studio-api-threads.html) | FT | |
| [Holtar et al. 2013](https://www.kmjn.org/publications/Audioverdrive_ICMC13.pdf) | FT | |
| [Hutchings e McCormack 2020](https://arxiv.org/abs/1907.01154) | FT\*; FT-parz | Sez. VI–VIII. [DOI](https://doi.org/10.1109/TG.2019.2921979). Sezioni I–V non lette |
| [Aspromallis e Gold 2016, FATE](https://discovery.ucl.ac.uk/id/eprint/1505747/1/Aspromallis_Gold_CR_.pdf) | FT-parz | Sez. 1–3 e conclusioni |
| [Scirea et al. 2014](https://www.kmjn.org/publications/Foreshadowing_AM14-abstract.html) | AM | |
| [Fiedler 2014](https://gafferongames.com/post/deterministic_lockstep/) | FT\* | |
| [G4 Mini Motorways](https://www.gamedeveloper.com/design/turning-road-noise-into-music-mini-motorways) | FT\* | Livello del repository |
| [G7 BPM](https://www.audiokinetic.com/en/blog/syncing-music-and-gameplay-in-bpm) | FT | Livello del repository |
| [Eigenfeldt e Pasquier 2009](http://www.sfu.ca/~eigenfel/A%20Realtime%20Generative%20Music%20System.pdf) | FT\* | Integrale |
| [Eigenfeldt 2010](http://www.sfu.ca/~eigenfel/ComingTogether_Beauty&Truth.pdf) | FT\* | Sez. 1–6 |
| [Eigenfeldt 2016, Musebots](https://zenodo.org/records/4285186/files/Eigenfeldt_Musebots.pdf) | FT\* | Passi selezionati |
| [Blackwell e Young 2004](https://eprints-gro.gold.ac.uk/106/1/OSYoung04.pdf) | FT\* | Sez. 1–3 |
| [Marchini et al. 2017](https://www.francoispachet.fr/wp-content/uploads/2021/01/marchini-17nime.pdf) | FT\* | Sez. 2 |
| [Tatar e Pasquier 2019](https://doi.org/10.1080/09298215.2018.1511736) | AM | Testo integrale non ottenuto |
| [Dong et al. 2018](https://arxiv.org/abs/1709.06298) | FT\* | Modelli, metriche, tab. 2–3 |
| [Huang et al. 2017](https://arxiv.org/abs/1903.07227) | FT\* | Sez. 1–3, 5–7 |
| [Liang et al. 2017](https://archives.ismir.net/ismir2017/paper/000156.pdf) | FT\* | Codifica, sez. 3, discussione |
| [Chu et al. 2016](https://arxiv.org/abs/1611.03477) | FT\* | Preprint |
| [Wang et al. 2024](https://arxiv.org/abs/2405.09901) | FT\* | Sez. 3 e valutazione; figure non leggibili |
| [Scirea et al. 2016](https://researcher.itu.dk/ws/files/82418836/scirea_evomusart16.pdf) | FT\* | Sez. 4–7 |
| [Anders e Miranda 2011](https://doi.org/10.1145/1978802.1978809) | FT\* | Sez. 3, 4, 5, 7; CHORAL e altri SEC |
| [Eigenfeldt e Pasquier 2013, GESMI](http://www.sfu.ca/~eigenfel/Eigenfeldt%20ICCC%2013.pdf) | FT\* | Selezione a festival, non ascolto validato |
| [Witek et al. 2014](https://doi.org/10.1371/journal.pone.0094446) | FT\* | 100.000 caratteri su 108.969; Tabella S1 non letta |
| [Smith e Honing 2006](https://www.mcg.uva.nl/mcg-2023/papers/smith-honing-2006.pdf) | FT | |
| [Tan et al. 2019](https://doi.org/10.1525/MP.2019.36.4.353) | FT-parz | |
| [Butler 2001](https://www.mtosmt.org/issues/mto.01.7.6/mto.01.7.6.butler_essay.html) | FT\* | |
| [Hove et al. 2014](https://doi.org/10.1073/pnas.1402039111) | FT\* | Via PMC4104866 |
| [Tal et al. 2017](https://doi.org/10.1523/JNEUROSCI.2500-16.2017) | AM | |
| [Povel e Essens 1985](https://doi.org/10.2307/40285311) | SEC/AM | Solo E0. DOI non verificato |
| [Harrison e Pearce 2020](https://doi.org/10.1037/rev0000169) | FT\* parziale | 200.000 caratteri su 333.946. DOI non verificato |
| [Plomp e Levelt 1965](https://doi.org/10.1121/1.1909741) | FT-parz | |
| [Yeh et al. 2021](https://arxiv.org/abs/2001.02360) | FT-parz | |
| [Reed 2022](https://mtosmt.org/ojs/index.php/mto/article/view/876/194) | FT\* | Nobile e Temperley SEC |
| [Bharucha 1984](https://doi.org/10.1016/0010-0285%2884%2990018-5) | SEC/AM | DOI non verificato |
| [Lavengood 2020](https://www.mtosmt.org/issues/mto.20.26.3/mto.20.26.3.lavengood.pdf) | FT-parz | Moore SEC |
| [Siedenburg et al. 2021](https://doi.org/10.3389/fpsyg.2021.608684) | FT\* | Huron SEC |
| [De Man et al. 2014](https://archives.ismir.net/ismir2014/paper/000293.pdf) | FT-parz | Incoerenza interna sulla voce |
| [Pestana et al. 2013](https://joshreiss.github.io/documents/2013/Pestana%20Ma%20Reiss%20-%20AES135.pdf) | FT-parz | |
| [Bazil 2014](https://www.soundonsound.com/techniques/mixing-pink-noise-reference) | FT\* | Pratica, nessuna prova |
| [Chowning 1973](https://web.eecs.umich.edu/%7Efessler/course/100/misc/chowning-73-tso.pdf) | FT | Copia; URL AES non verificato |
| [Sethares, codice](https://sethares.engr.wisc.edu/comprog.html) | FT\* | Articolo del 1993 non letto |
| [Lerch et al. 2025](https://doi.org/10.1145/3769106) | FT-parz | |
| [McAdams et al. 2004](https://mcgill.ca/mpcl/files/mpcl/mcadams_2004_musperc_1.pdf) | FT-parz | |
| [Ens et al. 2017](https://archives.ismir.net/ismir2017/paper/000079.pdf) | FT-parz | Melodie brevi, valutatori online |
| [Lalitte e Bigand 2006](https://lead.ube.fr/wp-content/uploads/2023/09/000296-music-in-the-moment-revisiting-the-effect-of-large-scale-structures.pdf) | FT-parz | OCR rumoroso |
| [Pearce 2018](https://pmc.ncbi.nlm.nih.gov/articles/PMC6849749/) | FT\* | |
| [Gold et al. 2019](https://pmc.ncbi.nlm.nih.gov/articles/PMC6867811/) | FT\* | |
| [Cheung et al. 2019](https://doi.org/10.1016/j.cub.2019.09.067) | AM | Numero di partecipanti non verificato |
| [Ventura 2016](http://www.computationalcreativity.net/iccc2016/wp-content/uploads/2016/01/Mere-Generation.pdf) | FT-parz | |
| [Yang e Lerch 2020](https://doi.org/10.1007/s00521-018-3849-7) | FT-parz | |
| [Baer e McKool 2014](https://www.rider.edu/sites/default/files/2024-05/GoldStandard.pdf) | FT-parz | Amabile SEC |
| [Kaufman et al. 2008](https://doi.org/10.1080/10400410802059929) | FT-parz | Poesie, non musica |
| [Sturm 2016](https://arxiv.org/abs/1606.03044) | AM | |
| [Bimbot et al. 2016](https://hal-univ-tlse3.archives-ouvertes.fr/INRIA/hal-01188244) | AM | |
| [Herremans e Chew](https://arxiv.org/pdf/1812.04832) | FT-parz | Valutazione aneddotica |
| [Smith e Little 2018](https://pmc.ncbi.nlm.nih.gov/articles/PMC6267527/) | FT\* | Primi 100.000 caratteri |
| [Szpunar et al. 2004](https://doi.org/10.1037/0278-7393.30.2.370) | FT; AM | Dati dalla nota FT |
| [Schellenberg et al. 2008](https://doi.org/10.1080/02699930701350753) | FT | Durata degli estratti non ricavata |
| [Hunter e Schellenberg 2011](https://doi.org/10.1016/j.paid.2010.09.021) | FT | |
| [Madison e Schiölde 2017](https://doi.org/10.3389/fnins.2017.00147) | FT\* | |
| [Chmiel e Schubert 2017](https://doi.org/10.1177/0305735617697507) | AM; SEC | |
| [Galak et al. 2011](https://www.cambridge.org/core/product/E993A1FE1E30B988E71BE3D6A6726025/core-reader) | FT\* | |
| [Agres et al.](https://www.dorienherremans.com/sites/default/files/preprint_trance.pdf) | FT | Sede e anno non confermati |
| [Worrall 2025](https://etheses.whiterose.ac.uk/id/eprint/38684/) | FT | §2.2, §3.6, §7.3–7.6; il resto non è stato letto per intero. G21 |
| [Sanders e Cairns 2010](https://www-users.york.ac.uk/~pc530/pubs/Sanders_HCI2010.pdf) | FT | |
| [Martins 2012](https://www.gamedeveloper.com/audio/rethinking-the-audio-loop-in-games) | FT\* | |
| [Dutta 2020](https://eprints.hud.ac.uk/id/eprint/35234/) | FT | Parti indicate; Findings per parole chiave; Volumi II–III non letti. G22 |
| [Margulis 2013, Berio e Carter](https://doi.org/10.2190/EM.31.1.c) | AM | |
| [Touizrar et al. 2021](https://doi.org/10.3389/fpsyg.2021.673706) | FT\* | |
| [Garcia 2005](https://www.mtosmt.org/issues/mto.05.11.4/mto.05.11.4.garcia.html) | FT\* | Analitica |
| [McNamara e Arthur](https://ccml.gtcmt.gatech.edu/projects/McNamara_Flow_2022.html) | FT\* | Pagina di progetto; N e statistiche non riportati |

Documenti del progetto letti direttamente: [ADR-0007](../adr/0007-m2-5-entry-decisions.md), [proposta M2.5](M2_5_Entry_Music_Quality_Proposal.md), [ascolto del 2026-10-28](../m2_4_human_listening_2026-10-28.md), [ascolto del 2026-11-03](../m2_4_human_listening_2026-11-03.md), [dossier delle evidenze](../m2_procedural_evidence.md), [progress](../progress.md), [specifica](../../CYMATICA_Specifica_Agentica_Sviluppo.md), [DESIGN](../../DESIGN.md), [misure d'insieme](experiments/m25_ensemble_baseline/README.md).

**Misure nostre, script.** Tutti in [m25_ensemble_baseline](experiments/m25_ensemble_baseline/README.md), con il proprio output. I valori armonici di questo dossier vengono da `vertical_check.py` e `ctnctr_check.py`; `vettore_corpo_registers.py` conserva l'artefatto corretto nella tabella delle incongruenze.

## Appendice B. Ciò che non è stato letto

| Tipo | Elementi |
|---|---|
| Libri e tesi | Huron, *Voice Leading* e *Sweet Anticipation*; Bregman, *Auditory Scene Analysis*; London, *Hearing in Time*; Lerdahl e Jackendoff; Moore, *An Introduction to the Psychology of Hearing*; Izhaki, *Mixing Audio*; Senior, *Mixing Secrets*; Butler, *Unlocking the Groove*; Sethares, *Tuning, Timbre, Spectrum, Scale*; Margulis, *On Repeat*; Schoenberg, Frisch, Haimo, Meyer; Collins, *Game Sound*; Phillips; Sweet; Thomas; Stevens e Raybould; tesi di Plut; tesi di Aspromallis |
| Articoli | Tatar e Pasquier 2019, testo integrale; Blackwell e Bentley 2002; Hutchings e McCormack 2017; Witek et al. 2014 su *Music Perception*; Fitch e Rosenfeld 2007; Moore e Gockel 2012; Huron 1989, 2001, 2008; Wright e Bregman 1987; Tymoczko 2006; Temperley 2007; Nobile 2015; Foote 2000; Abdallah e Plumbley 2009; Boden 1990 e 2004; Wiggins 2006; Pearce e Wiggins; Sethares 1993; Berlyne; Bornstein 1989; Montoya et al. 2017; Herremans VNS |
| Norme e video | ISO 226, risposta 403; talk GDC di Gordon, Flury e su Thumper, solo video; talk Nordic Game su Hellsinger, HTTP 404 |
| Parti non lette | Ultimi 40.053 caratteri del brevetto iMUSE; ultimi 57.305 di Medina-Gray; sezioni I–V di Hutchings; il resto di Booth e FATE; Volumi II–III di Dutta; gran parte di Worrall fuori dalle parti citate |
| Accesso fallito | Figshare per lo studio su DOOM; PDF della tesi di Plut, protetto da pagina anti-bot non aggirata; NPR su Sayonara Wild Hearts; Springer per EvoMUSART, Egermann e Povel; PMC e Royal Society per Moore e Gockel; kb.osu.edu e CORE per Huron 2008; Project MUSE per Ariza; cell.com per Cheung; documentazione ufficiale Wwise, risposta 403, per la nota sullo sviluppo; web.archive.org |

## Appendice C. Ciò che le note escludono come evidenza

Questi elementi compaiono nelle ricerche, ma non sono usati qui come evidenza. La tesi di Aspromallis con FATE preferito a un crossfade, nota solo da un riassunto. Il divario accademia-industria della tesi di Plut e la survey di Plut e Pasquier (A34). Red Dead Redemption e l'Intensity Director del remake di Dead Space, noti solo da riassunti. Una mappa stati-musica di DOOM (2016): nessuna descrizione primaria. Il funzionamento dei sequencer di Thumper e Sayonara Wild Hearts. Il thread su `SetRandomSeed` non identificato. L'equivalenza fra brevetto Hellsinger e gioco uscito. Il mascheramento verso l'alto, senza attribuzione affidabile. Ripartizioni di frequenza attribuite a Moore, Izhaki o Senior. Welker, Halpern, Rosner e Meyer, Prince e Tan sul patchwork, comparsi solo in riassunti automatici. I contenuti di Meyer. La motivazione del pianoforte di *Breath of the Wild*. I risultati sul tempo di Cassidy e MacDonald, noti solo da un blog. La cifra sulle repliche minime dei disegni a trattamenti alternati. La definizione di E1 di Povel e Essens. Valori numerici di ISO 226.

## Appendice D. DOI e URL non verificati

| Fonte | Stato |
|---|---|
| [Harrison e Pearce 2020](https://doi.org/10.1037/rev0000169) | DOI riportato a memoria, non verificato |
| [Povel e Essens 1985](https://doi.org/10.2307/40285311) | DOI riportato a memoria, non verificato |
| [Bharucha 1984](https://doi.org/10.1016/0010-0285%2884%2990018-5) | DOI riportato a memoria, non verificato |
| Chowning 1973, voce AES | URL AES non verificato; letta la copia dell'Università del Michigan |
| Wright e Bregman 1987 | DOI ripreso da una bibliografia, non letto; non citato come evidenza |
| Agres et al. | Sede e anno non confermati |
| Plut e Pasquier 2020 (A34) | DOI non verificato; non citata |
