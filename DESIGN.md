# CYMATICA — Game, Audio and Visual Design

**Documento:** design bible operativa  
**Versione:** 0.2 — proposta revisionata  
**Data:** 2026-09-23  
**Stato:** pre-produzione / vertical slice  
**Specifica tecnica correlata:** `CYMATICA_Specifica_Agentica_Sviluppo.md`  
**Regole agentiche:** `AGENTS.md`

---

## 0. Ruolo del documento

`DESIGN.md` è la fonte normativa per:

- esperienza del giocatore;
- regole di gameplay percepite;
- identità musicale;
- archetipi cimatici;
- visual language;
- motion language;
- HUD e menu;
- leggibilità;
- accessibilità;
- difficoltà e pacing percepito.

La specifica tecnica decide **come** implementare questi requisiti. Questo documento decide **che cosa deve percepire e capire il giocatore**.

Quando manca un dettaglio tecnico, non aggiungerlo qui. Quando una soluzione tecnica modifica il comportamento percepito, aggiornare entrambi i documenti.

---

### 0.1 Stato e ambito della revisione

Il repository di partenza è documentale. Questa versione mantiene identità, archetipi e linguaggio artistico; precisa le promesse verificabili e i gate di design. Le modalità future non sono feature già implementate. Infinite Resonance è il focus M0–M7; Generated/Custom appartengono a M9–M11. Training è un preset di test in M5/M6; menu e progressione dedicati richiedono scope esplicito.

Il termine cimatica indica qui ispirazione artistica a figure nodali, non simulazione fisica sperimentalmente validata. Fairness significa minacce leggibili e una risposta praticabile entro il profilo dichiarato; non salvataggio automatico da ogni errore del giocatore.

## 1. High concept

> CYMATICA è un musical bullet hell in cui il suono diventa materia. Ritmo, armonia, tensione e dissonanza generano una piastra viva di nodi, onde, colori e minacce. Il giocatore non attraversa un livello accompagnato dalla musica: attraversa la musica stessa.

Il gioco deve produrre la sensazione che:

- ogni forma sia causata da un suono;
- ogni pericolo possa essere letto musicalmente;
- ogni movimento del giocatore alteri la risonanza del sistema;
- ogni run sia irripetibile ma non arbitraria;
- la difficoltà emerga dalla trasformazione della musica, non da numeri invisibili;
- il fallimento sia una perdita di armonia, non soltanto di punti vita.

---

## 2. Player fantasy

Il giocatore controlla il **Seme**, una frequenza pura intrappolata in una piastra instabile. Non è un’astronave e non “spara” nel senso tradizionale. Sopravvive accordandosi al campo, attraversando la materia in cambio di fase, sfiorando il pericolo e liberando la Risonanza accumulata.

La fantasia centrale è:

> comprendere un sistema caotico abbastanza bene da danzare al suo interno.

Il giocatore esperto non deve apparire soltanto veloce. Deve sembrare sincronizzato con la piastra: anticipa, attraversa, sfiora e libera energia nel momento corretto.

---

## 3. Promessa al giocatore

Ogni brano o processo generativo deve produrre una personalità giocabile.

- Una struttura elettronica diventa precisione, griglia e scatto.
- Una composizione orchestrale diventa flusso, marea e traiettoria.
- Una massa distorta diventa frattura, impatto e instabilità.
- Un ambiente rarefatto diventa navigazione lenta e pressione diffusa.
- Una poliritmia diventa sovrapposizione, rotazione e anticipazione.

La stessa sorgente musicale può generare varianti, ma non deve perdere il proprio carattere. La promessa non è “casualità infinita”; è **identità coerente con variazione controllata**.

---

## 4. Pilastri di design

### 4.1 Musica incarnata

Ogni evento significativo deve possedere una ragione musicale o cimatica. Un attacco non compare soltanto perché un timer è scaduto.

### 4.2 Controlli minimali, ambiente profondo

Il giocatore usa pochi verbi. La complessità nasce dalla piastra e dalle relazioni tra i sistemi.

### 4.3 Leggibilità prima dello spettacolo

CYMATICA può essere un tripudio di colore, ma il giocatore deve distinguere istantaneamente:

- sé stesso;
- pericolo;
- telegraph;
- struttura;
- opportunità;
- sfondo.

### 4.4 Rischio volontario

Il gioco deve premiare l’avvicinamento consapevole al pericolo tramite Graze e Risonanza. La difficoltà non è soltanto sopravvivere, ma scegliere quanto esporsi.

### 4.5 Sorpresa giusta

Il gioco può sorprendere con combinazioni, orientamenti e transizioni; non con hitbox nascoste, cambi di regola istantanei o pattern inevitabili.

### 4.6 Dissonanza diegetica

Danno, audio, colore e stabilità della piastra sono una sola lingua. La salute non è una barra estranea al mondo.

### 4.7 Forma, memoria, ritorno

La musica e il livello devono ricordare motivi precedenti. Una run deve avere frasi, richiami e trasformazioni, non un flusso senza memoria.

### 4.8 Minimalismo espressivo

Le forme base restano semplici. Ricchezza e identità derivano da movimento, campo, stratificazione, luce, materiale e relazione con il suono.

---

## 5. Core loop

### 5.1 Loop secondo per secondo

1. Leggere il telegraph.
2. Riconoscere ritmo, direzione e forma del pericolo.
3. Muoversi o cambiare fase.
4. Scegliere se mantenere distanza o cercare Graze.
5. Accumulare Risonanza.
6. Gestire la Dissonanza.
7. Usare Drop Shock per trasformare una situazione critica.
8. Adattarsi alla successiva metamorfosi musicale.

### 5.2 Loop di frase

1. Esposizione di una regola/pattern.
2. Variazione controllata.
3. Sovrapposizione o intensificazione.
4. Climax.
5. Recovery leggibile.
6. Ritorno trasformato o cambio di archetipo.

### 5.3 Loop di run

- apprendimento del linguaggio corrente;
- aumento della saturazione;
- espansione della varietà;
- crescente richiesta di padronanza;
- alternanza tra flow e picchi;
- collasso per Dissonanza o uscita volontaria;
- score/replay/seed e nuova interpretazione.

---

## 6. Verbi del giocatore

### 6.1 Movimento

Il movimento base deve essere immediato, preciso e privo di inerzia non richiesta, salvo modificatori chiaramente appartenenti a un archetipo.

Requisiti percettivi:

- risposta istantanea;
- hitbox più piccola della silhouette luminosa;
- trail non letale e non confondibile;
- nessuna animazione che ritardi il controllo;
- velocità leggibile e stabile.

### 6.2 Quantum Dash

Il Seme cambia fase e attraversa determinate strutture.

Funzioni:

- evasione;
- attraversamento;
- riposizionamento;
- interpretazione ritmica;
- accesso a opportunità di Graze;
- espressione dell’archetipo.

Regole percettive:

- origine, traiettoria e destinazione devono essere leggibili;
- la destinazione assistita non deve sembrare teleport automatico;
- un dash valido non termina dentro una minaccia senza preavviso;
- il cooldown deve essere leggibile sul Seme;
- l’inizio del dash risponde all’input, anche quando l’effetto è quantizzato;
- le varianti archetipiche non cambiano silenziosamente la grammatica dei controlli.

#### Gamepad

- stick sinistro: direzione;
- tap: distanza standard;
- hold: preview e modulazione;
- soft snap limitato a nodi/corridoi.

#### Mouse e tastiera

- WASD: movimento;
- puntatore: destinazione entro raggio;
- oltre raggio: clamp alla distanza massima;
- stessi costi e rischio del gamepad.

### 6.2.1 Contratto da congelare prima di M3/M4

Prima della giocabilità M3 scegliere un unico profilo player implementabile e usato anche dal validator. M5 rifinisce il game feel; non può definire retroattivamente le regole su cui M4 ha validato i pattern.

| Decisione richiesta | Criterio verificabile |
|---|---|
| Unità, arena, raggio hitbox, velocità e diagonale | Stessa distanza a parità di tempo; movimento diagonale normalizzato; aspect ratio non altera il mondo |
| Attivazione dash: press, release o boundary | Un input genera una sola azione; specificare buffering, annullamento e latenza massima |
| Tap/hold e preview | Non eseguire un dash al press per poi reinterpretarlo come hold; alternativa candidata: preview su comando separato |
| Traiettoria, durata e range | Validator e gameplay usano la stessa regola, comprese pareti e confini |
| Invulnerabilità/fase e cooldown | Definire categorie attraversabili, inizio/fine della fase e cooldown disponibile all'arrivo |
| Landing assist | Raggio massimo di correzione, validità al tempo di arrivo, fallback se non esiste landing e feedback comprensibile |
| Quantizzazione | Distinguere movimento/collisione immediati e suono quantizzato; se l'azione stessa attende, mostrare attesa e fissare limite |
| Device parity | Identico envelope fisico, assist dichiarati; verifica con utenti, non promessa di equivalenza perfetta |

Profilo candidato per lo spike: dash immediato con accento musicale successivo e preview separata. È una proposta da provare, non una decisione creativa già approvata. Fino alla scelta, nessun pattern Standard deve richiedere un dash non definito.

### 6.3 Graze

Lo sfioramento controllato carica Risonanza.

Il Graze deve:

- avere distanza visualmente intuibile;
- produrre feedback audio leggero;
- premiare rischio reale;
- evitare farming su oggetti immobili o pattern già neutralizzati;
- scalare con velocità relativa, durata e pericolosità;
- non trasformare il giocatore in inseguitore compulsivo di ogni particella.

### 6.4 Drop Shock

Scarica la Risonanza come onda quantizzata.

Possibili effetti combinati:

- conversione di proiettili in particelle innocue;
- apertura temporanea di un corridoio;
- riduzione limitata della Dissonanza;
- interruzione di un attore Vettore;
- riallineamento visuale della piastra;
- accento musicale armonizzato.

Il Drop Shock deve essere:

- raro;
- potente;
- anticipabile;
- spettacolare;
- non dominante;
- incapace di risolvere ogni climax senza preparazione.

---

## 7. Risonanza e Dissonanza

### 7.1 Risonanza

È energia ottenuta assumendo rischio e mantenendo controllo. Non è mana passivo.

Fonti:

- Graze;
- dash perfetto;
- permanenza su nodi favorevoli;
- sequenze senza collisioni;
- opportunità specifiche di pattern.

Ogni fonte va regolata con budget, cooldown e condizioni di rischio. Un nodo favorevole non deve produrre energia infinita restando fermi in sicurezza; dare carica solo in finestre/esposizioni esplicite. Identificare gli hazard con ID e generazione per evitare farming al riuso dei pool. Graze di hazard neutralizzati o durante invulnerabilità non dà carica salvo eccezione dichiarata. Per pattern di contatto continuo definire un cap per hazard/finestra.

### 7.2 Dissonanza

Sostituisce la salute tradizionale.

Quando aumenta:

- il Seme perde coesione;
- il mix si degrada con misura;
- colori e linee divergono;
- il rumore invade la piastra;
- il feedback comunica rischio crescente.

Non deve:

- rendere invisibili le minacce;
- cambiare hitbox di nascosto;
- introdurre input lag;
- sabotare il giocatore in modo irreversibile troppo presto;
- confondere VFX e collisione.

### 7.3 Stati percettivi

| Dissonanza | Stato | Presentazione |
|---:|---|---|
| [0%, 25%) | Purezza | colore pieno, suono nitido, geometria stabile |
| [25%, 50%) | Disturbo | separazione cromatica lieve, texture ruvida |
| [50%, 75%) | Instabilità | glitch più marcato, audio compresso/filtrato |
| [75%, 100%) | Collasso imminente | perdita di coesione, rumore e pulsazione critica |
| 100% (valore limitato al massimo) | Collasso | risoluzione audiovisiva e game over |

### 7.4 Recupero

Il recupero deve derivare da abilità e pacing, non soltanto dal tempo:

- dash sincronizzati;
- nodi di stabilizzazione;
- Drop Shock;
- attraversamento di recovery phrase;
- bonus per sequenze pulite.

---

## 8. Lingua musicale

### 8.1 Pulso

È impulso e scansione. Deve aiutare a prevedere.

Gameplay:

- trigger;
- onde;
- spawn ritmici;
- flash controllati;
- apertura/chiusura di finestre.

### 8.2 Corpo

È massa e struttura.

Gameplay:

- muri;
- nodi;
- corridoi;
- gravità percettiva;
- forma dell’arena.

### 8.3 Trama

È densità e atmosfera.

Gameplay:

- micro-proiettili;
- campi diffusi;
- particelle;
- pattern secondari;
- complessità visuale non sempre letale.

### 8.4 Vettore

È direzione espressiva.

Gameplay:

- inseguitori;
- entità boss-like;
- linee di attacco mirate;
- traiettorie che “cantano” una frase;
- guida dell’occhio.

### 8.5 Regola di orchestrazione

Non tutti i canali devono essere letali contemporaneamente. In ogni momento il design deve distinguere:

- canale dominante;
- supporto;
- atmosfera;
- opportunità.


---

## 9. Generazione dinamica e contratto di design

### 9.1 Non determinismo desiderato

Il giocatore deve percepire che il livello:

- risponde alla musica;
- ricorda ciò che è accaduto;
- varia senza perdere la grammatica;
- reagisce alla performance con gradualità;
- non si lascia memorizzare completamente;
- rimane leggibile.

### 9.2 Cosa può cambiare

- family e combinazione dei pattern;
- orientamento;
- densità;
- velocità entro range;
- trasformazioni;
- archetype blend;
- durata delle fasi;
- opportunità di Graze;
- quantità di recovery;
- orchestrazione musicale futura;
- intensità visuale cosmetica.

### 9.3 Cosa non può cambiare senza preavviso

- significato dei colori semantici;
- hitbox;
- direzione già telegrafata;
- regola del dash durante l’azione;
- collisione di un elemento già percepito come decorativo;
- tempo residuo di un attacco già impegnato;
- input mapping;
- condizione di morte.

### 9.4 Adattamento invisibile, non ingannevole

Il gioco non deve mostrare numeri di “AI difficulty” durante una run normale. Tuttavia, l’adattamento deve rispettare regole comprensibili:

- dopo difficoltà prolungata arriva maggiore respiro;
- una buona performance può aumentare complessità nella frase successiva;
- un singolo errore non svuota immediatamente lo schermo;
- il climax resta climax;
- la modalità Pure Seed disattiva l’adattamento per confronto.

### 9.5 Fairness perceptual contract

Ogni minaccia significativa richiede:

1. origine percepibile;
2. telegraph;
3. tempo di reazione coerente;
4. almeno una risposta praticabile;
5. conseguenza coerente;
6. risoluzione visuale chiara.

### 9.5.1 Eccezione tecnica di sicurezza

Un evento già telegrafato mantiene direzione e tempi. Se un guasto tecnico impedisce il preavviso o invalida il piano, il gioco può soltanto ridurre il pericolo o sospendere la sessione, mostrando chiaramente la neutralizzazione. Non è adattamento ordinario e non serve a cancellare un errore volontario. Registrare l'eccezione nel replay e dichiarare la run non confrontabile in Pure Seed. La sicurezza ha precedenza sul sincronismo musicale.

### 9.6 Variazione e apprendimento

Il director deve usare una progressione:

```text
presenta -> ripete -> varia -> combina -> estremizza -> recupera
```

Non introdurre una nuova regola e una nuova combinazione nello stesso istante, salvo modalità dedicate.

---

## 10. Pacing

### 10.1 Stati

| Stato | Funzione |
|---|---|
| Intro | stabilisce tonalità, spazio e input |
| Exposition | presenta un pattern leggibile |
| Build | aumenta aspettativa e densità |
| Pressure | richiede applicazione sostenuta |
| Drop | picco ritmico e visuale |
| Recovery | restituisce controllo e leggibilità |
| Variation | trasforma materiale noto |
| Climax | massima sintesi delle regole apprese |
| Resolution | chiude o prepara nuova forma |

### 10.2 Regola di respiro

La pressione non deve essere costante. Il gioco deve respirare anche nelle tracce aggressive. Recovery non significa inattività: può offrire Graze, orientamento, colore o un motivo musicale riconoscibile.

### 10.3 Saturazione nel tempo

La piastra accumula energia:

| Fase | Identità |
|---|---|
| Cristallina | linee pure, una regola dominante |
| Eccitazione | più interazioni e trasformazioni |
| Turbolenza | combinazioni, instabilità e micro-frammenti |
| Trascendenza | climax raro, piena identità dell’archetipo |

La saturazione non autorizza pattern unfair.

### 10.4 Infinite mode

Una sessione lunga deve alternare cicli macro, non crescere linearmente fino all’impossibile. Dopo un climax, il sistema può:

- modulare tonalità;
- cambiare archetipo;
- ridurre densità;
- introdurre una nuova grammatica;
- mantenere un livello di base superiore, ma sostenibile.

---

## 11. Modalità

### 11.1 Infinite Resonance

Esperienza principale iniziale.

- musica e livello in tempo reale;
- adattamento opzionale;
- durata libera;
- seed condivisibile;
- archetype preset o evoluzione;
- score di sopravvivenza/risonanza.

### 11.2 Generated Track

- durata 1, 2 o 3 minuti;
- forma chiusa;
- seed;
- validazione completa;
- replay e condivisione;
- possibile ranking Pure Seed.

### 11.3 Custom Track

- musica dell’utente;
- livello generato offline;
- varianti seed;
- mix-only o stem;
- confidence su analisi;
- fallback per musica senza beat o senza voce.

### 11.4 Training

- selezione skill: movimento, dash, rhythm, graze, spatial planning;
- pattern family limitate;
- feedback esplicito;
- velocità e telegraph configurabili;
- nessuna penalità competitiva.

---

## 12. Archetipi cimatici

Gli archetipi sono stati fisici e comportamentali della piastra. Non sono semplici skin o generi musicali.

### 12.1 Sintetico / Matriziale

**Essenza:** precisione, quantizzazione, reticolo, controllo digitale.

**Forme:**

- griglie;
- quadrati;
- croci;
- segmenti ortogonali;
- celle;
- circuiti.

**Movimento:**

- scatti;
- velocità costante;
- blink;
- easing minimo;
- rotazioni a 90°.

**Gameplay:**

- corsie;
- pattern simmetrici;
- proiettili rettilinei;
- dash quantizzato;
- finestre precise;
- telegraph netto.

**Palette primaria:**

- sfondo nero profondo;
- cyan elettrico;
- magenta;
- giallo acido;
- bianco freddo per massima salienza.

**Audio:**

- transienti definiti;
- bassi controllati;
- sequenze;
- arpeggi;
- sidechain;
- timbri digitali con dettaglio analogico.

**Errore da evitare:** trasformarlo in generico cyberpunk pieno di glitch indistinguibili.

### 12.2 Organico / Concentrico

**Essenza:** flusso, crescita, curva, respiro.

**Forme:**

- anelli;
- ellissi;
- petali;
- spirali;
- linee di sabbia;
- vortici.

**Movimento:**

- morphing;
- espansione/contrazione;
- orbite;
- curve di Bézier;
- accelerazione musicale.

**Gameplay:**

- onde concentriche;
- corridoi curvi;
- graze continuo;
- Vettore sinuoso;
- arena che respira con la dinamica;
- glide armonico.

**Palette primaria:**

- antracite vellutato;
- oro caldo;
- verde smeraldo;
- bianco perla;
- rosa cipria come accento.

**Audio:**

- corde/modellazione fisica;
- pad respiranti;
- dinamica ampia;
- riverberi leggibili;
- fraseggio espressivo.

**Errore da evitare:** confondere morbidezza visuale con assenza di pericolo.

### 12.3 Fratturato / Caotico

**Essenza:** rottura, saturazione, impatto, materia sotto stress.

**Forme:**

- crepe;
- schegge;
- poligoni spezzati;
- linee oblique;
- masse erose;
- archi elettrici.

**Movimento:**

- rottura;
- recoil;
- jitter bounded;
- esplosioni direzionali;
- ricomposizione brusca.

**Gameplay:**

- muri distruttibili;
- shrapnel telegrafato;
- dash d’impatto;
- spazi instabili;
- rischio elevato;
- improvvisazione controllata.

**Palette primaria:**

- ferro bruciato;
- rosso lava;
- arancione incandescente;
- bianco fosforico;
- nero fuliggine.

**Audio:**

- saturazione;
- distorsione multibanda controllata;
- percussioni fisiche;
- rumore con pitch/ritmo;
- transienti aggressivi.

**Errore da evitare:** usare casualità pura; anche il caos deve avere direzione e telegraph.

### 12.4 Etereo / Sospeso

**Essenza:** spazio, deriva, lentezza, pressione invisibile.

**Forme:**

- macro-masse;
- nebulose;
- membrane;
- gradienti;
- filamenti;
- vuoti ampi.

**Movimento:**

- drift;
- inerzia;
- parallax lento;
- morphing continuo;
- pulsazione profonda.

**Gameplay:**

- ostacoli grandi e lenti;
- nebbia di Dissonanza;
- dash lungo e raro;
- pianificazione;
- danno progressivo;
- correnti sicure.

**Palette primaria:**

- blu notte;
- indaco;
- viola spettrale;
- verde bioluminescente;
- bianco lattiginoso.

**Audio:**

- granular texture;
- drone;
- sub armonico;
- riverbero lungo ma non fangoso;
- eventi radi ad alta salienza.

**Errore da evitare:** farlo diventare una modalità passiva o priva di tensione.

### 12.5 Sincopato / Spostato

**Essenza:** disallineamento, poliritmia, prospettiva, sorpresa metrica.

**Forme:**

- assi inclinati;
- piani sovrapposti;
- frammenti cubisti;
- poligoni asimmetrici;
- griglie ruotate;
- coppie di flussi.

**Movimento:**

- rotazioni su accenti;
- mirror;
- phase shift;
- velocità sovrapposte;
- cambi di prospettiva controllati.

**Gameplay:**

- pattern in metri differenti;
- Vettore sul levare;
- dash ritardato/anticipato;
- due flussi leggibili;
- cambi di asse;
- sfida cognitiva.

**Palette primaria:**

- ottanio;
- corallo;
- lime;
- arancione bruciato;
- crema/carta come possibile fondo raro.

**Audio:**

- sincopi;
- polymeter;
- timbri acustici/elettronici misti;
- accenti secchi;
- spazio tra gli eventi.

**Errore da evitare:** affidare la difficoltà a tempi illeggibili senza segnali visivi.

### 12.6 Blend archetipico

Il blend deve essere percepito come trasformazione, non confusione.

Regole:

- un archetipo mantiene la grammatica primaria;
- il secondario influenza forma, pattern o timbro;
- il dash conserva una regola primaria per intervallo;
- palette e motion segnalano la transizione;
- il blend completo richiede una frase di preparazione.


---

## 13. Visual language

### 13.1 Principio generale

Il minimalismo riguarda la chiarezza delle forme, non la quantità di esperienza. La scena può essere ricca, ma deve essere costruita per layer semantici.

### 13.2 Layer semantici

Ordine percettivo:

1. **Player / Seme** — sempre identificabile.
2. **Danger active** — massima priorità.
3. **Telegraph** — anticipazione distinta dal pericolo attivo.
4. **Solid structure** — muri e nodi.
5. **Opportunity** — graze, recovery, resonance node.
6. **Context** — campi e geometria musicale.
7. **Atmosphere** — particelle, bloom, texture.
8. **Background** — mai competitivo con il gameplay.

### 13.3 Semantica del colore

La palette cambia per archetipo, ma la funzione deve restare coerente.

| Funzione | Tratto obbligatorio |
|---|---|
| Player | nucleo chiaro, profilo unico, pulsazione propria |
| Danger | contrasto alto, bordo o motion distintivo |
| Telegraph | stessa famiglia della minaccia, minore opacità/forma incompleta |
| Structure | massa stabile, frequenza visuale inferiore |
| Opportunity | ritmo e segno positivo distinti, non solo verde |
| VFX | contrasto inferiore e nessuna silhouette simile a bullet |

Il colore non è l’unico codice. Usare anche:

- forma;
- bordo;
- frequenza di pulsazione;
- direzione;
- texture;
- suono;
- icona diegetica.

### 13.4 Materiali

Il mondo non è flat UI. Anche con primitive semplici, ogni archetipo possiede una qualità materiale:

- Sintetico: luce vettoriale e circuito;
- Organico: sabbia fine, liquido e metallo caldo;
- Fratturato: vetro, roccia, plasma e cenere;
- Etereo: nebbia, membrana e bioluminescenza;
- Sincopato: carta, smalto, collage e piani luminosi.

### 13.5 Background

Il background deve rispondere alla musica senza aggiungere falsi hazard. Può mostrare:

- interferenze cimatiche a bassa frequenza;
- gradienti;
- polvere;
- eco di pattern;
- anelli di fase;
- memoria visuale della run.

### 13.6 Contrasto dinamico

Durante fasi dense:

- ridurre dettaglio del background;
- attenuare particelle non necessarie;
- aumentare separazione tra telegraph e hazard;
- limitare bloom;
- conservare il profilo del Seme.

Lo spettacolo deve adattarsi alla leggibilità, non il contrario.

---

## 14. Threat grammar e telegraph

### 14.1 Ciclo visuale di una minaccia

```text
Dormant -> Telegraph -> Active -> Dissolve/Impact -> Residue
```

Ogni fase ha una rappresentazione distinta.

### 14.2 Telegraph

Un telegraph deve comunicare:

- origine;
- direzione;
- area;
- tempo;
- tipo di risposta attesa quando rilevante.

Esempi:

- linea sottile che si carica prima del laser;
- sabbia che migra verso un nodo prima del muro;
- anello trasparente prima dell’onda;
- scia vettoriale prima di una carica;
- frattura luminosa prima dello shrapnel.

### 14.3 Durata

La durata dipende da:

- velocità;
- simultaneità;
- distanza;
- familiarità del pattern;
- preset difficoltà;
- accessibilità;
- ritmo.

Non può scendere sotto il minimo di fairness stabilito dal validator.

### 14.4 Pattern combinati

Quando due pattern sono sovrapposti:

- almeno uno deve essere già noto;
- telegraph distinti per forma o pulsazione;
- evitare linee coincidenti ambigue;
- limitare flussi simultanei;
- preservare una strategia leggibile.

### 14.5 Impatto

L’impatto deve comunicare collisione senza nascondere il tick successivo:

- freeze-frame minimo o assente in bullet hell denso;
- feedback sul Seme;
- impulso sonoro localizzato;
- Dissonanza visibile;
- invulnerabilità post-hit chiaramente segnalata se presente;
- VFX che decadono rapidamente vicino all’hitbox.

---

## 15. Motion language

### 15.1 Il movimento è semantico

- moto rettilineo = precisione/sintetico;
- curva = organicità/espressione;
- rottura = frattura;
- drift = etereo;
- phase shift/rotazione = sincopato.

### 15.2 Easing

Non applicare easing decorativo a entità con collisione se altera la previsione. Il movimento visuale e quello autoritativo devono coincidere.

### 15.3 Screen shake

- solo per accenti importanti;
- ampiezza limitata;
- disattivabile;
- non sposta la collisione;
- ridotto automaticamente in pattern densi;
- niente rumore continuo.

### 15.4 Zoom e camera

La camera resta prevalentemente stabile. Zoom, rotazione o deformazione devono:

- essere rari;
- appartenere a frase/climax;
- preservare coordinate percepite;
- non alterare input mapping;
- avere profilo accessibilità.

### 15.5 Quantum Dash motion

- compressione del Seme all’origine;
- traccia di fase;
- transizione cromatica complementare;
- ricomposizione netta;
- indicazione del cooldown;
- nessuna scia simile a un hazard.

### 15.6 Drop Shock motion

L’onda agisce come riallineamento della materia:

- espansione concentrica o archetype-specifica;
- conversione cromatica delle minacce;
- particelle riassorbite nei nodi;
- impulso sincronizzato;
- breve finestra di chiarezza dopo l’esplosione.

---

## 16. Audio design

### 16.1 Obiettivo

La musica procedurale non deve sembrare un accompagnamento MIDI casuale. Deve avere:

- forma;
- motivi riconoscibili;
- contrasto;
- sound palette coerente;
- mix stabile;
- micro-variazione;
- ritorni;
- finalità ludica.

### 16.2 Gerarchia sonora

1. eventi musicali strutturali;
2. telegraph gameplay;
3. feedback player;
4. impatti/pericoli;
5. ambiente e texture.

Durante densità elevata, il mix deve creare spazio per telegraph e feedback.

### 16.3 SFX quantizzati

Input come dash e Drop Shock possono produrre un suono quantizzato, ma il feedback tattile iniziale deve essere immediato. Separare:

- acknowledgement immediato;
- risoluzione musicale sul boundary.

### 16.4 Dissonanza audio

Strumenti possibili:

- filtro;
- bit reduction moderata;
- saturazione;
- rumore armonizzato;
- wow/flutter;
- stereo instability;
- perdita selettiva di layer.

Non usare volume estremo o frequenze dolorose per comunicare pericolo.

### 16.5 Hit feedback

Un colpo deve essere chiaramente distinguibile dalla musica ma integrato tonalmente. Evitare esplosioni generiche sovrapposte.

### 16.6 Dynamic mix

- sidechain per preservare Pulso e telegraph;
- ducking localizzato sugli SFX importanti;
- limitazione del low-end;
- headroom per Drop/Drop Shock;
- loudness coerente tra archetipi;
- profilo “reduced intensity”.

### 16.7 Silenzio

Il silenzio o la rarefazione sono strumenti di design. Prima di un climax, una sottrazione breve può aumentare leggibilità e aspettativa.

---

## 17. HUD diegetico

### 17.1 Principio

Le informazioni principali vivono sul Seme e sulla piastra. Tuttavia, diegetico non significa criptico: un HUD esplicito alternativo deve essere disponibile.

### 17.2 Stati sul Seme

- Dissonanza: coesione e aberrazione;
- Risonanza: luminosità interna;
- dash cooldown: anello/segmento orbitale;
- Drop Shock pronto: pattern interno completo;
- invulnerabilità: fase distinta, non sola trasparenza.

### 17.3 Tempo/progressione

- Generated/Custom: anello perimetrale o waveform circolare;
- Infinite: indicatore di ciclo/saturazione, non countdown;
- Vettore boss-like: stato integrato nell’entità, non barra tradizionale se evitabile.

### 17.4 Debug vs player HUD

Le metriche AI, reachability e score restano debug. Non esporle nel gioco normale.

### 17.5 HUD esplicito opzionale

Accessibilità:

- barra Dissonanza;
- barra Risonanza;
- icona dash;
- beat indicator;
- warning testuale/sonoro;
- dimensione e opacità configurabili.

---

## 18. Menu e navigazione

### 18.1 Resonance nodes

Il menu principale può essere una piastra interattiva: il Seme raggiunge nodi che rappresentano modalità e opzioni.

Requisiti:

- selezione reversibile;
- testo leggibile;
- focus controller/tastiera;
- conferma esplicita per azioni distruttive;
- percorso rapido per chi non vuole navigare fisicamente;
- nessun tempo di attesa obbligatorio.

### 18.2 Menu fallback

Deve esistere un menu tradizionale accessibile per:

- screen reader futuro;
- motricità ridotta;
- navigazione rapida;
- debug;
- impostazioni complesse.

### 18.3 Flow principale

```text
Boot
-> Title Resonance
-> Mode
-> Archetype / Source / Difficulty
-> Calibration optional
-> Run
-> Collapse/Exit
-> Results + Seed + Replay
-> Retry / Transform / Back
```

### 18.4 Results

Mostrare:

- durata;
- score;
- purezza media;
- graze;
- dash sync;
- climax superati;
- archetipi attraversati;
- seed;
- opzione replay/retry.

Evitare di mostrare giudizi psicologici derivati dal player model.

---

## 19. Accessibilità

### 19.1 Colore

- profili daltonismo;
- contrasto regolabile;
- semantica anche tramite forma;
- test in grayscale;
- niente coppie colore come unico codice.

### 19.2 Photosensitivity

- profilo reduced flashes;
- limite a flash/strobe;
- niente inversioni full-screen ripetute;
- bloom e shake regolabili;
- warning prima di contenuti intensi;
- pattern Sintetico alternativo senza strobo.

### 19.3 Audio

- volume separato per musica, telegraph, SFX e ambiente;
- visual beat assist;
- low-frequency reduction;
- dynamic range profiles;
- nessun segnale essenziale solo audio.

### 19.4 Controlli

- remapping completo;
- hold/toggle dove applicabile;
- sensibilità stick;
- dead zone;
- cursor clamp configurabile;
- aim assist/snap configurabile;
- modalità one-button assist da studiare.

### 19.5 Difficoltà

- velocità;
- telegraph;
- densità;
- simultaneità;
- dash cooldown;
- Dissonanza per hit;
- recovery;
- adaptive strength.

Le opzioni possono ridurre difficoltà senza sottrarre l’identità musicale.

---

### 19.6 Gate di accessibilità della vertical slice

Disponibili prima della run: reduced flashes, shake disattivabile, bloom regolabile, segnali essenziali ridondanti per forma e audio/visuale, HUD esplicito, remapping e hold/toggle dove previsto. Un warning non sostituisce un profilo meno intenso.

Testare anche l'effetto combinato di più emitter sul frame completo. Come riferimento tecnico per i flash usare [W3C, Three Flashes or Below Threshold](https://www.w3.org/WAI/WCAG21/Understanding/three-flashes-or-below-threshold.html): considerare frequenza, area e luminanza, non soltanto quanti flash emette un oggetto. È un riferimento di progetto per contenuti web, non una certificazione medica del gioco né una dichiarazione automatica di conformità WCAG.

I profili solo cosmetici non cambiano collisioni, tempi o seed. Impostazioni che cambiano velocità, cooldown o telegraph costituiscono un profilo gameplay distinto e vengono registrate per replay/confronti. Supporto screen reader completo e one-button restano studi futuri, senza presentarli come disponibili.

## 20. Difficoltà, score e mastery

### 20.1 Difficoltà

La difficoltà è multidimensionale:

- precisione;
- velocità;
- pianificazione;
- ritmo;
- memoria;
- simultaneità;
- rischio;
- durata.

### 20.2 Score provvisorio

```text
score = survival
      + graze quality
      + rhythmic execution
      + resonance efficiency
      + clean phrase bonus
      + climax completion
      - dissonance penalties
```

Lo score non deve incoraggiare strategie noiose come restare in un angolo o farmare un singolo oggetto.

### 20.3 Combo

Una combo può rappresentare **Coerenza** o **Accordo**. Deve crescere con:

- beat-aligned actions;
- graze variato;
- frasi senza colpi;
- uso efficiente del Drop Shock.

### 20.4 Pure Seed

Per leaderboard o confronto:

- stesso seed e configurazione iniziale;
- stessa policy e budget logico indipendente dalla velocità hardware;
- adattamento disattivato;
- versione registrata;
- input device non deve conferire vantaggio irragionevole;
- replay verificabile.

Pure Seed promette identità a parità di input e profilo compatibile. Targeting player-relative e azioni del giocatore possono produrre traiettorie diverse fra partite. Per condividere un percorso identico occorre una timeline materializzata con regole compatibili. Registrare versione, assist, fixed tick e profilo input; una deadline mancata o un intervento tecnico interrompe l'idoneità al confronto. Nessuna leaderboard online è inclusa nella vertical slice.

### 20.5 Adaptive mode

Lo score deve dichiarare che la run è adattiva. Non confrontare direttamente con Pure Seed senza normalizzazione o categorie separate.

---

## 21. Esempi di momenti di gioco

### 21.1 Sintetico — Crossfire Drop

- Pulso stabilisce quattro battiti.
- La griglia si accende su celle alternate.
- Telegraph verticali appaiono sul terzo battito.
- Il giocatore si sposta o prepara un dash.
- Sul downbeat, due corsie diventano letali.
- Trama aggiunge proiettili rettilinei nei vuoti.
- Vettore effettua un blink e chiude una fuga secondaria.
- Una cella resta chiaramente raggiungibile.
- Il Drop Shock sul beat converte la matrice in frammenti luminosi.

### 21.2 Organico — Crescendo Tide

- Corpo genera due ellissi lente.
- Il crescendo comprime lo spazio.
- Trama produce gocce orbitanti.
- Il giocatore segue la curva in Graze continuo.
- Vettore attraversa l’arena come frase di violino.
- La safe route ruota gradualmente, non scompare.
- Il diminuendo espande la piastra e ripaga recovery debt.

### 21.3 Fratturato — Controlled Break

- Una crepa viene telegrafata da luce interna.
- Il muro si rompe sul rullante.
- Le schegge seguono coni prevedibili.
- Il giocatore può dashare attraverso il punto di rottura.
- Il rischio produce alta Risonanza.
- Il caos visuale decade prima del pattern successivo.

### 21.4 Vivaldi-like Organico/Sincopato

- Corpo segue il basso continuo con linee curve stabili.
- Trama traduce arpeggi in petali mobili.
- Vettore segue il solista con una traiettoria elegante.
- I contrasti orchestrali ruotano temporaneamente l’asse.
- La difficoltà deriva da densità e fraseggio, non da una cassa assente.

---

## 22. Do / Don’t

### Do

- far derivare le forme dalla musica;
- presentare prima di combinare;
- usare colore con funzione;
- conservare una via leggibile;
- alternare pressione e respiro;
- trasformare motivi noti;
- far sentire il Seme sincronizzato;
- rendere il fallimento audiovisivamente coerente;
- supportare profili accessibili.

### Don’t

- generare proiettili solo perché “servono difficoltà”;
- nascondere hazard nelle particelle;
- usare glitch come scusa per illeggibilità;
- cambiare controlli senza telegraph;
- punire il giocatore immediatamente dopo un errore con ulteriore caos;
- rendere ogni layer letale;
- confondere random con creativo;
- basare il gioco su strobe;
- sacrificare musicalità per quantità.

---

## 23. Definition of done della vertical slice

La vertical slice è design-complete quando include:

- Infinite Resonance;
- 60–90 secondi di esperienza significativa, più sessione estesa di almeno 10 minuti secondo il gate tecnico M7;
- Seme, movimento, Quantum Dash, Graze, Risonanza, Drop Shock e Dissonanza;
- musica procedurale a quattro ruoli;
- Sintetico e Organico completi;
- un terzo archetipo parziale;
- almeno cinque pattern family;
- pacing con exposition, build, climax e recovery;
- due run distinguibili ma leggibili;
- HUD diegetico e fallback esplicito;
- profilo reduced flashes;
- gamepad e mouse/tastiera;
- results con seed;
- nessun pattern percepito come inevitabile nel playtest previsto.

Prima di dichiarare il gate superato, documentare protocollo, partecipanti e dispositivi, seed/preset, compiti, osservazioni e limiti del campione. Distinguere errori di controllo, mancata lettura del telegraph, difficoltà intenzionale e bug. I bot non sostituiscono la verifica umana. Soglie e criteri di stop del playtest vanno fissati prima della valutazione, non dopo aver visto i risultati.

Il test decisivo è:

> Il giocatore descrive ciò che è accaduto in termini musicali e spaziali, non come una successione casuale di proiettili.

---

## 24. Open design questions

Le domande sono gate di design, non autorizzazione agli agenti a introdurre risposte permanenti. Congelare entro M3 le regole necessarie al movimento e alla collisione; entro M5 risorse/score e prima di M7 il resto della vertical slice. Il profilo scelto deve indicare valori iniziali, unità, motivazione e criteri di tuning.

1. Il Seme può attaccare direttamente o soltanto alterare il campo? — M5; l'attacco diretto non è requisito dello spike.
2. Drop Shock riduce Dissonanza o soltanto pressione? — M5; fissare costo, attesa massima, target neutralizzabili, effetto durante pausa/danno e ordine rispetto a collisioni.
3. Il dash quantizzato attende il boundary o esegue subito con risoluzione musicale successiva? — prima di M3/M4, secondo §6.2.1.
4. Quanto può cambiare il comportamento del dash tra archetipi?
5. Gli archetipi sono selezionati dal giocatore o emergono sempre dalla musica?
6. Quanti layer letali simultanei sono accettabili per ciascun preset?
7. Qual è il rapporto tra survival score e Graze score?
8. Infinite Resonance ha obiettivi intermedi, boss o soltanto cicli?
9. Il Vettore può essere neutralizzato o soltanto evitato?
10. Quale linguaggio visuale distingue nodi sicuri, muri e opportunità?
11. Come comunicare l’adattamento senza esporre il player model?
12. Come verificare le opzioni obbligatorie di §19.6 sul contenuto finale? — M7; il loro perimetro minimo è già definito.
13. CYMATICA è titolo definitivo o nome progetto?
14. Quanto danno per hit, quali hazard consumabili, quale invulnerabilità post-hit e quale ordine per colpi simultanei? — M5, fixture anche sui confini 25/50/75/100%.
15. Quali cap e cooldown per Graze/nodi, e come impedire una carica infinita senza rischio? — M5.
16. Quale formula di score intera/versionata, quali pesi e tie-break? — M5; risultati provvisori non sono una classifica comparabile fra build.

---

## 25. Direzione finale

CYMATICA deve apparire come un’opera astratta che ha imparato a difendersi. La piastra non è uno sfondo, la musica non è una colonna sonora e il colore non è decorazione: sono lo stesso sistema visto da prospettive diverse.

Il design deve sempre perseguire questa triade:

```text
musicalità + leggibilità + trasformazione
```

Quando una feature aumenta soltanto il caos, ma non migliora almeno due elementi della triade, non appartiene al core di CYMATICA.

