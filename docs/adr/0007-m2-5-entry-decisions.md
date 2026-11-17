# ADR-0007 — Ingresso in M2.5: sotto-fasi, checkpoint e calendario delle decisioni

- **Data:** 2026-11-03
- **Stato:** `accepted for implementation`: D-M2-17..20 approvate dal titolare il 2026-11-04 (fase M2.5.0); D-M2-21..29, decisioni di CP-1, approvate il 2026-11-16 (§10). Le altre decisioni del §5 restano ai rispettivi checkpoint. M2.5.1 è autorizzata dal titolare; stato in [progress.md](../progress.md).
- **Milestone:** M2 — sottofase M2.5, «Integrazione e verifica finale».
- **Owner:** coordinatore dello sviluppo, su richiesta del titolare del 2026-11-03: dividere M2.5 in sotto-fasi con checkpoint, chiudere i punti aperti e rinviare le decisioni al momento opportuno anziché prenderle tutte all'ingresso.
- **Gerarchia:** Specifica > DESIGN > AGENTS > ADR > piano operativo > ricerca. Questo ADR **organizza** M2.5 e non modifica:
  - i criteri di M2 e di M2.5 della specifica §32, salvo l'emendamento esplicito 0.8.5 deciso dal titolare a CP-1 (D-M2-21, §10; OSS-CP1-01);
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
| **M2.5.1** | Prontezza e baseline | DIF-INC4-R3-05; prima corsa WASAPI ≥ 180 s sul contenuto attuale; test di caratterizzazione dei guasti latenti che M2.5.3–4 toccherà; loudness per stem; viste a pentagramma leggibili (P-M25-08); prova di fattibilità del cambio di metro, senza integrarla; strumenti offline d'insieme e di sviluppo proposti dal [dossier R-M25-7](../research/M2_5_F5_Music_Director_Research.md) | no |
| **M2.5.2** | Mix e immagine stereo | F4 (livello del kick, poi bilanciamento generale) e F2. Costanti di bus e di stereo, più il rapporto FM della Trama se CP-1 lo autorizza: piano e trace invariati, golden PCM aggiornato | sì |
| **M2.5.3–4** | Struttura e sviluppo (D-M2-32, **non più condizionale**) | Fase unica dopo la chiusura di M2.5.2: F3 (prima la continuità dei pattern ai confini), F1 come àncora metrica dichiarata, cambio di metro se deciso a CP-1 (D-M2-21: scenario tecnico), e il contratto minimo di R-M25-7 (opzione B), un cambiamento per pacchetto. Verifica: rimescolamento, ablazione, fixture avverse, variogramma contro nullo, F7. Un solo ascolto umano cumulativo a fine fase. Golden di piano, trace e PCM aggiornati con il motivo | sì |
| **M2.5.5** | Congelamento e ricalibrazione | Applicazione di H-03 e H-04; contenuto congelato; ricalibrazione AC-AO-02 con verifica su insiemi separati; pacchetto con un seed nuovo; sessione alla cieca | solo H-03 |
| **M2.5.6** | Qualificazione integrata e chiusura | Build finale, `full_m2_suite`, AC-AO-01A con prova fisica SPACE, AC-AO-02, WASAPI ≥ 180 s, ascolto formale, dossier di qualità, review AGENTS §5.1.1 | no |

**Estensione di M2.5.1 (titolare, 2026-11-04).** La baseline su device ha limiti superabili, quindi la fase comprende anche, in quest'ordine: confronto del PCM catturato sul device con il render offline; prova limitata di cattura loopback a valle del mixer; carico controllato e misurato; ripetizione su una seconda uscita audio. La ricerca R-M25-8 precede il criterio PASS di CP-1.

**Ordine.** Il mix viene prima della struttura perché F1 si giudica solo con il kick al livello giusto. La prova su device viene prima degli effetti perché il loro costo va confrontato con il margine reale. M2.5.6 ripete tutte le verifiche: nessun esito delle fasi precedenti vale per la build finale.

## 4. Checkpoint

Ogni checkpoint è una decisione del titolare, presa sulle evidenze della fase conclusa. Gli esiti possibili sono: **procedi**, **approfondisci** (ricerca o prova limitata, §7) oppure **rinvia** (con la regola di D-M2-18).

| CP | Dopo | Domande | Evidenze |
|---|---|---|---|
| **CP-1** | M2.5.1 | Serve il cambio di metro? Quale criterio PASS per il device? Quanto margine DSP c'è per gli effetti? Quale protocollo d'ascolto? Il rapporto FM entra in M2.5.2? | Baseline su device, confronto PCM, loopback, R-M25-8, prova di fattibilità, confronto con la specifica |
| **CP-2** | M2.5.2 | Il mix è credibile? Con il kick più alto il metro regge ancora? F1 va corretto, e come? | Misure e A/B misurati; per D-M2-30 nessun ascolto umano dedicato: Q3 per sezione passa al primo ascolto cumulativo |
| **CP-3** | M2.5.3–4 | La transizione A→B è risolta? Serve una rampa di tempo? Lo sviluppo è autentico (non fittizio)? Quanto di F5 resta a M6? | Misure, prove simboliche di R-M25-7, ascolto cumulativo a fine fase (D-M2-30, D-M2-32) |
| **CP-4** | M2.5.3–4 | Quale rilascio (H-03)? Quale normalizzazione (H-04)? Il contenuto è congelabile? Come si imposta la sessione alla cieca (H-05)? | 5e.3, 5e.3b, prova sul contenuto corrente |
| **CP-5** | M2.5.5 | Si ratificano le soglie AC-AO-02 (H-01, H-02)? Si può passare alla build finale? | Ricalibrazione, esito della sessione alla cieca |
| **Gate** | M2.5.6 | Regola di chiusura D-M2-20 | Tutte le evidenze di M2 |

Ogni checkpoint riesamina anche le voci di §8.1 che gli competono.

**Ascolti.** Sono previsti tre ascolti umani: CP-2, CP-3 e il gate. **Emendato da D-M2-30 (2026-11-16):** gli ascolti umani si fanno solo su materiale maturo e cumulativo; quello di CP-2 confluisce nel primo ascolto successivo di questo tipo. L'agente non ascolta e non conta come partecipante. Il titolare ha confermato (2026-11-03) che un secondo ascoltatore è fattibile, purché le domande siano semplici e adatte anche a chi non ha studiato musica. Il questionario si fissa con il protocollo a CP-1, e non aggiunge criteri mandatory.

## 5. Calendario delle decisioni

| Decisione | Quando | Momento limite | Perché lì |
|---|---|---|---|
| D-M2-17..20 (P-M25-04, -05, -07) | ingresso | M2.5.1 | Struttura e metodo servono a tutto il resto |
| P-M25-08, viste leggibili | review di M2.5.1 | primo ascolto | Forma scelta nell'implementazione |
| P-M25-01, cambio di metro | CP-1 | inizio di M2.5.3–4 | [Prova di fattibilità](../research/M2_5_1b_Meter_Feasibility.md) eseguita in M2.5.1b; dopo costerebbe rifare fixture e golden |
| Criterio PASS del device | CP-1 | prima di M2.5.6 | D-M2-10 fissa la corsa, non la soglia; serve la baseline. Proposta in [dossier R-M25-8](../research/M2_5_1_Windows_Audio_Glitch_Detection.md): controllo positivo per uscita, PCM, intervalli, `IAudioClock`, loopback e ascolto umano |
| Canale ETW `/GlitchDetection` | CP-1 | prima di M2.5.6 | Attivarlo cambia una configurazione di sistema e richiede privilegi: decisione del titolare |
| Margine DSP per gli effetti | CP-1 | prima di ogni effetto | Dipende dalla misura su device |
| LFO sul filtro, saturazione, supersaw e riassegnazione delle voci libere | CP-1 | inizio di M2.5.2 | Proposte dell'esperto del 2026-11-04: 3 voci su 8 sono ferme. Estendono il perimetro di M2.5.2 |
| F7, linea di basso | CP-2 | inizio di M2.5.3–4 | Sotto Corpo c'è solo il kick; va valutata con il kick (F4) e cambia il piano |
| Rapporto FM della Trama | CP-1 | inizio di M2.5.2 | Il modello indica un problema vicino al rapporto 2, da confermare all'ascolto; la costante non tocca piano e trace |
| P-M25-03, protocollo d'ascolto | CP-1 | prima dell'ascolto di CP-2 | Va fissato prima di ascoltare |
| Valori di F2 e F4 | review di M2.5.2 | CP-2 | Dipendono dalle prove A/B |
| F1 | CP-2 | inizio di M2.5.3–4 | Si giudica solo a mix corretto |
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

Ogni fase che tocca il codice si chiude con build Release, Debug e Headless, 0 warning `/W4` contati sui log, il 100 % dei test e la CI verde. M2.5.2 e M2.5.3–4 aggiungono `full_m2_suite` e l'ispezione CACP di AGENTS §5.1.2.

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
| R-M25-3 | Effetti (riverbero, chorus, saturazione): costo DSP rispetto a D-M2-10 e ADR-0004. Parte anticipata eseguita per CP-1 ([risultati](../research/CP1_RM25_3_Early_Results.md)) | dopo CP-1 |
| R-M25-10 | Stress con controllo positivo del percorso audio e confronto controbilanciato Default contro Games (MMCSS); aggiunta da D-M2-28 | prima di M2.5.6 |
| R-M25-12 | Punto di cattura del loopback rispetto al volume. **Eseguita e chiusa:** [risultati](../research/experiments/cp1_volume_loopback/README.md) (cursore per app applicato come livello² prima della cattura) | CP-1 |
| R-M25-4 | Transizioni: continuità dei pattern contro rampe di tempo | M2.5.3–4 |
| R-M25-5 | Onset: normalizzazione robusta o locale (H-04) | prima di CP-4 |
| R-M25-6 | Statistica per gruppi e permutazioni (H-05) | prima di CP-4 |
| R-M25-9 | Palette timbrica: a CP-3 l'obiettivo artistico (evocare strumenti acustici, creare materiali impossibili o combinarli), senza scegliere un algoritmo; poi, se serve, confronto fra campioni, modelli fisici, sintesi modale e soluzioni ibride per memoria, CPU, licenze, determinismo e budget di voci, con uno spike limitato prima del gate di M7. Criteri: riconoscibilità dei ruoli, espressività, coerenza nel mix, assenza di artefatti; il realismo è un mezzo, non il fine | CP-3 e prima di M7, non in M2.5.2 |
| R-M25-8 | Rilevazione dei glitch audio su Windows: eventi di sistema (ETW e simili), contatori e strumenti equivalenti, con ciò che misurano davvero e i limiti in modalità condivisa. **Eseguita:** [dossier R-M25-8](../research/M2_5_1_Windows_Audio_Glitch_Detection.md) | prima di CP-1 |
| R-M25-7 | Direttore musicale (F5): stato dell'arte dei «conductor», già chiesto dal titolare il 2026-10-28, compreso il coordinamento fra i quattro ruoli (indicazione del titolare del 2026-11-03). **Eseguita:** [dossier R-M25-7](../research/M2_5_F5_Music_Director_Research.md) | prima di CP-3 |

## 8. Destinazione dei punti aperti

| Punto aperto | Destinazione |
|---|---|
| DIF-INC4-R3-05 | M2.5.1 |
| Qualificazione realtime su device | baseline in M2.5.1, gate in M2.5.6 |
| Limiti della baseline (underrun non esposti, PCM non confrontato, carico non misurato, una sola uscita) | M2.5.1, estensione del §3 |
| R-06, guasti latenti | M2.5.1 per quelli toccati da M2.5.3–4 (`barsPerPhrase`, `phrasePhase`, trasposizione al bordo, pad incrociati); gli altri in M2.5.6 o rinvio motivato |
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
| Kick: più peso nelle basse, durata oltre i 54–62 ms attuali | F4, M2.5.2. **Livello adottato** (+8 dB, D-M2-31). **Durata: one-shot** (D-M2-35) e **decadimento ×1,5** (D-M2-37): da 54–62 ms troncati a 116 ms; quota sotto 100 Hz da 0,74 a 0,82. Giudizio finale all'ascolto cumulativo (registro delle domande, voce 5) |
| Kick campionato o drum kit in un campionatore | R-M25-9, confronto A/B con il kick sintetico |
| Backbeat e ghost notes per ancorare il metro | F1, CP-2 |
| LFO sul filtro a quarti o ottavi, su pad o lead | **Adottata** a CP-1 come A/B in M2.5.2 (D-M2-29) |
| Saturazione ed eventuale distorsione, sul synth o sul master | **Adottata** a CP-1 come A/B in M2.5.2, ADAA o sovracampionamento secondo la misura (D-M2-29) |
| Supersaw sul lead: due voci all'unisono, leggermente stonate e aperte | **Adottata** a CP-1 come A/B in M2.5.2, fasi deterministiche (D-M2-29) |
| Modulazioni di altri parametri, per esempio l'attacco | **Rinviata** a CP-1: dopo l'esito dell'A/B dell'LFO in M2.5.2 (D-M2-29) |
| Rifare il delay della Trama, oggi lateralizzato | F2, M2.5.2. **Pan centrato** (D-M2-34): primo fronte centrato; il delay asimmetrico lascia circa −1,5 dB sull'intera nota, da trattare in F2b con una sua pre-registrazione |
| Giudicare la larghezza stereo a orecchio: il Side/Mid può dire il contrario del percepito | F2 e R-M25-2: bilanciamento fra i canali più ascolto |
| Bilanciamento con rumore rosa e curve isofoniche | R-M25-1, M2.5.2 |
| Riverberi di durate diverse, con livelli prudenti | R-M25-3, dopo CP-1 |
| Chorus, flanger, delay con feedback | R-M25-3, dopo CP-1 |
| Compressori solo come scelta stilistica | M2.5.2, nessun compressore di default |
| Transizioni meno brusche: rampe di BPM o modulazioni ritmiche | F3, CP-3 |
| Variazione melodica decisa o nuovo suono dopo il secondo loop | F3 e F5, CP-3 |
| VST open source con preset. Il determinismo non lo esclude a priori: su molti synth la fase degli oscillatori si può bloccare; provare render ripetuti e con blocchi diversi, e distinguere il bit esatto dei golden dal replay, che dipende da eventi e piano | R-M25-9 |
| Viste con nomi di strumenti e griglia | P-M25-08, M2.5.1 |
| Ascoltare a un volume che permetta di giudicare il kick | **Adottata** a CP-1 nel protocollo d'ascolto (D-M2-27, D-M2-29) |
| Null test del rilascio: residuo di −14 dB su clip giudicate uguali, con il rilascio cambiato su tutti i ruoli insieme. Esportare il residuo, misurarlo per ruolo e nel tempo, escludere errori nelle clip, provare un A/B per ruolo a partire dal kick. Ipotesi da verificare: kick chiuso quando l'inviluppo è già al 14–35 %, quindi serve più decay e non più rilascio; FM mascherata da livello basso e delay; residuo dovuto alle code sovrapposte di pad e lead | H-03, prima di CP-4. Fino ad allora il ridimensionamento di F6 è provvisorio |
| Ipotesi del titolare (2026-11-05): parte del problema è la dissolvenza di 2 ms quando una voce viene interrotta. Verifica sul piano e sul codice: i ruoli non si interrompono fra loro, ma i 2 ms esistono in tre casi. Ogni NoteOff ordinario. I 190 tagli di rilascio di Corpo: a ogni cambio d'accordo la nota nuova riusa la voce appena rilasciata, che sfuma sempre in 2 ms, a qualunque rilascio impostato: l'A/B del rilascio non ha mai variato questo punto (190 in tutte e 5 le varianti). L'attacco del ducking di 2 ms con cui Pulso abbassa Corpo. Effetto udibile non verificato | H-03, prima di CP-4: null test per ruolo, poi A/B offline che cambi solo la durata delle code di Corpo dovute al riutilizzo delle voci. Pause e Stop restano separati: sono dissolvenze di sicurezza, vincolate ai tempi approvati |
| Null test: invertire di fase una clip e sommarla all'altra per capire quanto è cambiato | D-M2-19: da riportare in ogni confronto A/B, accanto all'hash. Residuo molto basso: nulla di significativo è cambiato. Residuo alto: non prova una differenza udibile (clip del rilascio: −14 dB, giudicate uguali) |
| Non rincorrere un synth universale: modulazioni e algoritmi efficienti sono una sfida enorme | Vincolo per R-M25-9 e D-M2-19: aggiungere solo ciò che serve al suono di CYMATICA |

## 9. Verifica di M2.5.0

- Nessuna modifica al runtime C++, ai test o ai golden: build e test del progetto non applicabili. Il commit aggiunge solo documenti e strumenti offline di ricerca in `docs/research/experiments/`.
- Link interni controllati; ogni ID del §5 e del §8 ha un riscontro nei documenti di handoff.
- Review del titolare: approvazione di D-M2-17..20. M2.5.1 parte solo con un'autorizzazione esplicita.

## 10. Decisioni di CP-1 (2026-11-16)

Il titolare ha approvato il 2026-11-16 tutte le proposte del [dossier delle decisioni di CP-1](../CP1_decision_dossier.md), con un'annotazione su D-M2-27. Le evidenze, le alternative e i motivi sono nel dossier; qui resta il registro unico. Stato di tutte: `accepted for implementation`, nessuna `validated`.

| ID | Decisione (dossier) | Esito approvato | Contratti toccati | Verifica |
|---|---|---|---|---|
| **D-M2-21** | Cambio di metro, P-M25-01 (D1) | **Opzione B + C.** La fixture resta in 4/4; la specifica §32 è emendata (0.8.5): in M2 la transizione obbligatoria nel contenuto è quella di tempo, e il cambio di time signature via `SetTempoMap` è qualificato da uno scenario tecnico ripetibile con manifest, distinto dalla fixture musicale. Un metro diverso come scelta musicale resta possibile più avanti con R-M25-11 | Specifica §32 (M2 e riga M2.5); nessun golden | Scenario tecnico 4/4 → 3/4 → 4/4 con manifest, promosso dalla [prova di fattibilità](../research/M2_5_1b_Meter_Feasibility.md); destinazione: M2.5.3, al più tardi prima del gate |
| **D-M2-22** | Criterio PASS del device (D2) | Per ogni uscita qualificata, tutti insieme: controllo positivo `glitchcal` superato, ripetuto dopo ogni cambio di driver; binario compilato da un commit; manifest con endpoint e formato attesi, rerouting disattivato e client invariato, cursore dell'app a 1 e volume di sistema registrati; PCM della callback uguale al golden; `suspected_underruns` 0; monitor `IAudioClock` valido per tutta la corsa, cioè `device_clock.status` `MEASURED`, client invariato, **0 letture fallite** e copertura temporale non inferiore a una soglia minima, con intervallo massimo fra letture non superiore a un limite, entrambi **fissati in una pre-registrazione prima della qualificazione** e motivati anche dalla copertura osservata a CP-1 (circa 80 %, K1 mancata), senza adattarli a posteriori; con il monitor valido, nessun salto oltre 5 ms (DIF-CP1-D22-01); loopback con 0 slittamenti, silenzi, raffiche e overflow; ascolto umano della stessa corsa senza artefatti; **tre corse** per uscita. Un rilevatore che non reagisce al controllo positivo rende la corsa `INVALID` | D-M2-10 resta la corsa; questo è la soglia | Al gate M2.5.6, su ogni uscita dichiarata; limite dichiarato sugli eventi rari |
| **D-M2-23** | Canale ETW `/GlitchDetection` (D3) | **Non attivato.** Da riconsiderare se un artefatto viene sentito e nessun rilevatore lo vede, o per un'uscita senza loopback affidabile | Nessuno | Riesame se ricorre la condizione |
| **D-M2-24** | Default di MMCSS nel runtime (D4) | **Rinviato a R-M25-10**; il default non cambia, il probe usa `games` | Nessuno | R-M25-10, prima di M2.5.6 |
| **D-M2-25** | Margine DSP per gli effetti (D5) | Margine indicativo ≈ 1,6 ms per 512 frame (D-M2-10). Ogni effetto entra solo con la sua misura nel profilo D-M2-10 e con l'azzeramento dei denormali nelle strutture a retroazione; riverbero, chorus e delay restano a R-M25-3 | D-M2-10, ADR-0004 | Misura di ogni effetto prima dell'integrazione |
| **D-M2-26** | Perimetro di M2.5.2 (D6) | F4 (kick), F2 (stereo della Trama) e **rapporto FM della Trama**, ciascuno con A/B, null test e golden aggiornato con motivo. LFO sul filtro, saturazione e supersaw entrano come A/B successivi, uno per pacchetto, dopo F4, con misura di costo e ipotesi registrata prima dell'ascolto; la supersaw usa fasi da uno stream deterministico isolato. La riassegnazione delle voci libere richiede un ADR dedicato, fuori da M2.5 | Golden PCM (ogni intervento); D-M2-03 invariato | Gli A/B misurati; l'ascolto cumulativo successivo (D-M2-30) |
| **D-M2-27** | Protocollo d'ascolto, P-M25-03 (D7) | **Adottato**: [protocollo](../m2_5_listening_protocol_P-M25-03.md). **Annotazione del titolare:** in una sessione di 45 minuti al massimo va garantita varietà per non annoiare l'ascoltatore; recepita nel protocollo (§2bis) | Procedura degli ascolti di CP-2, CP-3 e gate | Il modulo di risposta di ogni sessione |
| **D-M2-28** | Ricerche R-M25-10..12 (D8) | R-M25-10 aggiunta al §7; R-M25-12 eseguita e chiusa; R-M25-11 non aggiunta, perché D-M2-21 non sceglie l'opzione A | §7 | §7 |
| **D-M2-29** | Voci di §8.1 in scadenza a CP-1 (D9) | LFO, saturazione e supersaw adottate come A/B in M2.5.2 (D-M2-26); modulazioni di altri parametri rinviate a dopo l'esito dell'LFO; volume per giudicare il kick adottato nel protocollo (D-M2-27) | §8.1 | §8.1 |

## 11. Decisioni del titolare durante M2.5.2 (2026-11-16)

Prese dal titolare il 2026-11-16 dopo aver ascoltato il brano del pacchetto CP-2 di F4a. Il pacchetto conteneva lo stesso brano del 2026-11-03 con il solo kick più alto. Stato: `accepted for implementation`.

| ID | Decisione | Motivo | Alternative scartate | Contratti toccati | Verifica |
|---|---|---|---|---|---|
| **D-M2-30** | **Ascolti umani solo su materiale maturo; verifica autonoma delle variazioni misurabili.** Una variazione piccola e misurabile, come un livello, un guadagno di bus o una costante di bilanciamento, si decide e si verifica senza ascolto umano. Valgono criteri oggettivi pre-registrati (livelli in LU, headroom e limiter, null test, invarianza degli altri ruoli) e i giudizi umani già raccolti. Se una variazione di volume non è verificabile in autonomia, è un difetto degli strumenti da correggere, non un motivo per chiamare un ascoltatore. L'ascolto umano resta per le domande che le misure non coprono (coerenza, interesse, sviluppo, «ricordi, sviluppi, sorprenda e ritorni»). Si fa ai checkpoint, su materiale cumulativo, più maturo e completo. | Titolare: per 6 dB di kick un ascolto umano non vale il disturbo; far riascoltare alla stessa persona lo stesso brano non ha senso | Ascolto dedicato per ogni intervento (pacchetto CP-2 di F4a, archiviato senza invio) | §4, colonna CP-2 e paragrafo «Ascolti»; D-M2-26, colonna verifica; il [protocollo P-M25-03](../m2_5_listening_protocol_P-M25-03.md) resta valido per gli ascolti che si faranno | Ogni intervento di M2.5.2 ha la sua pre-registrazione e le sue misure; il primo ascolto cumulativo raccoglie Q3 per sezione e le domande di CP-2 |
| **D-M2-31** | **Livello del kick (F4a): +8 dB sul bus di Pulso**, da −6 a +2 dB (`kPulsoGain` 1,2589). È il centro della stima dell'esperto del 2026-11-03 (kick 7–9 dB sotto gli altri ruoli). Il ducking di Corpo segue il guadagno (`kDuckingFullScaleLevel` = 0,25 · `kPulsoGain`), quindi resta invariato. **Cambia la regola di decisione** della [pre-registrazione F4a](../research/M2_5_2_F4_Kick_Level_Preregistration.md) §5, che affidava il valore all'ascolto di CP-2: il file firmato non si modifica, e le ipotesi d'ascolto L1, L2 e S1–S3 restano non verificate | Il giudizio umano esiste già; le misure di `kick_level_ab_v1` (P1–P7 confermate) dicono che fra +6 e +9 dB il limiter non interviene e gli altri ruoli restano invariati | +6 dB (sotto la stima); +9 dB (bordo della stima); attendere un ascolto (escluso da D-M2-30) | Gain staging di ADR-0003 D-M2-06 («calibrabile senza alterare contratti pubblici»); golden PCM della fixture, aggiornato con questo motivo | [Addendum di adozione](../research/M2_5_2_F4_Kick_Level_Adoption_Preregistration.md), predizioni registrate prima della misura |
| **D-M2-33** | **Rapporto FM della Trama 1:1 e bus della Trama −9,0 dB** (prima m/c 1,88 e −10 dB). Scelta con la regola pre-registrata C1–C3 sul modello di Sethares; la compensazione di +1 dB riallinea la loudness della Trama (A/B di timbro a loudness allineata, indicazione della review di CP-1) | Autodissonanza da 0,83 a 0,001; dissonanza di contesto sopra Corpo da 1,17 a 0,49; giudizio dell'esperto «un po' dissonante» | 2,0 (non supera C2: tritono più consonante della quinta); √2 (non supera C1 e C2); 1,88 | Timbro della Trama in ADR-0003 D-M2-06; gain staging; golden PCM della fixture | [Risultati](../research/M2_5_2_FM_Ratio_Results.md); la distinguibilità timbrica da Corpo va all'ascolto cumulativo |
| **D-M2-34** | **F2: voce della Trama centrata a potenza costante** (prima L 1,0 / R 0,5 sullo slot 4, l'unico usato). Delay 11/17 ms invariato | Primo fronte da +6,02 a 0,00 dB di ILD; due segnalazioni dell'esperto («a sinistra, fastidiosa in cuffia») | Ritardi o wet del delay (altra variabile, F2b); alternanza del pan per nota (scelta musicale) | Pan della Trama; golden PCM della fixture | [Risultati](../research/M2_5_2_F2_Trama_Stereo_Results.md); resta circa −1,5 dB di ILD sull'intera nota per il delay asimmetrico (F2b, da pre-registrare) |
| **D-M2-32** | **Struttura e sviluppo anticipati in una fase unica** ([proposta](../M2_5_D-M2-32_Proposal.md), approvata dal titolare il 2026-11-17 nella variante «prima si chiude M2.5.2»). M2.5.3 e M2.5.4 diventano la fase M2.5.3–4, non più condizionale a CP-3. Parte dopo la chiusura di M2.5.2: restano F4b (durata del kick, oggi decisa dal NoteOff del planner) e il livello della Trama, entrambi verificati in autonomia. Perimetro: F3, F1, contratto minimo di R-M25-7 (opzione B), un cambiamento per pacchetto. §16.3–§16.4 resta a M6 | Lo stesso brano senza varietà da M2.4 (`NULL_DIVERSITY`, Teorema D); gli ascolti umani su materiale invariato sono esclusi da D-M2-30; il planner si tocca una volta sola; la sessione alla cieca di M2.5.5 richiede materiale diverso | Piano precedente (M2.5.3, CP-3, M2.5.4 condizionale); fase subito, prima di chiudere il mix; §16.3–§16.4 completo ora (escluso da D-M2-19) | §3 (sotto-fasi), §4 (CP-3), §5 (P-M25-06, F5); specifica §32 invariata (non elenca le sotto-fasi); D-M2-03 invariato | Per pacchetto: predizioni registrate prima e golden aggiornati con il motivo; per lo sviluppo, le prove simboliche di R-M25-7; ascolto cumulativo a fine fase |
| **D-M2-35** | **F4b: Pulso one-shot.** Il kick segue il proprio inviluppo progettato (decadimento di 71–82 ms con i `timbreParam` del planner) e il NoteOff non lo interrompe più. Il NoteOff resta `Applied` nella catena degli eventi, e la scadenza di durata non lo tronca | Il registro delle dissolvenze mostrava i 416 kick troncati dal NoteOff a 53,6 e 62,5 ms con 2 ms a −26 dBFS; durata mediana da 64,2 a 76,1 ms; giudizio dell'esperto «durata oltre i 54–62 ms» | Allungare le note nel planner (fuori da M2.5.2: piano e trace invariati); allungare il decadimento (altra variabile, F4c) | Comportamento della voce di Pulso; golden PCM della fixture | [Risultati](../research/M2_5_2_F4b_Kick_OneShot_Results.md); B5 mancata (+0,05 LU): se servono più durata o più basse, F4c con una sua pre-registrazione |
| **D-M2-36** | **Livello della Trama: resta −9 dB; la scelta va all'ascolto cumulativo.** È l'esito della regola pre-registrata, non una preferenza: nessun offset fra 0 e +8 dB soddisfa presenza, subordinazione e sicurezza in tutte le sezioni. In B la presenza richiede +6 dB, che porta il mix a +0,54 LU, oltre il limite di +0,5 | Criterio da DESIGN §8.3, «presente ma subordinata», fissato prima di misurare | Adottare +4 o +6 dB (viola la regola); cambiare il limite dopo l'esito (adattamento escluso) | Nessuno (produzione invariata) | [Risultati](../research/M2_5_2_Trama_Level_Results.md); scala −9 / −5 / −3 dB, già misurata, per l'ascolto cumulativo di M2.5.3–4 |
| **D-M2-37** | **F4c: decadimento di Pulso ×1,5**, cioè 75 + 105 · `timbreParam` ms (107–122 ms con i valori del planner). Scelto dal criterio pre-registrato: nessuna sovrapposizione fra kick, Corpo non coperto (+0,02 al massimo), mix entro +0,5 LU e limiter fermo | Durata mediana del kick da 76 a 116 ms, Pulso +1,47 LU, quota sotto 100 Hz da 0,74 a 0,82; suggerimento dell'esperto (§8.1) | k = 1,25 (meno peso); k ≥ 1,75 (sovrapposizione e mix oltre +0,5 LU); decadimento esponenziale (altra variabile) | ADR-0004 §2.1, intervallo di decadimento da 50–120 a 75–180 ms; golden PCM della fixture | [Risultati](../research/M2_5_2_F4c_Kick_Decay_Results.md); giudizio finale all'ascolto cumulativo |
| **D-M2-38** | **Ingresso di M2.5.3–4** ([documento](../M2_5_3_4_Entry.md), E1–E3 approvati dal titolare il 2026-11-17). E1: pacchetto di misura 6 sullo sviluppo, validato sui controlli F1–F9 e sulle fixture avverse, prima della variazione derivata. E2: versione esplicita della policy del planner al primo cambio, senza conservare il percorso v1 eseguibile. E3: ordine dei pacchetti, cioè scenario tecnico del metro (D-M2-21), misura 6, F3, F1, poi il contratto minimo 1→6 | Senza una misura validata lo sviluppo si giudicherebbe solo all'ascolto (M4 `ORDER_FREE` sui controlli); il piano deve restare riproducibile da seed, policy e trace | Misura 6 omessa; F3 prima del metro | Policy del planner (versione nel manifest); ordine di §3 per M2.5.3–4 | Pre-registrazione per ogni pacchetto; CP-3 a fine fase |
| **D-M2-39** | **F3: policy del planner 1.0.2.0 e continuità dei pattern ai confini.** Nella prima frase di ogni sezione dopo la prima, Pulso e Trama mantengono i parametri euclidei della frase precedente e cambiano dalla seconda. La versione è propria del planner (`kPlannerPolicyVersion`); il default globale, usato da RunRecord e SeedBank, resta 1.0.1.0 | Giudizio dell'esperto: A→B «molto brusca», B→A′ «più fluida» dove i pattern proseguono; indicazione della proposta d'ingresso | Rampa di tempo subito (richiede un ADR per `MusicClock`); cambiare il default globale della policy | Planner (stato `lastPatterns_` nel checkpoint); golden di piano, trace e PCM della fixture, della forma di prova A→B e di `meter_change_v1` | [Risultati](../research/M2_5_3_F3_Boundary_Continuity_Results.md); valore musicale all'ascolto cumulativo |
| **D-M2-40** | **F1: àncora metrica dichiarata.** Ogni frase dichiara `Ancorato`, `Spostato` o `Ritorno` e lo spostamento di Pulso in passi (R-MET-1). La rotazione di Pulso è lo spostamento solo in `Spostato`, altrimenti 0; una dichiarazione incoerente è una violazione strutturale. Profilo iniziale della fixture `fixture-metric-v1`: un ottavo nelle frasi 12–13 di B, ritorno alla 14; default ancorato per ogni altra forma. Stato `accepted for implementation`, valori non validati | CP-2: F1 «stato metrico da dichiarare, non difetto da eliminare»; il kick evitava il tempo 1 in 19 frasi su 24 per l'indice del candidato | Togliere ogni spostamento; lasciare lo spostamento in B e A′; R-MET-3 e R-MET-4 subito | `PhrasePlan` (campi `metricState`, `pulsoDisplacementSteps`), planner; Trama complementare segue; golden di piano, trace e PCM della fixture e della forma di prova; gruppi simbolici 8 → 9 | [Risultati](../research/M2_5_3_F1_Metric_Anchor_Results.md); voce 7 del registro all'ascolto cumulativo |
| **D-M2-41** | **Contratto minimo, punto 1: diade di Corpo riparata dopo la scelta (C1c).** I candidati si scelgono senza riparazione. Nel solo vincitore, se la voce alta di Corpo (terza o quinta) forma classe d'intervallo 1 o 6 con un'altezza di Vettore che attacca nella battuta e l'altra nota dell'accordo no, le due si scambiano; il candidato riparato ripassa i vincoli duri, altrimenti resta quello non riparato. Il trace registra le riparazioni (hash canonico del piano, formato 2). Scelta del titolare il 2026-11-17 | R-HAR-5 (prima la diade); tritono Si–Fa di 8,0 s in 16 battute di A e A′. C1 (solo la terza) sposta il conflitto su La minore; C1b (nella selezione) cambia la melodia in 12 frasi per un margine dello 0,4% | C1, esito negativo; C1b, che cambia la melodia per caso; riparare anche il grado di Vettore (punto 2) | Planner (`generateCandidates`, `planPhrase`), `PhraseDecisionTrace`, hash canonico; golden di piano, trace e PCM della fixture, della forma di prova e di `meter_change_v1` | [Risultati](../research/M2_5_3_C1_Corpo_Dyad_Results.md); voce 9 del registro all'ascolto cumulativo |
