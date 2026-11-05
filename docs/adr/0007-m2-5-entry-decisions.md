# ADR-0007 — Ingresso in M2.5: sotto-fasi, checkpoint e calendario delle decisioni

- **Data:** 2026-11-03
- **Stato:** `accepted for implementation`: D-M2-17..20 approvate dal titolare il 2026-11-04 (fase M2.5.0). Le decisioni del §5 restano ai rispettivi checkpoint. M2.5.1 è autorizzata dal titolare; stato in [progress.md](../progress.md).
- **Milestone:** M2 — sottofase M2.5, «Integrazione e verifica finale».
- **Owner:** coordinatore dello sviluppo, su richiesta del titolare del 2026-11-03: dividere M2.5 in sotto-fasi con checkpoint, chiudere i punti aperti e rinviare le decisioni al momento opportuno anziché prenderle tutte all'ingresso.
- **Gerarchia:** Specifica > DESIGN > AGENTS > ADR > piano operativo > ricerca. Questo ADR **organizza** M2.5 e non modifica:
  - i criteri di M2 e di M2.5 della specifica §32;
  - la definizione normativa di AC-AO-02 ([agent_control_observability](../agent_control_observability.md));
  - gli ADR da 0003 a 0006, le ABI pubbliche e il budget di 8 voci.
- **Perimetro:** fissa solo ciò che serve ora (AGENTS §1.1): la struttura, il calendario, il metodo degli interventi e la regola di chiusura. Le scelte tecniche e musicali restano ai checkpoint.

## 1. Contesto

M2.4 è chiusa su `0dcc6da`. L'handoff è nel [dossier delle evidenze](../m2_procedural_evidence.md) §3–§4; la [proposta sulla qualità musicale](../research/M2_5_Entry_Music_Quality_Proposal.md) raccoglie P-M25-01..08 e le priorità riviste dopo l'[ascolto del 2026-11-03](../m2_4_human_listening_2026-11-03.md).

Un parere esterno consegnato dal titolare il 2026-11-03 dà «GO» e propone di congelare le decisioni all'ingresso con un ADR. Questo ADR ne conserva l'ordine generale (prima le decisioni, poi il codice) con due differenze, spiegate nel §7: le soglie AC-AO-02 si ratificano dopo il congelamento del contenuto, e una prima prova su device precede gli interventi musicali.

## 2. Decisioni approvate all'ingresso

### D-M2-17 — Sotto-fasi e checkpoint

- **Scelta:** M2.5 si esegue nelle sotto-fasi del §3, separate dai checkpoint del §4.
- **Motivazione:** gli interventi musicali cambiano segnale, stem e golden. Ogni decisione ha bisogno di evidenze che esistono solo dopo la fase precedente.
- **Alternative scartate:** fase unica con review finale (un difetto di ordine si scopre troppo tardi); tutte le decisioni all'ingresso (§7).
- **Contratti toccati:** nessuno. La roadmap §32 resta autoritativa; lo stato è in [progress.md](../progress.md).
- **Verifica:** ogni sotto-fase si chiude con la regola del §6.

### D-M2-18 — Calendario delle decisioni

- **Scelta:** ogni decisione aperta ha un checkpoint di riferimento e un momento limite (§5). Il rinvio è ammesso ed è registrato: una decisione non presa blocca solo il lavoro che ne dipende.
- **Regola:** ogni rinvio dichiara il motivo, la nuova scadenza e una milestone responsabile. Nessun punto resta senza destinazione (§8).
- **Alternativa scartata:** decidere per completezza, che AGENTS §1.1 esclude.

### D-M2-19 — Metodo degli interventi musicali (P-M25-05)

- **Scelta:** ogni intervento su F1–F4 e F6 è piccolo, misurabile e reversibile:
  - cambia una variabile alla volta, o un gruppo dichiarato;
  - ha un confronto A/B con il WAV di riferimento, a loudness allineata quando il livello non è l'oggetto della prova;
  - registra le misure prima e dopo;
  - aggiorna i golden con il motivo, secondo la regola esistente.
- **Riferimento:** il PCM 15126969907236845529 resta la baseline di M2.4 per tutti i confronti.
- **Vincolo artistico:** nessuna variazione aggiunta solo per far crescere una metrica. Il criterio resta «ricordi, sviluppi, sorprenda e ritorni»; la sua declinazione per milestone (P-M25-07) è un orientamento, non un contratto.
- **Soluzioni esistenti (titolare, 2026-11-04):** prima di scrivere un DSP non banale, come riverbero, chorus o campioni di percussioni, si valutano codice o materiali esistenti, secondo la regola document-driven. Una nuova dipendenza resta soggetta ad approvazione (AGENTS §2).
- **Alternativa scartata:** riscrittura del planner in M2.5, sproporzionata prima di avere misurato cosa si ottiene con l'architettura attuale.

### D-M2-20 — Livelli di qualità e regola di chiusura (P-M25-04)

- **Scelta:** si adottano i tre livelli e la tabella di chiusura della [proposta](../research/M2_5_Entry_Music_Quality_Proposal.md) §3 e §5. Il livello 1 è il gate normativo; il livello 2 è un obiettivo **non vincolante**; il livello 3 è un'ambizione.
- **Motivazione:** la regola va fissata prima di vedere i risultati, altrimenti si adatta ad essi. Non aggiunge criteri mandatory, come chiede la specifica §32.
- **Alternativa scartata:** rinviarla al gate finale, quando sarebbe influenzata dagli esiti.

## 3. Sotto-fasi

| Fase | Oggetto | Deliverable | Tocca il segnale? |
|---|---|---|---|
| **M2.5.0** | Ingresso | Questo ADR, stato in `progress.md` | no |
| **M2.5.1** | Prontezza e baseline | DIF-INC4-R3-05; prima corsa WASAPI ≥ 180 s sul contenuto attuale; test di caratterizzazione dei guasti latenti che M2.5.3 toccherà; loudness per stem; viste a pentagramma leggibili (P-M25-08); prova di fattibilità del cambio di metro, senza integrarla; strumenti offline d'insieme e di sviluppo proposti dal [dossier R-M25-7](../research/M2_5_F5_Music_Director_Research.md) | no |
| **M2.5.2** | Mix e immagine stereo | F4 (livello del kick, poi bilanciamento generale) e F2. Costanti di bus e di stereo, più il rapporto FM della Trama se CP-1 lo autorizza: piano e trace invariati, golden PCM aggiornato | sì |
| **M2.5.3** | Transizioni e struttura | F3 (prima la continuità dei pattern ai confini); F1 secondo CP-2; cambio di metro se deciso a CP-1. Golden di piano e trace aggiornati | sì |
| **M2.5.4** | Sviluppo mirato (**condizionale**) | Solo se CP-3 lo autorizza: variazione o nuovo suono entro l'architettura attuale, ablazione se il materiale la rende possibile | sì |
| **M2.5.5** | Congelamento e ricalibrazione | Applicazione di H-03 e H-04; contenuto congelato; ricalibrazione AC-AO-02 con verifica su insiemi separati; pacchetto con un seed nuovo; sessione alla cieca | solo H-03 |
| **M2.5.6** | Qualificazione integrata e chiusura | Build finale, `full_m2_suite`, AC-AO-01A con prova fisica SPACE, AC-AO-02, WASAPI ≥ 180 s, ascolto formale, dossier di qualità, review AGENTS §5.1.1 | no |

**Estensione di M2.5.1 (titolare, 2026-11-04).** La baseline su device ha limiti superabili, quindi la fase comprende anche, in quest'ordine: confronto del PCM catturato sul device con il render offline; prova limitata di cattura loopback a valle del mixer; carico controllato e misurato; ripetizione su una seconda uscita audio. La ricerca R-M25-8 precede il criterio PASS di CP-1.

**Ordine.** Il mix viene prima della struttura perché F1 si giudica solo con il kick al livello giusto. La prova su device viene prima degli effetti perché il loro costo va confrontato con il margine reale. M2.5.6 ripete tutte le verifiche: nessun esito delle fasi precedenti vale per la build finale.

## 4. Checkpoint

Ogni checkpoint è una decisione del titolare, presa sulle evidenze della fase conclusa. Gli esiti possibili sono: **procedi**, **approfondisci** (ricerca o prova limitata, §7) oppure **rinvia** (con la regola di D-M2-18).

| CP | Dopo | Domande | Evidenze |
|---|---|---|---|
| **CP-1** | M2.5.1 | Serve il cambio di metro? Quale criterio PASS per il device? Quanto margine DSP c'è per gli effetti? Quale protocollo d'ascolto? Il rapporto FM entra in M2.5.2? | Baseline su device, confronto PCM, loopback, R-M25-8, prova di fattibilità, confronto con la specifica |
| **CP-2** | M2.5.2 | Il mix è credibile? Con il kick più alto il metro regge ancora? F1 va corretto, e come? | Misure, A/B, ascolto con Q3 per sezione |
| **CP-3** | M2.5.3 | La transizione A→B è risolta? Serve una rampa di tempo? Dove va F5: una parte in M2.5.4 o tutto a M6? | Misure, ascolto, costo stimato |
| **CP-4** | M2.5.3 o M2.5.4 | Quale rilascio (H-03)? Quale normalizzazione (H-04)? Il contenuto è congelabile? Come si imposta la sessione alla cieca (H-05)? | 5e.3, 5e.3b, prova sul contenuto corrente |
| **CP-5** | M2.5.5 | Si ratificano le soglie AC-AO-02 (H-01, H-02)? Si può passare alla build finale? | Ricalibrazione, esito della sessione alla cieca |
| **Gate** | M2.5.6 | Regola di chiusura D-M2-20 | Tutte le evidenze di M2 |

Ogni checkpoint riesamina anche le voci di §8.1 che gli competono.

**Ascolti.** Sono previsti tre ascolti umani: CP-2, CP-3 e il gate. L'agente non ascolta e non conta come partecipante. Il titolare ha confermato (2026-11-03) che un secondo ascoltatore è fattibile, purché le domande siano semplici e adatte anche a chi non ha studiato musica. Il questionario si fissa con il protocollo a CP-1, e non aggiunge criteri mandatory.

## 5. Calendario delle decisioni

| Decisione | Quando | Momento limite | Perché lì |
|---|---|---|---|
| D-M2-17..20 (P-M25-04, -05, -07) | ingresso | M2.5.1 | Struttura e metodo servono a tutto il resto |
| P-M25-08, viste leggibili | review di M2.5.1 | primo ascolto | Forma scelta nell'implementazione |
| P-M25-01, cambio di metro | CP-1 | inizio di M2.5.3 | Serve la prova di fattibilità; dopo costerebbe rifare fixture e golden |
| Criterio PASS del device | CP-1 | prima di M2.5.6 | D-M2-10 fissa la corsa, non la soglia; serve la baseline |
| Margine DSP per gli effetti | CP-1 | prima di ogni effetto | Dipende dalla misura su device |
| LFO sul filtro, saturazione, supersaw e riassegnazione delle voci libere | CP-1 | inizio di M2.5.2 | Proposte dell'esperto del 2026-11-04: 3 voci su 8 sono ferme. Estendono il perimetro di M2.5.2 |
| F7, linea di basso | CP-2 | inizio di M2.5.3 | Sotto Corpo c'è solo il kick; va valutata con il kick (F4) e cambia il piano |
| Rapporto FM della Trama | CP-1 | inizio di M2.5.2 | Il modello indica un problema vicino al rapporto 2, da confermare all'ascolto; la costante non tocca piano e trace |
| P-M25-03, protocollo d'ascolto | CP-1 | prima dell'ascolto di CP-2 | Va fissato prima di ascoltare |
| Valori di F2 e F4 | review di M2.5.2 | CP-2 | Dipendono dalle prove A/B |
| F1 | CP-2 | inizio di M2.5.3 | Si giudica solo a mix corretto |
| Rampa di tempo per F3 | CP-3 | CP-4 | Solo se la continuità dei pattern non basta: richiede di estendere `MusicClock`, con un ADR |
| P-M25-06, F5 e ablazione | CP-3 | CP-4 | Serve sapere cosa ottengono i primi interventi. Vincoli di M7: «run diverse mostrano diversità misurabile» e «sessione infinita di almeno 10 minuti». La decisione riguarda quale milestone attua la specifica §16.3–§16.4, non un ampliamento del CIE Director. Il [dossier R-M25-7](../research/M2_5_F5_Music_Director_Research.md) raccomanda un contratto minimo in M2.5.4 e il §16.3–§16.4 a M6 come deliverable proprio, con una modifica della specifica §32 |
| H-03, rilascio | CP-4 | congelamento | Cambia il PCM, quindi precede il congelamento. Il giudizio percettivo non vincola, ma resta la motivazione tecnica del rivelatore (H-02, H-04) |
| H-04, normalizzazione | CP-4 | ricalibrazione | Eventuale `descriptors_v2`; la ricalibrazione di δ dipende da H-03 |
| H-05 e sessione alla cieca | CP-4 | M2.5.5 | Dipendono dalla varietà del contenuto congelato |
| Evidenza dell'adapter di AC-AO-02 | CP-4 | M2.5.6 | Stabilire se serve una sessione reale. Un esito `INSUFFICIENT_DATA` su sensibilità o specificità non è un fallimento |
| P-M25-02, H-01, H-02 | CP-5 | prima delle misure finali | Le soglie attuali derivano dagli stem di oggi. Si fissano dopo il congelamento, prima di misurare la build finale |
| Separazione dei dati per AC-AO-02 | CP-4 | prima di M2.5.5 | Evita il leakage: dichiarare quali dati calibrano, quali verificano le soglie da ratificare a CP-5 e quali servono alla misura finale, che usa soglie congelate. Se il materiale coincide, va dichiarato come limite |
| Stato dei profili non validati | gate | gate | Restano non `validated` salvo evidenza |

## 6. Regola di avanzamento

Vale la regola di Inc-5, una fase alla volta:

1. l'agente esegue solo la fase autorizzata e presenta diff, test ed evidenze;
2. si ferma senza commit né push;
3. segue la review indipendente, e le correzioni restano nella stessa fase;
4. con l'approvazione esplicita del titolare si committa, e il checkpoint autorizza la fase successiva.

Ogni fase che tocca il codice si chiude con build Release, Debug e Headless, 0 warning `/W4` contati sui log, il 100 % dei test e la CI verde. M2.5.2, M2.5.3 e M2.5.4 aggiungono `full_m2_suite` e l'ispezione CACP di AGENTS §5.1.2.

## 7. Scelte d'ordine rispetto al parere esterno

| Tema | Parere esterno | Questo ADR | Motivo |
|---|---|---|---|
| Soglie AC-AO-02 | all'ingresso | CP-5 | F2, F4 e H-03 cambiano gli stem, e la calibrazione attuale riguarda il segnale di oggi. Ratificarle ora le renderebbe obsolete. Restano fissate prima delle misure finali |
| Protocollo e criteri | all'ingresso | regola di chiusura ora, protocollo a CP-1 | La regola non dipende da misure; il protocollo sì, e serve solo prima del primo ascolto |
| Device | prepararlo, poi interventi, poi validazione | anche una baseline in M2.5.1 | Il costo degli effetti va confrontato con il margine reale, e un problema di underrun va scoperto prima |
| Rilascio | invariato salvo nuove motivazioni | aperto fino a CP-4 | Una motivazione tecnica esiste già: rilevazioni sui rilasci e δ della Trama calibrato sui transitori di 2 ms |
| F1 | da rivalutare con F4 | CP-2 | Coincide. In più DESIGN §8.1 chiede che Pulso «aiuti a prevedere», rilevante da M3: il «Sì» a Q3 non chiude la questione (punto condiviso dal titolare il 2026-11-03) |

**Ricerche limitate, solo se un checkpoint le richiede** (con fonti lette e dati nel repository):

| ID | Tema | Quando |
|---|---|---|
| R-M25-1 | Bilanciamento: rumore rosa, curve isofoniche, loudness per stem | M2.5.2 |
| R-M25-2 | Immagine stereo: precedenza e alternative al delay 11/17 ms | M2.5.2 |
| R-M25-3 | Effetti (riverbero, chorus, saturazione): costo DSP rispetto a D-M2-10 e ADR-0004 | dopo CP-1 |
| R-M25-4 | Transizioni: continuità dei pattern contro rampe di tempo | M2.5.3 |
| R-M25-5 | Onset: normalizzazione robusta o locale (H-04) | prima di CP-4 |
| R-M25-6 | Statistica per gruppi e permutazioni (H-05) | prima di CP-4 |
| R-M25-9 | Palette timbrica: a CP-3 l'obiettivo artistico (evocare strumenti acustici, creare materiali impossibili o combinarli), senza scegliere un algoritmo; poi, se serve, confronto fra campioni, modelli fisici, sintesi modale e soluzioni ibride per memoria, CPU, licenze, determinismo e budget di voci, con uno spike limitato prima del gate di M7. Criteri: riconoscibilità dei ruoli, espressività, coerenza nel mix, assenza di artefatti; il realismo è un mezzo, non il fine | CP-3 e prima di M7, non in M2.5.2 |
| R-M25-8 | Rilevazione dei glitch audio su Windows: eventi di sistema (ETW e simili), contatori e strumenti equivalenti, con ciò che misurano davvero e i limiti in modalità condivisa | prima di CP-1 |
| R-M25-7 | Direttore musicale (F5): stato dell'arte dei «conductor», già chiesto dal titolare il 2026-10-28, compreso il coordinamento fra i quattro ruoli (indicazione del titolare del 2026-11-03). **Eseguita:** [dossier R-M25-7](../research/M2_5_F5_Music_Director_Research.md) | prima di CP-3 |

## 8. Destinazione dei punti aperti

| Punto aperto | Destinazione |
|---|---|
| DIF-INC4-R3-05 | M2.5.1 |
| Qualificazione realtime su device | baseline in M2.5.1, gate in M2.5.6 |
| Limiti della baseline (underrun non esposti, PCM non confrontato, carico non misurato, una sola uscita) | M2.5.1, estensione del §3 |
| R-06, guasti latenti | M2.5.1 per quelli toccati da M2.5.3 (`barsPerPhrase`, `phrasePhase`, trasposizione al bordo, pad incrociati); gli altri in M2.5.6 o rinvio motivato |
| «Â§» nel manifest; OSS-5e.3b-R2-01 | M2.5.1 |
| 0 warning contati solo sui log (`/WX` assente) | M2.5.1: decidere se farne un controllo automatico |
| OSS-M2.4-CL-02, test in parallelo | M2.5.1 se i percorsi di uscita sono indipendenti, altrimenti rinvio |
| OSS-5e.2-R2-01, staging dei pacchetti; AG-ADR-03 | M2.5.5, con il nuovo pacchetto |
| R-07, pulizie | quando si toccano quei file |
| F1–F4, F6 | §3 e §5 |
| F5, R-02, ablazione | CP-3; la varietà fra seed resta comunque legata al gate di M7 |
| H-01..H-05, P-M25-01..08 | §5 |
| Sessione alla cieca, R1 e R3 del dossier CACP | M2.5.5 |
| AC-AO-01A, riverifica con prova SPACE | M2.5.6 |
| AC-AO-01B | facoltativo |
| Dossier di qualità dell'addendum (clipping, aliasing, voice stealing, budget, ascolto) | M2.5.6 |
| FTZ/DAZ per thread | solo se la baseline su device lo motiva |
| Strumenti realistici e librerie di suoni | R-M25-9, da approfondire nel momento opportuno; oggi non è un obiettivo dichiarato |
| `PlayerProfile_v1` | M5/M7, fuori da M2.5 |

### 8.1 Suggerimenti dell'esperto

Registro unico dei suggerimenti raccolti il [2026-10-28](../m2_4_human_listening_2026-10-28.md) e il [2026-11-03 e 04](../m2_4_human_listening_2026-11-03.md). Nessuno è adottato: ognuno ha una destinazione, e ogni voce si chiude con una decisione registrata.

**Regola (titolare, 2026-11-04, AGENTS §5.3):** ogni suggerimento dell'esperto entra in questa tabella quando arriva. Ogni checkpoint riesamina le voci con quella scadenza e registra l'esito: adottata, rinviata con nuova destinazione, o scartata con motivo.

| Suggerimento | Destinazione |
|---|---|
| Linea di basso (urgente) | F7, CP-2 |
| Kick: più peso nelle basse, durata oltre i 54–62 ms attuali | F4, M2.5.2 |
| Kick campionato o drum kit in un campionatore | R-M25-9, confronto A/B con il kick sintetico |
| Backbeat e ghost notes per ancorare il metro | F1, CP-2 |
| LFO sul filtro a quarti o ottavi, su pad o lead | CP-1, poi M2.5.2 |
| Saturazione ed eventuale distorsione, sul synth o sul master | CP-1, poi M2.5.2 |
| Supersaw sul lead: due voci all'unisono, leggermente stonate e aperte | CP-1, poi M2.5.2 |
| Modulazioni di altri parametri, per esempio l'attacco | CP-1, come estensione dell'LFO |
| Rifare il delay della Trama, oggi lateralizzato | F2, M2.5.2 |
| Giudicare la larghezza stereo a orecchio: il Side/Mid può dire il contrario del percepito | F2 e R-M25-2: bilanciamento fra i canali più ascolto |
| Bilanciamento con rumore rosa e curve isofoniche | R-M25-1, M2.5.2 |
| Riverberi di durate diverse, con livelli prudenti | R-M25-3, dopo CP-1 |
| Chorus, flanger, delay con feedback | R-M25-3, dopo CP-1 |
| Compressori solo come scelta stilistica | M2.5.2, nessun compressore di default |
| Transizioni meno brusche: rampe di BPM o modulazioni ritmiche | F3, CP-3 |
| Variazione melodica decisa o nuovo suono dopo il secondo loop | F3 e F5, CP-3 |
| VST open source con preset. Il determinismo non lo esclude a priori: su molti synth la fase degli oscillatori si può bloccare; provare render ripetuti e con blocchi diversi, e distinguere il bit esatto dei golden dal replay, che dipende da eventi e piano | R-M25-9 |
| Viste con nomi di strumenti e griglia | P-M25-08, M2.5.1 |
| Ascoltare a un volume che permetta di giudicare il kick | P-M25-03, CP-1 |
| Null test del rilascio: residuo di −14 dB su clip giudicate uguali, con il rilascio cambiato su tutti i ruoli insieme. Esportare il residuo, misurarlo per ruolo e nel tempo, escludere errori nelle clip, provare un A/B per ruolo a partire dal kick. Ipotesi da verificare: kick chiuso quando l'inviluppo è già al 14–35 %, quindi serve più decay e non più rilascio; FM mascherata da livello basso e delay; residuo dovuto alle code sovrapposte di pad e lead | H-03, prima di CP-4. Fino ad allora il ridimensionamento di F6 è provvisorio |
| Ipotesi del titolare (2026-11-05): parte del problema è la dissolvenza di 2 ms quando una voce viene interrotta. Verifica sul piano e sul codice: i ruoli non si interrompono fra loro, ma i 2 ms esistono in tre casi. Ogni NoteOff ordinario. I 190 tagli di rilascio di Corpo: a ogni cambio d'accordo la nota nuova riusa la voce appena rilasciata, che sfuma sempre in 2 ms, a qualunque rilascio impostato: l'A/B del rilascio non ha mai variato questo punto (190 in tutte e 5 le varianti). L'attacco del ducking di 2 ms con cui Pulso abbassa Corpo. Effetto udibile non verificato | H-03, prima di CP-4: null test per ruolo, poi A/B offline che cambi solo la durata delle code di Corpo dovute al riutilizzo delle voci. Pause e Stop restano separati: sono dissolvenze di sicurezza, vincolate ai tempi approvati |
| Null test: invertire di fase una clip e sommarla all'altra per capire quanto è cambiato | D-M2-19: da riportare in ogni confronto A/B, accanto all'hash. Residuo molto basso: nulla di significativo è cambiato. Residuo alto: non prova una differenza udibile (clip del rilascio: −14 dB, giudicate uguali) |
| Non rincorrere un synth universale: modulazioni e algoritmi efficienti sono una sfida enorme | Vincolo per R-M25-9 e D-M2-19: aggiungere solo ciò che serve al suono di CYMATICA |

## 9. Verifica di M2.5.0

- Nessuna modifica al runtime C++, ai test o ai golden: build e test del progetto non applicabili. Il commit aggiunge solo documenti e strumenti offline di ricerca in `docs/research/experiments/`.
- Link interni controllati; ogni ID del §5 e del §8 ha un riscontro nei documenti di handoff.
- Review del titolare: approvazione di D-M2-17..20. M2.5.1 parte solo con un'autorizzazione esplicita.
