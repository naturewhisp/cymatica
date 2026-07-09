# CYMATICA — Specifica completa per sviluppo agentico

**Documento:** Specifica tecnica-operativa pronta per avvio prototipo  
**Versione:** 0.3  
**Data:** 2026-07-08  
**Target primario:** Windows  
**Target secondario da preservare:** Android  
**Approccio consigliato:** sviluppo agentico incrementale con Google Antigravity 2.0 o IDE equivalente  
**Stack core raccomandato per prototipo:** C++20, CMake, raylib, miniaudio, shader GLSL  
**Stack differito:** ONNX Runtime, FFmpeg, stem separation, editor offline avanzato, Android technical preview  
**Strategia repository:** monorepo modulare con distribuzioni separate per gioco e tool

---

## 0. Changelog

### 0.3 — 2026-07-08

Questa revisione separa la specifica tecnica dalle istruzioni operative per agenti:

- rimossa dal documento la sezione con prompt operativi per Antigravity;
- creato un file `AGENTS.md` separato, completo e pensato come contratto operativo per Google Antigravity 2.0 o IDE agentici equivalenti;
- mantenuta nel documento solo una breve nota di collegamento verso `AGENTS.md`;
- aggiornato il decision log con la scelta di spostare le regole operative degli agenti fuori dalla specifica;
- confermata la priorità alla Milestone 0 e Milestone 1 senza ONNX, FFmpeg, Strudel runtime, Android e motori fisici esterni.

### 0.2 — 2026-07-08

Questa revisione integra le decisioni e considerazioni emerse dopo la prima specifica:

- confermata la priorità alla **musica procedurale** nella fase iniziale;
- rimandati tool avanzato, stem separation e import custom a milestone successive;
- definita strategia **monorepo con target e distribuzioni separate**;
- aggiunto capitolo dedicato a **dipendenze, prerequisiti e ambiente di sviluppo**;
- aggiunta policy per dipendenze C++ senza package manager applicativo;
- aggiunto uso possibile di **Strudel** come laboratorio/research track, non come dipendenza runtime;
- aggiunto sistema **particellare custom** come modulo separato da bullet e fisica;
- esplicitato che un motore fisico esterno non è necessario nella Milestone 1;
- aggiornati roadmap, backlog e decision log.

---

## 1. Executive summary

CYMATICA è un gioco **musical bullet hell** basato sulla **cimatica**: la musica non accompagna il livello, ma diventa la fonte fisica delle forme, dei colori, delle minacce e della struttura dell’arena.

Il giocatore controlla un’entità luminosa, il **Seme**, dentro una piastra vibrante. Le frequenze musicali generano linee nodali, antinodi, onde d’urto, proiettili, fratture, particelle e anomalie. Il gameplay deve rimanere minimale nei controlli, ma ricco nella variazione ambientale: invece di aggiungere molte azioni al giocatore, il gioco cambia comportamento in base alla musica e all’archetipo cimatica attivo.

Il progetto prevede tre famiglie di contenuti:

1. **Musica procedurale infinita**  
   Sintesi audio in tempo reale con survival continuo. È la modalità prioritaria per il vertical slice.

2. **Musica procedurale finita**  
   Tracce generate offline o semi-offline da 1, 2 o 3 minuti, con timeline nota, dati strutturati e livello esportabile.

3. **Musica custom importata**  
   Brani dell’utente convertiti in pacchetti di livello tramite tool offline. Il tool analizzerà il brano, produrrà stem o canali funzionali, estrarrà parametri musicali e genererà una timeline giocabile.

La priorità non è realizzare subito tutte le modalità. Il primo obiettivo è costruire una **vertical slice Windows** che dimostri:

- audio procedurale stabile;
- rendering cimatica in tempo reale;
- movimento del Seme;
- Quantum Dash;
- Dissonanza;
- sistema di proiettili leggibile;
- sistema particellare visivo di base;
- almeno due archetipi musicali;
- loop di gameplay divertente per 60–90 secondi.

---

## 2. Valutazione del brainstorming

### 2.1 Punti forti

Il brainstorming ha una base molto solida.

**1. La cimatica è funzionale, non decorativa.**  
Molti rhythm game usano la musica come timeline o trigger. CYMATICA può usare la musica come modello fisico: nodi, antinodi, risonanza, frequenza, dissonanza e saturazione diventano regole di gameplay.

**2. Le modalità diverse hanno senso ludico.**  
Musica custom, procedurale finita e procedurale infinita non sono semplici varianti di contenuto:

- custom = scoperta e rigiocabilità su brani personali;
- procedurale finita = livello generato ma validabile;
- infinito = survival generativo reattivo.

**3. I quattro canali funzionali risolvono il problema degli stem.**  
La divisione in **Pulso, Corpo, Trama, Vettore** è migliore di una divisione rigida drums/bass/vocals/other, perché funziona anche con brani strumentali e musica procedurale senza voce.

**4. Gli archetipi creano varietà senza moltiplicare i controlli.**  
Il gioco modifica le regole ambientali in base alla natura musicale della traccia. Questo consente complessità progressiva senza sovraccaricare il giocatore con troppe azioni.

**5. La Dissonanza è un’ottima barra salute diegetica.**  
Se il giocatore sbaglia, non “perde solo vita”: la musica e la piastra si degradano. Questo lega errore, feedback audio, feedback visivo e difficoltà.

### 2.2 Rischi principali

#### Rischio 1 — Scope eccessivo

Bullet hell, rhythm game, sintesi procedurale, cimatica, source separation, tool offline, editor livelli, Android e sviluppo agentico sono troppi assi di complessità per la prima fase.

**Mitigazione:** partire da modalità procedurale infinita, vertical slice Windows, niente import custom, niente ONNX, niente editor GUI, niente Android reale nella prima milestone.

#### Rischio 2 — Qualità audio procedurale

La musica generata in tempo reale rischia di sembrare finta, piatta o ripetitiva.

**Mitigazione:** evitare all’inizio una “AI composer” completa. Implementare un generatore ibrido con ritmi euclidei, scale modali, layering, sidechain, saturazione controllata, filtri, micro-variazioni e sound design curato.

#### Rischio 3 — Leggibilità visuale

Un tripudio di colori e particelle può diventare illeggibile in un bullet hell.

**Mitigazione:** separare sempre elementi letali, elementi informativi, effetti estetici, preavvisi/telegraph e feedback di stato. La leggibilità viene prima dello spettacolo.

#### Rischio 4 — Fairness tra gamepad e mouse/tastiera

Il Quantum Dash rischia di essere troppo preciso con mouse e troppo approssimativo con controller.

**Mitigazione:** definire un modello coerente:

- controller = direzione analogica + distanza modulata/quantizzata;
- mouse = destinazione puntata entro raggio massimo;
- entrambi = safe landing assist, telegraph e cooldown comparabili.

#### Rischio 5 — Importazione musica custom

Stem separation, beat tracking e analisi MIR sono complessi e possono produrre risultati imperfetti.

**Mitigazione:** il gioco non deve dipendere da stem perfetti. Il tool offline deve produrre canali funzionali e confidence score. Se lo stem vocale è assente o debole, il Vettore viene derivato da lead/pitch/energia nello stem Other o nel mix.

#### Rischio 6 — Android e bassa latenza audio

Android è critico per rhythm game e audio interattivo.

**Mitigazione:** preservare l’architettura multi-platform, ma validare prima Windows. Per Android prevedere test di latenza dedicati e backend/configurazioni specifiche solo dopo stabilizzazione del core.

#### Rischio 7 — Dipendenze C++ non tracciate

Non usando un sistema a pacchetti applicativo come npm, le dipendenze potrebbero diventare ambigue: alcune saranno scaricate da CMake, alcune vendorizzate, altre installate a livello di sistema.

**Mitigazione:** introdurre un capitolo formale `docs/dependencies.md`, mantenere una tabella dipendenze, definire chi scarica cosa e bloccare l’introduzione di nuove librerie senza aggiornamento documentale.

---

## 3. Visione di prodotto

### 3.1 High concept

> CYMATICA è un bullet hell musicale in cui la musica diventa materia: frequenze, armoniche e dissonanze generano un’arena cimatica viva, colorata e pericolosa.

### 3.2 Promessa al giocatore

Ogni traccia produce una forma di sfida diversa.

Una traccia EDM diventa una matrice ritmica letale. Un brano barocco diventa un organismo fluido di curve, archi e vortici. Una traccia metal frattura la piastra. Un brano ambient trasforma il gioco in navigazione ipnotica tra masse lente. Una traccia jazz/progressive crea poliritmie e disallineamenti percettivi.

### 3.3 Pilastri di design

1. **Musica incarnata**  
   Ogni evento visivo o meccanico deve derivare da una proprietà musicale o da una regola di cimatica.

2. **Controlli minimali, profondità ambientale**  
   Il giocatore ha pochi input ma deve leggere un ambiente complesso e mutevole.

3. **Non-determinismo controllato**  
   La stessa traccia deve mantenere identità musicale ma generare variazioni di pattern, seed e minacce.

4. **Leggibilità prima dello spettacolo**  
   Colore, particelle e shader devono aumentare l’immersione senza compromettere la leggibilità.

5. **Sviluppo agentico modulare**  
   Il codice deve essere leggibile da agenti: moduli piccoli, contratti chiari, test automatici, dati testuali, poche dipendenze opache.

---

## 4. Modalità di gioco

### 4.1 Modalità Infinite Resonance

Modalità prioritaria per il primo prototipo.

**Descrizione:**  
Musica generata in tempo reale. Il giocatore sopravvive finché riesce a mantenere bassa la Dissonanza.

**Caratteristiche:**

- durata infinita;
- progressione dinamica della difficoltà;
- generatore ritmico euclideo;
- scale modali;
- sintesi e parametri audio prodotti internamente;
- archetipi selezionabili dal giocatore;
- adattamento alla performance;
- nessuna dipendenza da tool offline.

**Obiettivo prototipo:**  
Implementare Sintetico e Organico, poi aggiungere Fratturato.

### 4.2 Modalità Generated Track

Da sviluppare dopo il vertical slice.

**Descrizione:**  
Il tool genera una traccia finita da 1, 2 o 3 minuti e produce un pacchetto completo di musica + timeline gameplay.

**Caratteristiche:**

- generazione offline o semi-offline;
- timeline completa nota prima del gameplay;
- difficoltà stimabile prima dell’avvio;
- possibile playtest automatico;
- contenuto esportabile in formato pacchetto.

### 4.3 Modalità Custom Track

Da sviluppare dopo core gameplay e formato pacchetto.

**Descrizione:**  
L’utente importa una traccia audio. Un tool offline esegue analisi musicale, eventuale separazione stem e codifica in una timeline di parametri.

**Caratteristiche:**

- import MP3/WAV/OGG;
- normalizzazione audio;
- beat tracking;
- onset detection;
- energy envelope;
- stima canali funzionali;
- eventuale source separation;
- generazione pacchetto `.cymlevel`;
- livello rigiocabile con seed diversi.

---

## 5. Modello musicale: quattro canali funzionali

Il gioco non deve dipendere da strumenti specifici. Deve usare quattro funzioni musicali astratte.

| Canale | Funzione | Origine custom possibile | Origine procedurale | Ruolo gameplay |
|---|---|---|---|---|
| **Pulso** | ritmo, attacco, transitori | kick, snare, percussioni, onset forti | sequencer euclideo | onde d’urto, trigger, telegraph ritmico |
| **Corpo** | massa, basso, gravità | basso, sub, pad bassi | bass synth, drone | linee nodali, muri, labirinto |
| **Trama** | densità, armonia, texture | synth, archi, chitarre, piano, accompagnamenti | arpeggi, pad, granular layer | micro-proiettili, pioggia energetica |
| **Vettore** | lead, espressione, dinamismo | voce, violino solista, lead synth, assolo | lead synth procedurale | boss, inseguitori, attacchi mirati |

### 5.1 Musica senza voce

La voce non è obbligatoria. Se il tool non rileva un canale vocale affidabile, il Vettore viene ricavato dal contenuto più espressivo e dominante:

- pitch saliente;
- lead strumentale;
- centroide spettrale alto;
- energia melodica;
- variazione dinamica;
- eventi ad alta confidence nello stem Other o nel mix.

### 5.2 Strudel come riferimento concettuale

Strudel può essere utile come laboratorio di prototipazione per pattern musicali procedurali, soprattutto per:

- pattern ciclici;
- poliritmie;
- trasformazioni ritmiche;
- densità variabile;
- probabilità controllata;
- live sketching delle idee musicali degli archetipi.

**Decisione:** Strudel non deve diventare dipendenza runtime del gioco nella fase iniziale. Può stare in `research/strudel-patterns/` o in un tool/laboratorio separato. Le idee valide possono essere tradotte in una mini-DSL o in un sequencer C++ nativo ispirato a Strudel/Tidal.

**Nota licenza:** qualsiasi integrazione diretta di pacchetti Strudel deve essere preceduta da license review, perché Strudel dichiara licenza AGPL-3.0. Per ora si usa come riferimento e ambiente di ricerca, non come componente distribuibile.

---

## 6. Cimatica applicata al gameplay

### 6.1 Arena: la piastra vibrante

L’arena è una piastra bidimensionale. Le forme emergono dalla vibrazione.

Concetti fisici tradotti in gioco:

| Concetto | Interpretazione gameplay |
|---|---|
| Nodo | zona stabile, linea di sabbia, struttura, muro |
| Antinodo | zona energetica, emissione proiettili, instabilità |
| Risonanza | stato favorevole, energia accumulata, potere del giocatore |
| Dissonanza | danno, distorsione audio/visiva, perdita di controllo |
| Frequenza crescente | geometria più densa e complessa |
| Saturazione | difficoltà crescente nel tempo |

### 6.2 Formula base di Chladni

Formula di riferimento:

```text
cos(n * pi * x) * cos(m * pi * y) - cos(m * pi * x) * cos(n * pi * y) = 0
```

Nel gioco non serve simulazione fisica scientificamente perfetta. Serve una funzione sufficientemente coerente da generare pattern leggibili, belli e pilotabili.

### 6.3 Regole di mapping

- `Pulso` controlla trigger, onset, flash, onde d’urto.
- `Corpo` controlla `m/n`, spessore e stabilità delle linee nodali.
- `Trama` controlla densità di micro-proiettili e particelle.
- `Vettore` controlla entità dinamiche, inseguitori e attacchi mirati.
- `Dissonanza` introduce rumore, offset, glitch e perdita di purezza.

---

## 7. Meccaniche base del giocatore

### 7.1 Movimento

Il giocatore controlla il **Seme**, una particella di luce/frequenza pura.

Requisiti:

- movimento fluido e responsivo;
- supporto gamepad e mouse/tastiera;
- arena delimitata;
- hitbox piccola e leggibile;
- trail visivo non confondibile con proiettili;
- input latency misurabile.

### 7.2 Dissonanza come salute

La Dissonanza sostituisce la vita tradizionale.

Quando il giocatore viene colpito:

- aumenta Dissonanza;
- il mix audio si degrada;
- le linee di Chladni tremano;
- aumentano glitch, aberrazione cromatica e rumore;
- sopra soglie alte, alcuni pattern diventano più instabili;
- al 100% la traccia collassa in rumore/feedback e avviene il game over.

Possibili soglie:

| Dissonanza | Stato | Effetto |
|---:|---|---|
| 0–25% | Purezza | audio limpido, visual stabile |
| 25–50% | Disturbo | leggera distorsione, jitter visivo |
| 50–75% | Instabilità | difficoltà lettura, glitch evidenti |
| 75–100% | Collasso | audio degradato, piastra instabile |
| 100% | Silenzio/Rumore | game over |

### 7.3 Graze

Il Graze premia il rischio controllato.

Il giocatore accumula Risonanza quando sfiora proiettili, muri o antinodi senza collisione.

Requisiti:

- distanza di graze chiara;
- no farming banale;
- ricompensa proporzionale al rischio;
- feedback audio sottile;
- interazione con Drop Shock.

### 7.4 Drop Shock

Il Drop Shock è l’abilità di rilascio della Risonanza.

Effetti possibili:

- pulizia temporanea della piastra;
- conversione proiettili in particelle innocue;
- reset locale della Dissonanza visiva;
- apertura corridoi;
- forte feedback audio sincronizzato al beat.

Vincoli:

- deve essere potente ma raro;
- non deve risolvere ogni situazione;
- deve essere quantizzato musicalmente;
- deve avere telegraph chiaro.

---

## 8. Quantum Dash

### 8.1 Funzione ludica

Il Quantum Dash è la meccanica centrale di mobilità avanzata.

Non è solo una schivata: il Seme cambia fase/frequenza, attraversando alcune strutture ma diventando vulnerabile ad altre.

### 8.2 Safe landing assist

Per evitare frustrazione, il dash deve calcolare destinazioni valide.

Regola:

> Se la destinazione teorica cade dentro un muro o zona invalida, il sistema accorcia o estende il dash verso il punto sicuro più vicino lungo la traiettoria, entro limiti controllati.

Questo assist non deve rendere il dash automatico: deve solo evitare fallimenti ingiusti da pochi pixel.

### 8.3 Input gamepad

- stick sinistro = direzione;
- tap dash = distanza standard;
- hold dash = preview di raggio/destinazione;
- rilascio = esecuzione;
- soft snap su nodi/corridoi vicini.

### 8.4 Input mouse e tastiera

- WASD = movimento;
- mouse = destinazione dash;
- se cursore dentro raggio massimo, destinazione esatta;
- se cursore fuori raggio massimo, dash verso massimo raggio;
- stesso cooldown e stesso rischio del gamepad.

### 8.5 Varianti per archetipo

| Archetipo | Variante dash |
|---|---|
| Sintetico | dash quantizzato su griglia/celle |
| Organico | glide armonico, più curvo e fluido |
| Fratturato | dash d’impatto, può rompere strutture ma genera schegge |
| Etereo | dash lungo, cooldown alto, uso strategico |
| Sincopato | dash attivato su levare/off-beat, input anticipato |

---

## 9. Archetipi cimatici

Gli archetipi sono **stati fisici della piastra**, non semplici generi musicali. Una traccia custom può cambiare archetipo nel tempo o mescolare più stati.

### 9.1 Sintetico / Matriziale

**Musiche tipiche:** EDM, techno, chiptune, cyberpunk.  
**Identità:** rigidità, griglia, precisione, quantizzazione.

Meccaniche:

- muri ortogonali;
- proiettili rettilinei;
- pattern simmetrici;
- dash a celle;
- telegraph molto netto;
- alto valore per riflessi e timing.

### 9.2 Organico / Concentrico

**Musiche tipiche:** classica, barocca, orchestrale, acustica.  
**Identità:** curve, onde, flusso, dinamica.

Meccaniche:

- linee concentriche/ellittiche;
- crescendi che comprimono arena;
- particelle fluide;
- Vettore su curve di Bézier;
- graze continuo lungo linee curve;
- gioco di posizionamento e lettura spaziale.

### 9.3 Fratturato / Caotico

**Musiche tipiche:** rock, metal, punk, noise, industrial.  
**Identità:** saturazione, rottura, frattura, schegge.

Meccaniche:

- muri frastagliati;
- strutture distruttibili;
- shrapnel;
- jitter controllato;
- dash d’impatto;
- rischio alto e improvvisazione.

### 9.4 Etereo / Sospeso

**Musiche tipiche:** ambient, drone, lo-fi, cinematic soft.  
**Identità:** lentezza, nebbia, macroforme, ipnosi.

Meccaniche:

- blocchi grandi e lenti;
- nebbia di Dissonanza;
- dash lungo ma raro;
- pianificazione;
- danno nel tempo invece di impatti rapidi;
- flow meditativo ma pericoloso.

### 9.5 Sincopato / Spostato

**Musiche tipiche:** jazz, progressive, IDM, math rock.  
**Identità:** tempi dispari, poliritmia, off-beat, rotazioni.

Meccaniche:

- assi che ruotano;
- pattern su ritmi sovrapposti;
- Vettore in levare;
- dash ritardato/anticipato;
- spostamenti prospettici;
- sfida cerebrale.

---

## 10. Progressione temporale: saturazione della piastra

### 10.1 Saturazione

Più una traccia è lunga, più la piastra accumula energia. La difficoltà cresce senza dover aumentare soltanto il numero di proiettili.

Fasi possibili:

| Fase | Tempo indicativo | Effetto |
|---|---:|---|
| Cristallina | 0–60s | linee sottili, pattern leggibili |
| Eccitazione | 60–180s | muri più spessi, più particelle, più pressione |
| Turbolenza | 180s+ | sfaldamento, polvere letale, corridoi stretti |

### 10.2 Effetti

- aumento densità particellare;
- incremento spessore nodale;
- proiettili più frequenti;
- telegraph più breve ma sempre presente;
- Dissonanza più difficile da ridurre;
- archetipi più estremi.

---

## 11. Visual design

### 11.1 Principio

Il gioco deve essere minimale nelle forme primarie ma ricco in colore, movimento e metamorfosi. La musica deve letteralmente prendere forma e colore.

### 11.2 HUD diegetico

Evitare UI invasiva.

- Dissonanza = degrado visivo/audio del Seme e della piastra;
- tempo traccia = anello/oscilloscopio perimetrale;
- beat = respiro luminoso dell’arena;
- risonanza caricata = intensità del Seme;
- cooldown dash = forma/alone del Seme;
- Drop Shock pronto = pattern di risonanza interno al Seme.

### 11.3 Palette per archetipi

| Archetipo | Palette indicativa |
|---|---|
| Sintetico | nero, cyan elettrico, magenta, giallo acido |
| Organico | antracite, oro caldo, verde smeraldo, bianco perla |
| Fratturato | ferro bruciato, rosso lava, arancione, bianco fosforico |
| Etereo | blu notte, indaco, viola spettrale, verde bioluminescente |
| Sincopato | ottanio, corallo, lime, arancione bruciato |

### 11.4 Requisiti di leggibilità

- Le minacce devono avere silhouette nette.
- Ogni attacco importante richiede telegraph.
- I colori estetici non devono nascondere proiettili.
- La Dissonanza non deve rendere il gioco illeggibile prima del game over.
- Accessibilità: prevedere profili colore alternativi.

---

## 12. Sistema particellare e fisica

### 12.1 Decisione principale

CYMATICA ha bisogno di un **sistema particellare custom**, ma non ha bisogno di un motore fisico esterno nella Milestone 1.

Distinzione obbligatoria:

| Sistema | Scopo | Collisione gameplay | Determinismo richiesto |
|---|---|---:|---:|
| Bullet system | proiettili e minacce | sì | alto |
| Particle system | sabbia, scie, polvere, glow, glitch | no o limitata | medio |
| Physics engine | rigid body, urti realistici, vincoli | eventuale | variabile |

La separazione è critica:

```text
Bullet = entità di gameplay.
Particle = effetto visivo.
Physics = simulazione fisica opzionale, non core.
```

### 12.2 Particle system custom

Modulo proposto:

```text
engine/
├── particles/
│   ├── particle.h
│   ├── particle_system.h/.cpp
│   ├── particle_emitters.h/.cpp
│   ├── particle_presets.h
│   └── particle_budget.h
├── gameplay/
│   ├── bullet.h
│   ├── bullet_pool.h
│   ├── bullet_patterns.h
│   └── collision.h
└── graphics/
    ├── particle_renderer.h/.cpp
    └── shaders/
        ├── particles.vs
        └── particles.fs
```

Struttura base:

```cpp
struct Particle {
    Vector2 position;
    Vector2 velocity;
    float lifetime;
    float age;
    float size;
    float rotation;
    float angularVelocity;
    Color color;
    ParticleKind kind;
};
```

Regola di aggiornamento:

```cpp
particle.position += particle.velocity * dt;
particle.velocity += cymaticFieldForce * dt;
particle.age += dt;
particle.size *= decay;
```

Dove `cymaticFieldForce` è guidata da audio e piastra:

```text
fieldForce =
    chladni_gradient(position, audio_frame)
  + pulse_impulse
  + dissonance_noise
  + archetype_wind
```

### 12.3 Budget e performance

Milestone 1:

- CPU particle system;
- object pooling;
- niente allocazioni per frame;
- target iniziale 2.000–5.000 particelle;
- limite dinamico scalabile;
- nessun compute shader obbligatorio;
- nessun motore fisico esterno.

Milestone successive:

- renderer dedicato;
- texture atlas o mesh batching;
- preset per archetipi;
- particelle attratte/respinte da nodi e antinodi;
- eventuale GPU particles solo se necessario;
- fallback CPU obbligatorio per Android.

### 12.4 Comportamento per archetipo

| Archetipo | Particelle |
|---|---|
| Sintetico | quadrate, rettilinee, scie digitali, dissolvenza a step |
| Organico | sabbia fluida, moto orbitale, scie morbide |
| Fratturato | schegge, scintille, frammenti veloci, jitter |
| Etereo | nebbia, particelle lente, drift inerziale |
| Sincopato | impulsi irregolari, rotazioni improvvise, split cromatici |

### 12.5 Motori fisici esterni

Non integrare Box2D o simili nella Milestone 1.

Potrebbero essere valutati solo più avanti per:

- helper collisione;
- prototipi specifici;
- modalità speciali con oggetti fisici;
- tooling/debug.

Per il core loop, collisioni e pattern devono restare deterministici, semplici e controllabili.

---

## 13. Stack tecnologico

### 13.1 Scelta raccomandata per prototipo

**C++20 + CMake + raylib + miniaudio**

Motivazione:

- controllo diretto su loop audio e rendering;
- poche dipendenze;
- codice testuale, adatto ad agenti;
- build deterministica;
- buona portabilità;
- prototipo veloce;
- basso overhead;
- possibilità di dist separate per gioco e tool.

### 13.2 Motivi per non partire da Flutter

Flutter può essere utile per tool/editor o app companion, ma non è la scelta più sicura per il core game prototipo perché:

- audio a bassa latenza richiederebbe comunque codice nativo;
- rendering bullet hell massivo richiederebbe CustomPainter/shader/FFI;
- il rischio è costruire due stack invece di uno;
- per un agente, FFI + Dart + C++ aumenta i punti di rottura.

Flutter può restare candidato per un futuro editor/launcher, ma non per il primo core gameplay.

### 13.3 Motivi per non partire da Unity/Unreal/FMOD/Wwise

Unity, FMOD, Wwise e Unreal/MetaSounds sono potenti, ma nel contesto di sviluppo agentico hanno più superfici opache:

- asset database;
- editor visuali;
- file progetto complessi;
- workflow non interamente testuale;
- più difficile generare e verificare modifiche da terminale.

La scelta nativa non esclude di ispirarsi a MetaSounds/FMOD/Wwise. Il progetto può replicare parte del loro modello tramite codice C++ e node graph/audio routing in miniaudio o moduli interni.

### 13.4 Motivi per considerare Godot in futuro

Godot resta una valida seconda scelta se si desidera un engine 2D completo e open-source. È più testuale di Unity/Unreal, ma introduce comunque scene, import asset e GDExtension.

Per ora non è la strada primaria.

---

## 14. Ambiente di sviluppo e dipendenze

### 14.1 Principio

Poiché il progetto non usa un package manager applicativo come npm, tutte le dipendenze devono essere censite, classificate e rese riproducibili.

Ogni dipendenza deve avere:

- nome;
- ruolo;
- obbligatoria/opzionale;
- fase di utilizzo;
- modalità di acquisizione;
- versione/pin;
- licenza;
- piattaforme target;
- note per build agentica.

Ogni nuova dipendenza richiede aggiornamento di:

```text
docs/dependencies.md
docs/build.md
CMakeLists.txt o cmake/*.cmake
```

### 14.2 Requisiti minimi Windows

| Requisito | Obbligatorio | Note |
|---|---:|---|
| Windows 10/11 x64 | sì | target primario sviluppo |
| Git | sì | clone repo e submodule eventuali |
| Visual Studio 2022/18 Insiders con MSVC C++ | sì | compilatore `cl.exe` |
| CMake | sì | anche se non globale nel PATH, va documentato path effettivo |
| Ninja o MSBuild | consigliato | scegliere uno come default |
| Python 3 | opzionale in M0, utile dopo | script tooling/test, non core runtime |
| PowerShell | sì | comandi Windows e script utilità |
| Antigravity 2.0 | consigliato | IDE agentico di riferimento |

### 14.3 Dipendenze runtime/prototipo

| Dipendenza | Uso | Fase | Acquisizione consigliata | Note |
|---|---|---|---|---|
| raylib | finestra, input, rendering base, shader | M0+ | CMake `FetchContent` con versione/tag fissato | non vendorizzare subito, ma cache CMake ammessa |
| miniaudio | audio low-level, callback, mixing/DSP | M0+ | vendorizzare `miniaudio.h` in `thirdparty/miniaudio/` | single-header, aggiornamento manuale controllato |
| GLSL shader | Chladni, particelle, visual | M0+ | asset sorgente nel repo | nessuna dipendenza esterna |
| doctest/Catch2 o test harness custom | test unitari | M0+ | FetchContent o vendored single-header | scegliere una sola soluzione |

### 14.4 Dipendenze tool/offline future

| Dipendenza | Uso | Fase | Acquisizione consigliata | Note |
|---|---|---|---|---|
| FFmpeg | conversione, normalizzazione, decoding tool | M7+ | eseguibile esterno configurabile o libreria solo se necessario | evitare linkage complesso all’inizio |
| ONNX Runtime | inferenza stem separation | M8+ | pacchetto separato, CPU-only iniziale | non richiesto per game runtime |
| Modelli source separation | stem extraction | M8+ | download manuale/cache tool | non committare modelli pesanti nel repo |
| Essentia/librosa-equivalent | MIR avanzata | M7/M8+ | valutazione separata | evitare dipendenza pesante prima del formato stabile |
| Strudel | research/pattern sketching | research | fuori runtime, eventualmente in `research/` | license review obbligatoria prima di integrazione |

### 14.5 Dipendenze Android future

| Dipendenza | Uso | Fase | Note |
|---|---|---|---|
| Android Studio / SDK | build e deploy | M9 | installazione sistema |
| Android NDK | C++ cross compile | M9 | necessario per CMake Android |
| Gradle | packaging Android | M9 | generato/gestito da toolchain Android |
| AAudio/Oboe o backend miniaudio Android | low-latency audio | M9 | test su device reale obbligatorio |
| ONNX Runtime Android/NNAPI | inferenza mobile opzionale | M8/M9+ | non prioritaria |

### 14.6 Policy di acquisizione dipendenze

Classificare ogni libreria in una di queste categorie:

1. **System dependency**  
   Installata nel sistema operativo o nella toolchain. Esempi: MSVC, CMake, Git, Android SDK.

2. **CMake-fetched dependency**  
   Scaricata automaticamente da CMake tramite `FetchContent` o script equivalente. Esempio: raylib.

3. **Vendored dependency**  
   Copiata nel repo sotto `thirdparty/`. Esempio: `miniaudio.h`.

4. **External binary dependency**  
   Eseguibile esterno referenziato da path/config. Esempio: FFmpeg nella prima versione del tool.

5. **Model/data dependency**  
   File pesante non committato nel repo. Esempio: modelli ONNX per source separation.

6. **Research-only dependency**  
   Usata per esperimenti, non inclusa nei build distribuibili. Esempio: Strudel.

### 14.7 Regole per Antigravity

Antigravity può scaricare dipendenze se l’ambiente lo consente, ma il progetto non deve dipendere implicitamente da questa capacità.

Direttive:

- se una dipendenza viene scaricata automaticamente, documentare URL, versione/tag e cache path;
- se una dipendenza va scaricata manualmente, indicare istruzioni precise in `docs/dependencies.md`;
- non introdurre dipendenze nuove dentro il codice senza aggiornare CMake e documentazione;
- non committare binari grandi o modelli ML nel repo principale;
- tenere il runtime del gioco privo di dipendenze tool-only;
- ONNX/FFmpeg/Strudel non devono essere richiesti per compilare `cymatica_game`.

### 14.8 File consigliato `docs/dependencies.md`

Template:

```markdown
# CYMATICA Dependencies

| Name | Version/Tag | Required | Used by | Acquisition | License | Notes |
|---|---|---:|---|---|---|---|
| raylib | TBD pinned tag | yes | game, renderer | CMake FetchContent | zlib/libpng | pin before M0 closes |
| miniaudio | vendored snapshot | yes | audio | thirdparty/miniaudio | license review | store source + version note |
| FFmpeg | TBD | no | tool CLI | external binary | license review | M7+ only |
| ONNX Runtime | TBD | no | ingestion | external package | MIT | M8+ CPU-only first |
| Strudel | none | no | research | external/web | AGPL-3.0 | not runtime |
```

---

## 15. Repository, target e distribuzioni

### 15.1 Decisione

Il tool deve stare **nello stesso repo** del gioco, ma come **target separato** e con **distribuzione separata**.

Motivo:

- gioco e tool condividono formato dati, archetipi, matematica e validatori;
- il gioco deve restare snello;
- il tool può avere dipendenze pesanti;
- il formato `.cymlevel` va evoluto insieme al runtime;
- un monorepo è più adatto allo sviluppo agentico nelle prime fasi.

### 15.2 Struttura consigliata

```text
cymatica/
├── CMakeLists.txt
├── cmake/
│   ├── dependencies.cmake
│   ├── options.cmake
│   └── platform.cmake
├── apps/
│   ├── cymatica_game/
│   │   ├── main.cpp
│   │   └── CMakeLists.txt
│   ├── cymatica_tool_cli/
│   │   ├── main.cpp
│   │   └── CMakeLists.txt
│   └── cymatica_studio/
│       ├── main.cpp
│       └── CMakeLists.txt
├── engine/
│   ├── core/
│   ├── audio/
│   ├── graphics/
│   ├── gameplay/
│   ├── particles/
│   └── level_runtime/
├── toolchain/
│   ├── ingestion/
│   ├── analysis/
│   ├── packaging/
│   └── preview/
├── shared/
│   ├── cymatica_format/
│   ├── archetypes/
│   ├── math/
│   └── utils/
├── thirdparty/
│   └── miniaudio/
├── assets/
│   ├── shaders/
│   ├── presets/
│   └── test_audio/
├── research/
│   └── strudel-patterns/
├── tests/
│   ├── unit/
│   ├── integration/
│   └── golden/
└── docs/
    ├── architecture.md
    ├── build.md
    ├── dependencies.md
    ├── level-format.md
    ├── agent-workflow.md
    └── roadmap.md
```

### 15.3 Dipendenze tra target

Regola architetturale:

```text
cymatica_game dipende da shared + engine.
cymatica_tool_cli dipende da shared + toolchain.
cymatica_studio dipende da shared + toolchain + eventuale preview engine.
Il gioco non deve dipendere da ONNX, FFmpeg, Strudel o modelli ML.
```

Target CMake consigliati:

```text
cymatica_shared
cymatica_engine
cymatica_game
cymatica_toolchain
cymatica_tool_cli
cymatica_studio        # futuro/opzionale
```

Opzioni CMake:

```cmake
option(CYMATICA_BUILD_GAME "Build CYMATICA game runtime" ON)
option(CYMATICA_BUILD_TOOLS "Build CYMATICA offline tools" OFF)
option(CYMATICA_BUILD_STUDIO "Build CYMATICA graphical studio" OFF)
option(CYMATICA_ENABLE_ONNX "Enable ONNX Runtime integration" OFF)
option(CYMATICA_ENABLE_FFMPEG "Enable FFmpeg ingestion" OFF)
option(CYMATICA_ENABLE_ANDROID "Enable Android build settings" OFF)
```

### 15.4 Distribuzione

Dist separate:

```text
dist/
├── game/
│   ├── CYMATICA.exe
│   └── assets/
├── tools/
│   ├── cymatica-tool-cli.exe
│   └── README-tool.md
├── studio/
│   ├── cymatica-studio.exe
│   └── assets/
└── examples/
    └── custom_levels/
```

Regola:

> Stesso repo sì. Stessa dist no.

### 15.5 Quando separare in due repo

Separare il tool in un repo autonomo solo se diventa prodotto indipendente, con:

- ciclo release separato;
- plugin marketplace;
- modelli scaricabili;
- editor community avanzato;
- dipendenze molto diverse;
- team/manutenzione separata.

Prima di quel punto, due repo aumentano solo attrito.

---

## 16. Architettura software runtime

### 16.1 Moduli principali runtime

```text
engine/
├── audio/
│   ├── audio_engine.h/.cpp
│   ├── synthesizer.h/.cpp
│   ├── euclidean_sequencer.h/.cpp
│   ├── modal_quantizer.h/.cpp
│   ├── audio_frame.h
│   └── audio_buffer_exchange.h/.cpp
├── graphics/
│   ├── cymatic_renderer.h/.cpp
│   ├── particle_renderer.h/.cpp
│   ├── shader_uniforms.h
│   └── palette.h/.cpp
├── gameplay/
│   ├── game_state.h/.cpp
│   ├── player.h/.cpp
│   ├── quantum_dash.h/.cpp
│   ├── dissonance.h/.cpp
│   ├── bullet.h/.cpp
│   ├── bullet_pool.h/.cpp
│   ├── bullet_patterns.h/.cpp
│   ├── resonance_node.h/.cpp
│   └── archetype_runtime.h/.cpp
├── particles/
│   ├── particle_system.h/.cpp
│   ├── particle_emitters.h/.cpp
│   └── particle_presets.h/.cpp
└── level_runtime/
    ├── level_loader.h/.cpp
    ├── timeline_player.h/.cpp
    └── custom_level_index.h/.cpp
```

### 16.2 Thread model

#### Audio thread

Gestito da miniaudio.

Responsabilità:

- generare audio;
- calcolare parametri normalizzati;
- scrivere `AudioFrame`;
- non allocare memoria nella callback;
- non usare lock bloccanti nella callback;
- non chiamare codice tool/offline.

#### Game/render thread

Gestito dal loop raylib.

Responsabilità:

- leggere ultimo `AudioFrame`;
- aggiornare stato gameplay;
- aggiornare player, proiettili, particelle, collisioni;
- passare uniform allo shader;
- renderizzare.

### 16.3 AudioFrame

```cpp
struct ChannelFrame {
    float energy;      // 0..1
    float onset;       // 0..1
    float pitch;       // normalized or Hz
    float density;     // 0..1
    float confidence;  // 0..1
};

struct AudioFrame {
    double audioTimeSeconds;
    float bpm;
    float beatPhase;       // 0..1
    float beatConfidence;  // 0..1

    ChannelFrame pulse;
    ChannelFrame body;
    ChannelFrame texture;
    ChannelFrame vector;

    float globalEnergy;
    float spectralFlux;
    float dissonanceInfluence;
    int archetypeId;
};
```

### 16.4 Double buffering

Il passaggio audio → render deve usare double buffer o atomic index.

Requisiti:

- audio thread scrive su buffer inattivo;
- quando il frame è completo, aggiorna indice atomico;
- render thread legge snapshot coerente;
- nessun mutex pesante nella callback audio.

---

## 17. Pipeline grafica

### 17.1 Shader Chladni

Lo shader riceve uniform:

```text
u_time
u_pulse
u_body_energy
u_texture_density
u_vector_energy
u_mode_m
u_mode_n
u_dissonance
u_archetype
u_saturation
```

Calcola una funzione Chladni approssimata e produce linee nodali, distorsioni e colori.

### 17.2 Output shader

Output minimi:

- linee nodali;
- antinodi o regioni di energia;
- glitch Dissonanza;
- palette per archetipo;
- telegraph layer;
- debug mode opzionale.

### 17.3 Collisioni con geometria cimatica

Non fare collisione pixel-perfect sullo shader nella Milestone 1.

Approccio consigliato:

- gameplay usa rappresentazione semplificata CPU;
- shader è visualizzazione coerente ma non autorità fisica;
- bullet/muri hanno primitive collisione leggibili;
- eventuale sampling della funzione Chladni solo per zone nodali principali.

---

## 18. Audio procedurale

### 18.1 Obiettivo prototipo

La musica procedurale deve essere sufficientemente credibile, non necessariamente finale.

Obiettivo minimo:

- beat euclideo;
- bass/body layer;
- texture/arpeggio;
- vector/lead semplice;
- mix con sidechain base;
- Dissonanza audio;
- variazione progressiva.

### 18.2 Sequencer euclideo

Implementare funzione `E(k, n)`.

Esempio:

```text
E(5, 8) = X . X X . X X .
```

Usi:

- Pulso;
- hi-hat/texture;
- poliritmia Sincopato;
- aumento complessità con Graze/performance.

### 18.3 Quantizzatore modale

Il generatore melodico deve quantizzare su scale definite.

Esempi:

- minore naturale;
- frigio dominante;
- pentatonica minore;
- dorico;
- scala esatonale per Sincopato/Etereo.

Ogni nota casuale viene forzata al grado più vicino.

### 18.4 Dissonanza audio

La Dissonanza modula:

- bitcrushing leggero;
- saturation;
- band-pass/low-pass;
- stereo wobble;
- rumore controllato;
- pitch instability.

Regola:

> La Dissonanza deve comunicare pericolo senza rendere sgradevole il gioco troppo presto.

---

## 19. Formato pacchetto livello

### 19.1 Cartella livello

Formato cartella:

```text
MySong.cymlevel/
├── level_info.json
├── cymatic_timeline.json
├── audio/
│   ├── mix.ogg
│   ├── pulse.ogg
│   ├── body.ogg
│   ├── texture.ogg
│   └── vector.ogg
└── preview/
    └── cover.png
```

Per custom iniziale, gli stem possono essere assenti. Il runtime deve accettare:

- solo timeline;
- timeline + mix;
- timeline + stem funzionali.

### 19.2 `level_info.json`

```json
{
  "format_version": 1,
  "title": "Example Level",
  "artist": "Unknown",
  "source": "procedural|custom|generated",
  "duration_seconds": 120.0,
  "bpm": 128.0,
  "default_archetype": "synthetic",
  "difficulty_estimate": 0.45,
  "seed_policy": "fixed|randomized|daily",
  "audio": {
    "mix": "audio/mix.ogg",
    "pulse": null,
    "body": null,
    "texture": null,
    "vector": null
  }
}
```

### 19.3 `cymatic_timeline.json`

```json
{
  "format_version": 1,
  "frame_rate": 50,
  "channels": ["pulse", "body", "texture", "vector"],
  "frames": [
    {
      "time": 0.00,
      "pulse": { "energy": 0.0, "onset": 0.0, "pitch": 0.0, "density": 0.0, "confidence": 1.0 },
      "body": { "energy": 0.2, "onset": 0.0, "pitch": 55.0, "density": 0.1, "confidence": 0.8 },
      "texture": { "energy": 0.1, "onset": 0.0, "pitch": 440.0, "density": 0.2, "confidence": 0.7 },
      "vector": { "energy": 0.0, "onset": 0.0, "pitch": 0.0, "density": 0.0, "confidence": 0.2 },
      "archetype": "synthetic"
    }
  ]
}
```

---

## 20. Tool offline

### 20.1 Ruolo

Il tool offline non è necessario per il primo prototipo. Diventerà necessario per musica custom e generated tracks.

La prima versione deve essere **CLI**, non GUI.

### 20.2 Collocazione nel repo

Target:

```text
apps/cymatica_tool_cli
```

Dipendenze:

```text
shared/cymatica_format
toolchain/packaging
toolchain/analysis   # solo quando necessario
```

Il tool non deve essere richiesto per compilare il gioco.

### 20.3 CLI iniziale

Prima dei file audio reali:

```bash
cymatica-tool new-level --name test_procedural
cymatica-tool validate path/to/level.cymlevel
cymatica-tool inspect path/to/level.cymlevel
```

Quando il formato è stabile:

```bash
cymatica-tool ingest song.mp3 --out ./CustomLevels/SongName
cymatica-tool analyze ./CustomLevels/SongName
cymatica-tool package ./CustomLevels/SongName --format cymlevel
```

### 20.4 Pipeline custom futura

1. Import file audio.
2. Convert/normalize con FFmpeg.
3. Analisi globale: durata, loudness, BPM, beat grid.
4. Analisi locale: onset, RMS, spectral flux, pitch salience.
5. Separazione stem opzionale.
6. Mappatura a Pulso/Corpo/Trama/Vettore.
7. Segmentazione archetipi.
8. Stima difficoltà.
9. Esportazione pacchetto.
10. Validazione pacchetto.

### 20.5 Regola importante

La separazione stem è un acceleratore qualitativo, non una dipendenza critica. Il sistema deve funzionare anche con sola analisi del mix.

---

## 21. Roadmap

### Milestone 0 — Repository, build e dependency inventory

**Obiettivo:** progetto compilabile su Windows con dipendenze censite.

Deliverable:

- monorepo iniziale;
- `docs/build.md`;
- `docs/dependencies.md`;
- CMake options;
- raylib via FetchContent con tag fissato;
- miniaudio vendored;
- `cymatica_game` compilabile;
- finestra raylib;
- audio callback che emette tono/test beat;
- shader caricato da file;
- nessun tool obbligatorio.

Criteri accettazione:

- build pulita da terminale MSVC;
- eseguibile avviabile;
- nessun crash in chiusura;
- audio udibile;
- shader visibile;
- dipendenze documentate.

### Milestone 1 — Piastra cimatica e audio procedurale minimo

**Obiettivo:** prima sinestesia audio-visuale.

Deliverable:

- sequencer euclideo;
- modal quantizer;
- `AudioFrame`;
- double buffer;
- shader Chladni pilotato da audio;
- parametri `m/n` dinamici;
- debug overlay opzionale.

Criteri accettazione:

- linee cambiano con il beat;
- nessun crackling audio evidente;
- nessuna race condition nota;
- test euclideo e quantizer passano.

### Milestone 2 — Player, Dissonanza e particelle base

**Obiettivo:** primo loop giocabile e feedback visivo.

Deliverable:

- movimento player;
- collisione base con proiettili;
- Dissonanza;
- feedback audio/visuale;
- particle system CPU object-pooled;
- trail del Seme;
- game over;
- reset.

Criteri accettazione:

- il giocatore può perdere;
- la Dissonanza è percepibile;
- lo stato è leggibile;
- nessuna allocazione per frame nel sistema particellare core;
- gameplay minimo di 60 secondi.

### Milestone 3 — Quantum Dash e Graze

**Obiettivo:** game feel centrale.

Deliverable:

- dash standard;
- destinazione visualizzata;
- safe landing assist;
- cooldown;
- graze;
- carica Risonanza.

Criteri accettazione:

- dash affidabile con gamepad e mouse/tastiera;
- nessuna destinazione ingiusta evidente;
- graze non farmabile in modo banale.

### Milestone 4 — Drop Shock e archetipi

**Obiettivo:** identità meccanica.

Deliverable:

- Drop Shock;
- archetipo Sintetico;
- archetipo Organico;
- palette e shader differenziati;
- pattern proiettili differenziati;
- preset particellari per archetipo.

Criteri accettazione:

- i due archetipi si giocano diversamente;
- Drop Shock è utile ma non dominante;
- la difficoltà scala con saturazione.

### Milestone 5 — Fratturato, Etereo, Sincopato

**Obiettivo:** espansione varietà.

Deliverable:

- tre archetipi rimanenti;
- regole dash specifiche opzionali;
- pattern e palette;
- particelle differenziate;
- bilanciamento base.

Criteri accettazione:

- ogni archetipo è riconoscibile;
- nessun archetipo rompe leggibilità o performance.

### Milestone 6 — Pacchetti livello e timeline

**Obiettivo:** preparare custom/generated.

Deliverable:

- `cymatica_format` shared;
- loader `level_info.json`;
- loader `cymatic_timeline.json`;
- playback dati timeline;
- cartella custom levels;
- menu resonance node per selezione;
- `cymatica_tool_cli validate`.

Criteri accettazione:

- un pacchetto fittizio viene caricato;
- il livello usa la timeline invece dell’audio procedurale live;
- errori JSON gestiti chiaramente.

### Milestone 7 — Tool offline minimo

**Obiettivo:** generare pacchetti da audio reale senza stem separation.

Deliverable:

- CLI tool;
- FFmpeg integration documentata;
- analisi RMS/onset/beat semplificata;
- export `.cymlevel`.

Criteri accettazione:

- un file WAV/MP3 viene convertito in pacchetto;
- il gioco carica il pacchetto;
- gameplay sincronizzato accettabile;
- se FFmpeg manca, il tool spiega chiaramente come configurarlo.

### Milestone 8 — Stem separation / ONNX

**Obiettivo:** migliorare qualità custom.

Deliverable:

- modulo sperimentale ONNX;
- CPU-only prima;
- eventuale NNAPI/DirectML dopo;
- fallback senza stem;
- cache locale;
- modelli esterni non committati.

Criteri accettazione:

- se ONNX fallisce, il tool produce comunque un livello;
- i tempi di elaborazione sono mostrati;
- nessuna dipendenza GPU obbligatoria.

### Milestone 9 — Android technical preview

**Obiettivo:** validare portabilità.

Deliverable:

- toolchain Android NDK;
- build Android;
- input touch/controller;
- test audio latency;
- profilo performance;
- fallback grafico/particellare.

Criteri accettazione:

- app avviabile su dispositivo reale;
- audio stabile;
- frame pacing misurato;
- backlog problemi Android documentato.

### Milestone 10 — Studio/editor GUI

**Obiettivo:** creare editor visuale solo dopo stabilità formato/tool CLI.

Opzioni:

- GUI nativa C++/raylib se minimale;
- Flutter/desktop se serve UI complessa;
- web-based tool se più semplice per preview e distribuzione;
- nessuna decisione prima di M6/M7.

---

## 22. Backlog prioritizzato

### P0 — Necessario per vertical slice

- build Windows;
- dependency inventory;
- audio callback;
- shader Chladni;
- player movement;
- Dissonanza;
- Quantum Dash;
- proiettili base;
- particle system CPU base;
- due archetipi;
- test core.

### P1 — Necessario per prototipo convincente

- Drop Shock;
- Graze;
- resonance nodes menu;
- saturazione piastra;
- palette archetipi;
- particle presets;
- debug overlay;
- input gamepad completo;
- calibrazione audio/video di base.

### P2 — Custom/generated content

- formato `.cymlevel`;
- timeline loader;
- tool CLI validate/inspect;
- generated finite tracks;
- custom track senza stem.

### P3 — Advanced audio/AI

- stem separation;
- ONNX Runtime;
- modelli locali;
- playtesting automatico;
- analisi MIR avanzata;
- editor visuale.

### P4 — Platform expansion

- Android technical preview;
- input touch;
- performance mobile;
- packaging;
- eventuale cloud/community sharing.

---

## 23. Testing strategy

### 23.1 Unit test

Test obbligatori:

- Euclidean sequencer;
- modal quantizer;
- Chladni evaluation;
- double buffer;
- dash destination;
- dissonance update;
- object pool particelle;
- particle budget;
- level manifest parser;
- dependency config parser se presente.

### 23.2 Integration test

- audio thread produce frame;
- render thread consuma frame;
- shader riceve uniform;
- player collide con bullet;
- particle system non alloca oltre init;
- game state resetta correttamente;
- tool CLI valida pacchetto fittizio.

### 23.3 Manual test checklist

- audio crackle;
- input lag;
- leggibilità minacce;
- dash fairness;
- bilanciamento Dissonanza;
- performance con 1k/5k/10k proiettili;
- performance con 1k/5k particelle;
- gamepad e mouse/tastiera;
- shader leggibile con Dissonanza alta.

### 23.4 Performance budget iniziale

Target Windows:

- 60 FPS minimo;
- 120 FPS desiderabile;
- audio callback senza allocazioni;
- frame time stabile sotto 16.6 ms;
- nessun hitch percepibile su shader reload disattivato in release.

Target Android futuro:

- 60 FPS;
- qualità shader scalabile;
- numero proiettili adattivo;
- budget particellare adattivo;
- audio low-latency profile;
- fallback grafico.

---

## 24. Sviluppo agentico e AGENTS.md

Le istruzioni operative per Google Antigravity 2.0 o IDE agentici equivalenti non sono più mantenute come prompt interni a questa specifica.

La regola adottata è:

- questo documento descrive **vision, requisiti, architettura, milestone, vincoli tecnici e decisioni progettuali**;
- `AGENTS.md` descrive **come gli agenti devono lavorare sul repository**: workflow, limiti, build/test, policy dipendenze, regole C++, audio callback, rendering, particelle, tool CLI, documentazione e review checklist.

Quando il progetto viene aperto in Antigravity, l’agente deve ricevere il repository con entrambi i file:

```text
CYMATICA_Specifica_Agentica_Sviluppo.md
AGENTS.md
```

`AGENTS.md` deve essere trattato come contratto operativo del repository. Se una regola operativa cambia, aggiornare `AGENTS.md`. Se cambia una decisione di prodotto, architettura o milestone, aggiornare questa specifica.

---

## 25. Decision log

| Decisione | Stato | Motivazione |
|---|---:|---|
| Cimatica come base scientifica | Accettata | Differenzia gameplay e visual |
| Dissonanza al posto della vita | Accettata | Integra audio, rischio e feedback |
| Canali Pulso/Corpo/Trama/Vettore | Accettata | Funziona con voce, strumentale e procedurale |
| 5 archetipi cimatici | Accettata | Varietà senza moltiplicare controlli |
| Procedurale prima del custom | Accettata | Riduce rischio e valida game feel |
| C++20/CMake/raylib/miniaudio per core | Accettata per prototipo | Adatto ad agenti e performance |
| Tool nello stesso repo | Accettata | Condivide formato e codice shared |
| Tool nella stessa distribuzione del gioco | Rifiutata | Dist separate per runtime snello |
| Tool GUI subito | Rifiutata | Prima CLI e formato stabile |
| ONNX in Milestone 1 | Rifiutata | Troppo rischio/scope |
| FFmpeg in Milestone 1 | Rifiutata | Non necessario per procedurale |
| Strudel come runtime dependency | Rifiutata per ora | Utile per ricerca, licenza da verificare |
| Strudel come research track | Accettata | Ottimo per pattern/prototipi musicali |
| Motore fisico esterno in M1 | Rifiutato | Custom particle/bullet system sufficiente |
| Particle system custom | Accettato | Necessario per identità visuale |
| Flutter per core gameplay | Non raccomandato | Troppo FFI e latenza audio |
| Android in prima milestone | Rimandato | Prima validare core Windows |
| UI tradizionale complessa | Evitata | Menu come resonance nodes |
| Prompt operativi dentro specifica | Rifiutata | La specifica deve restare documento tecnico, non manuale operativo dell’agente |
| AGENTS.md dedicato | Accettata | Contratto operativo separato per Antigravity e agenti equivalenti |
| Dependency inventory | Obbligatorio | Evita ambiguità in C++/CMake |

---

## 26. Open questions residue

1. Nome definitivo: CYMATICA è nome progetto o titolo finale?
2. Il gioco sarà single-player puro o si prevede co-op locale in futuro?
3. Il Seme può “attaccare” direttamente o interagisce solo tramite Drop Shock?
4. La Dissonanza deve avere soglie discrete o continua?
5. I pacchetti custom potranno essere condivisi? Se sì, servono regole su copyright e metadati.
6. Quanto deve essere punitivo il fallimento ritmico del dash?
7. Il gioco deve includere calibrazione latenza audio/video già nella vertical slice?
8. Le modalità generated finite devono usare solo sintesi interna o anche modelli generativi esterni?
9. Gli archetipi possono fondersi simultaneamente o solo cambiare per segmenti?
10. Lo studio/editor futuro sarà C++/raylib, Flutter, web o altro?
11. Quale test harness C++ adottare: doctest, Catch2 o soluzione custom minima?
12. Raylib va sempre FetchContent o si vuole supportare anche installazione di sistema?
13. FFmpeg sarà invocato come binario esterno o linkato come libreria?
14. La community potrà distribuire pacchetti `.cymlevel` contenenti audio protetto da copyright?
15. Serve una mini-DSL musicale nativa ispirata a Strudel/Tidal?

---

## 27. Raccomandazione finale

La direzione è valida e merita un prototipo. La cosa più importante è non partire dal tool custom, dagli stem o dall’AI: il valore del gioco dipende prima dal **game feel cimatica/audio/player**.

Il primo obiettivo deve essere una scena giocabile di 60–90 secondi in modalità procedurale infinita, con:

- piastra Chladni viva;
- beat procedurale;
- Seme controllabile;
- Quantum Dash;
- Dissonanza;
- proiettili;
- particelle VFX;
- Sintetico e Organico come archetipi dimostrativi.

Se quella scena è divertente, il resto del progetto ha basi forti. Se quella scena non funziona, stem separation, editor, Android e AI non la salveranno.

Strategia consigliata:

1. **Milestone 0:** repository, build, dependency inventory.
2. **Milestone 1–4:** runtime procedurale giocabile.
3. **Milestone 6–7:** formato pacchetto e tool CLI.
4. **Milestone 8+:** stem/ONNX/custom music.
5. **Milestone 9+:** Android.
6. **Milestone 10+:** editor GUI.

---

## 28. Riferimenti tecnici utili

- raylib — libreria C/C++ per videogiochi, rendering e input: https://www.raylib.com/
- raylib GitHub: https://github.com/raysan5/raylib
- raylib releases: https://github.com/raysan5/raylib/releases
- miniaudio — libreria audio single-file con API low-level e node graph: https://miniaud.io/
- miniaudio GitHub: https://github.com/mackron/miniaudio
- Android Oboe low latency audio: https://developer.android.com/games/sdk/oboe/low-latency-audio
- Android Oboe library: https://developer.android.com/games/sdk/oboe
- ONNX Runtime mobile: https://onnxruntime.ai/docs/tutorials/mobile/
- ONNX Runtime NNAPI Execution Provider: https://onnxruntime.ai/docs/execution-providers/NNAPI-ExecutionProvider.html
- Strudel: https://strudel.cc/
- Strudel project guide/license note: https://strudel.cc/technical-manual/project-start/
- Google Antigravity docs: https://antigravity.google/docs
