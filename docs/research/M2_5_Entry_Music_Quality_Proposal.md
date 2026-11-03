# Proposta per l'ingresso di M2.5: qualità musicale e regola di chiusura

- **Data:** 2026-11-03
- **Origine:** considerazioni consegnate dal titolare il 2026-11-03, di provenienza esterna, scritte sul repository a `b8b811f`. Il testo originale è nell'[appendice A](#appendice-a--testo-originale), con il testo invariato.
- **Verifica:** ogni citazione è stata confrontata con la specifica, l'addendum di roadmap musicale e i documenti di M2.4. Le correzioni sono nel §6.
- **Status:** **proposed**, non normativo. Non modifica decisioni, contratti, criteri di accettazione né golden. Le decisioni dei §7–§8 si prendono all'ingresso di M2.5 (AGENTS §1.1), dopo la chiusura di M2.4; fino ad allora nessuna si applica.
- **Aggiornamento:** il §8 riesamina le priorità dopo l'[ascolto dell'esperto del 2026-11-03](../m2_4_human_listening_2026-11-03.md). I §1–§7 restano come scritti.

## 1. Sintesi

M2.5 chiude la prima versione integrata e verificata del motore musicale, la «Music Engine v1». Non chiude la sua evoluzione compositiva, timbrica e artistica. La proposta:

- distingue tre livelli di qualità: uno obbligatorio, uno raccomandato e non vincolante, uno di ambizione (§3);
- ordina gli interventi F1–F6 privilegiando modifiche piccole, misurabili e reversibili (§4);
- propone una regola di chiusura di M2.5 che separa i gate tecnici dal giudizio artistico (§5);
- colloca la diversità musicale (F5) e lo sviluppo del materiale dopo M2.5, rispettando una scadenza obbligatoria già presente in M7 (§2, §7).

Il criterio di riferimento resta quello del titolare: la musica deve «ricordi, sviluppi, sorprenda e ritorni», senza varietà fittizia ([principi del titolare per F1–F5](../progress.md)).

## 2. Il motore musicale dopo M2.5

La specifica (§16, riga 1570) chiede che il motore crei «continuità, memoria e forma». L'addendum di roadmap musicale ([riga 11](../roadmap_music_research_addendum.md)) precisa che l'eccellenza artistica «richiede iterazioni progressive e playtest umani durante M3–M7».

Ciò che è **obbligatorio** nelle milestone successive va distinto da ciò che è soltanto **indirizzo di ricerca**. La specifica raccoglie i secondi nel §16.13 e nella tabella del §32 «Checkpoint della ricerca musicale… (indirizzi, non nuovi gate)», riga 3401.

| Milestone | Obbligatorio (specifica §32) | Indirizzo di ricerca musicale (non vincolante) |
|---|---|---|
| M3 — Pattern catalog e generatore costruttivo | livelli da Music Intent **senza adattamento**; pattern deterministici; livello di 60–90 s giocabile | piano semantico condiviso o sidecar; mapping beat-only, per ruolo e strutturale (§16.13: proiezione musica-geometria, proposta M3–M8) |
| M4 — Fairness validator | validazione spazio-temporale | validare come ogni candidato le trasformazioni geometriche derivate dalla musica |
| M5 — Player loop completo | ciclo di gioco completo | leggibilità dei segnali musicali con utenti reali |
| M6 — CIE Director v1 | gestione dell'esperienza del giocatore: `PlayerModel`, pacing, recovery debt, scoring, **novelty memory**, **stochastic top-k selector** (righe 3325-3326) | adattamento della frase che preservi motivi e cadenze; budget di sorpresa sperimentale |
| M7 — Archetipi e vertical slice | tra l'altro: «run diverse mostrano diversità misurabile» (riga 3366); «vertical slice ritenuta divertente da playtest umano» (riga 3369) | valutazione umana di riconoscibilità, coerenza e affaticamento; nuovi criteri solo con procedure e soglie decise prima |
| M8 — AI Lab e quality-diversity | quality-diversity dei **pattern** nel Lab | descrittori e versioni di motivi, tensione e trasformazioni |
| M9–M13 | pacchetti e timeline (M9), custom audio mix-only (M10), NatuStem (M11), Android (M12), Studio/editor (M13) | piano semantico, licenze, budget DSP Android, authoring |

**Conseguenza principale.** Il CIE Director di M6 non è un direttore musicale: oggi nessun deliverable obbligatorio prevede lo stato musicale sopra la frase (F5). Il primo gate obbligatorio che il generatore attuale non soddisferebbe sul lato musicale è in M7: «run diverse mostrano diversità misurabile». La diversità fra 64 seed è infatti `NULL_DIVERSITY` (Teorema D: il seed agisce solo sui pareggi). Per questo F5 richiede una decisione esplicita di perimetro (§7, P-M25-06).

Gli strumenti di M6 (novelty memory, selettore top-k) coincidono con quelli indicati dalle ricerche per F5: varietà guidata dallo stato e selezione deterministica fra candidati quasi ottimi ([verifica della review di ADR-0006](M2_4_Inc5_ADR0006_Review_Verification.md), AG-ADR-09).

## 3. Livelli di qualità

| Livello | Contenuto | Natura |
|---|---|---|
| **1 — Correttezza tecnica** | Criteri di M2.5 della specifica (riga 3228): tre minuti di musica continua senza crackle; due sezioni distinguibili con transizione tempo/metrica; sincronismo fra intent e audio; trace deterministico; callback che esegue soltanto; build e review secondo AGENTS. Inoltre AC-AO-02 con le soglie ratificate e la mappatura di AC-AO-01A/01B. Evidenze sulla build finale, comprese la qualificazione realtime su device (dopo DIF-INC4-R3-05) e le verifiche percettive pertinenti | **gate normativo** |
| **2 — Credibilità musicale** | L'ascoltatore riconosce un ritmo, ruoli timbrici diversi, la forma A–B–A′ e almeno un motivo che ritorna trasformato. Il mix non affatica, le transizioni sono comprensibili anche se semplici. Nessun difetto evidente si nasconde dietro un PASS | **obiettivo raccomandato, non vincolante**, da perseguire con ascolto e interventi mirati |
| **3 — Eccellenza artistica** | Linguaggio originale, sviluppo motivico espressivo, sorpresa intenzionale, transizioni raffinate, varietà significativa fra run, timbri distintivi, reattività al giocatore senza perdere identità | **ambizione progressiva**, banco di prova M6–M8 e vertical slice M7 |

Il livello 2 diventa un criterio obbligatorio solo con una decisione esplicita, e soltanto se la procedura di giudizio è fissata prima dell'ascolto (P-M25-03).

## 4. Interventi F1–F6

Numerazione e cause dal [dossier delle evidenze](../m2_procedural_evidence.md) §4.2 (righe 107-112). L'ascolto del 2026-10-28 ([registro](../m2_4_human_listening_2026-10-28.md), riga 27) ha dato coerenza 6, interesse 4, ripetitività 7 su 7: è il giudizio esplorativo di un solo esperto e non è statisticamente rappresentativo.

| Priorità | Questione | Proposta |
|---|---|---|
| Alta | F2 — Trama lateralizzata a sinistra (precedenza, delay 11/17 ms) | Correggere il bilanciamento stereo; ripetere l'ascolto in cuffia |
| Alta | F1 — Pulso senza appoggio metrico | Sincope o spostamento **intenzionali**, non conseguenza dell'indice del candidato; mantenere la possibilità di ritmi spostati ([§3bis dell'ascolto](../m2_4_human_listening_2026-10-28.md)) |
| Alta | F3 — Ripetitività e transizioni brusche; F4 — Mix e timbro | Poche correzioni controllate, confrontando sempre con il WAV di riferimento |
| Media | F6 — Rilascio di 2 ms | Scegliere con l'ascolto A/B (H-03), senza cambiare automaticamente il default |
| Strategica | F5 — «Direttore musicale», stato sopra la frase | Piano evolutivo, principalmente per M6, senza riscrivere ora il planner (P-M25-06) |

**Vincoli:**

- Le modifiche a F1, F2, F4 e F6 cambiano il golden PCM, e forse il piano e la trace. Ogni golden si aggiorna con il motivo registrato, come per gli altri golden (`engine/audio/audio_probe.h`, commento a `kFixtureProbeV1GoldenPcmHash`).
- Servono un ADR d'ingresso di M2.5 con le evidenze e un confronto A/B con la versione precedente.
- M2.5 non diventa una nuova grande fase di sviluppo compositivo: si sfrutta prima l'architettura esistente.

## 5. Regola di chiusura proposta

| Situazione dopo ascolto e collaudi | Decisione |
|---|---|
| Un gate tecnico obbligatorio fallito, non eseguito o bloccato | M2.5 **NON OK** |
| Audio tecnicamente corretto, ma sezioni non distinguibili o crackle udibili | M2.5 **NON OK**: sono requisiti espliciti della specifica |
| Audio corretto e struttura distinguibile, ma ripetitività alta o timbri migliorabili | M2.5 chiudibile sul piano tecnico, con i limiti artistici registrati e gli interventi pianificati |
| Audio corretto, struttura convincente, mix equilibrato, ascolto coinvolgente | M2.5 **OK**: baseline v1 adatta alle evoluzioni successive |

**Condizioni:**

- Per la sessione alla cieca valgono le regole già in vigore: gruppi musicali indipendenti; con dati insufficienti si dichiara `INSUFFICIENT_DATA`, senza trasformare un test eseguito in una validazione statistica.
- Un PASS tecnico non certifica la qualità artistica, e un singolo ascolto negativo non crea da solo un nuovo requisito.
- Le righe 2–4 richiedono un giudizio percettivo: la procedura va fissata prima (P-M25-03).

## 6. Correzioni rispetto al testo originale

1. **Tabella delle milestone.** Il testo presenta come «risultato atteso» alcuni indirizzi di ricerca: legame musica-geometria in M3, sviluppo musicale in M6, riduzione delle ripetizioni musicali in M8. La specifica li definisce indirizzi, non gate (riga 3401). Il §2 li separa dagli obblighi.
2. **Scadenza di F5.** Il testo non indica il gate di diversità di M7 né il fatto che il CIE Director di M6 non abbia compiti musicali. ADR-0006 rinvia inoltre ablazione e F5 al gate di M2.5 (5e-07, [riga 574](../adr/0006-m2-4-inc5-entry-decisions.md)): spostarle a M6 modifica questo rinvio.
3. **AC-AO-02 «superato».** Le soglie sono proposte (H-01, H-02, dossier delle evidenze righe 97-98) e vanno ratificate prima della misura.
4. **Transizione tempo/metrica.** La fixture non cambia metro (tutto 4/4). Il dossier delle evidenze (riga 79) segna il criterio come **parziale**: va deciso se basta il cambio di tempo.
5. **Numerazione F.** «F3/F4: transizioni e mix» non corrisponde ai documenti: F3 è ripetitività e transizioni, F4 mix e timbro, F5 il direttore musicale.

## 7. Decisioni proposte per l'ingresso di M2.5

| ID | Decisione da prendere | Proposta |
|---|---|---|
| P-M25-01 | Interpretazione di «transizione tempo/metrica» | Decidere, con motivazione, se basta il cambio di tempo o se la fixture deve includere anche un cambio di metro |
| P-M25-02 | Soglie AC-AO-02 | Ratificare H-01/H-02, o rivederle con evidenze, prima di qualunque misura sulla build finale |
| P-M25-03 | Procedura percettiva | Fissare prima dell'ascolto il protocollo per «sezioni distinguibili» e per il livello 2: stesse scale 1–7 del 2026-10-28, confronto A/B con il WAV di riferimento, descrittori CACP per sezione; dichiarare il livello 2 obiettivo non vincolante |
| P-M25-04 | Regola di chiusura | Adottare la tabella del §5 |
| P-M25-05 | Perimetro degli interventi F1–F4 e F6 | Modifiche piccole, misurabili e reversibili, ciascuna con A/B, evidenze e golden aggiornati con il motivo, in un ADR d'ingresso |
| P-M25-06 | Collocazione di F5 e dell'ablazione (5e-07) | Spostarle esplicitamente a M6, ampliando il perimetro del CIE Director allo stato musicale sopra la frase, oppure fissare un'altra milestone. Vincolo: arrivare al gate di M7 «run diverse mostrano diversità misurabile» con diversità musicale autentica, non fittizia |
| P-M25-07 | Declinazione del criterio artistico | M2.5: qualcosa da ricordare e una forma da riconoscere. M6: sviluppare il materiale e sorprendere intenzionalmente. M7: esperienza audiovisiva coinvolgente e giocabile. M8 e oltre: varietà e qualità ripetibili, non legate a un seed curato |

## 8. Aggiornamento dopo l'ascolto del 2026-11-03

L'[ascolto dell'esperto](../m2_4_human_listening_2026-11-03.md) riguarda lo stesso PCM del 2026-10-28: non valuta interventi, ma precisa dove intervenire. Le priorità del §4 diventano:

| Priorità | Questione | Motivo | Prima prova proposta |
|---|---|---|---|
| Alta | F4 — Livello del kick | «TROOOPPO basso» con forte enfasi; misurato 7–9 dB sotto Corpo e Vettore, con circa 9 dB di margine al ceiling | Alzare Pulso e confrontare con il WAV di riferimento |
| Alta | F2 — Trama lateralizzata | Seconda segnalazione: «ancora fastidiosa», con il ruolo già il più basso del mix | Correggere l'immagine stereo, non il livello |
| Alta | F3 — Transizione A→B | «Molto brusca», mentre B→A′, dove i pattern proseguono, è «più fluida» | Non cambiare i pattern di Pulso e Trama sullo stesso confine del tempo, prima di valutare rampe di BPM |
| Da rivalutare | F1 — Kick e metro (era Alta) | Risposta «Sì» a Q3. Il metro può reggersi su Vettore e Corpo | Valutare insieme a F4: alzando il kick il controtempo diventerà più evidente |
| Bassa | F6 — Rilascio (era Media) | Nessuna differenza significativa fra 2 e 80 ms, nessun click | Decidere H-03 sulle misure |
| Strategica | F5 — Sviluppo | Dopo il secondo loop serve una «decisa variazione melodica» o un nuovo suono | Definire in P-M25-05 e P-M25-06 cosa si può ottenere in M2.5 con l'arrangiamento e cosa resta a M6 |

**Decisione aggiuntiva proposta:**

| ID | Decisione da prendere | Proposta |
|---|---|---|
| P-M25-08 | Leggibilità delle viste per l'ascolto umano | Etichette con strumenti riconoscibili e griglia di quarti, ottavi o sedicesimi nelle viste a pentagramma, come strumento offline senza effetti sul percorso realtime |

**Precisazioni a P-M25-03:** il prossimo pacchetto deve dire cosa è cambiato rispetto all'ascolto precedente, riusare le scale 1–7 del 2026-10-28 e chiedere Q3 per sezione.

**Per la regola di chiusura (§5):** sul materiale attuale l'esperto giudica le sezioni distinguibili e non sente click. Il caso più vicino è quindi la terza riga, «chiudibile sul piano tecnico con i limiti artistici registrati». L'indicazione vale solo per questo materiale: gli interventi di M2.5 richiedono un nuovo ascolto sulla build finale.

## Appendice A — Testo originale

Testo e link invariati. Solo due adattamenti di formato: i titoli numerati sono scesi di un livello, e i tre punteggi dell'ascolto, che nell'originale erano titoli di primo livello, sono resi come testo normale.

---

Il motore musicale di CYMATICA non si conclude con M2.5. M2.5 conclude la prima versione integrata e verificata del motore, non la sua evoluzione compositiva, timbrica o artistica.

La roadmap distingue due obiettivi:

- M2.5: dimostrare che CYMATICA sa generare e riprodurre autonomamente un brano musicale strutturato, coerente, sincronizzato e tecnicamente affidabile.
- M3–M13: trasformare quel sistema in un'esperienza musicale interattiva, capace di evolvere, reagire al giocatore, costruire una propria identità artistica e produrre contenuti diversi.

La specifica lo chiarisce già nel §16: «Il motore musicale deve creare continuità, memoria e forma. Randomizzare note valide in una scala non è sufficiente».

La roadmap di ricerca, inoltre, afferma esplicitamente che l'eccellenza artistica richiede iterazioni progressive e playtest umani durante M3–M7.

La qualità che pretenderei per M2.5 è quella di un prototipo musicale credibile e tecnicamente maturo, non ancora di un'opera compiuta. Questo non significa accettare qualunque sequenza di suoni purché superi i test: deve già esistere una composizione riconoscibile, sufficientemente curata da costituire una base solida per le evoluzioni successive.

### 1. Come evolverà il motore dopo M2.5

Le milestone successive non si limitano ad aggiungere gameplay attorno a un sintetizzatore ormai congelato.

| Milestone | Evoluzione musicale                                                                          | Risultato atteso                                              |
| --------- | -------------------------------------------------------------------------------------------- | ------------------------------------------------------------- |
| M3        | Collegamento strutturale fra motivi, frasi e geometria; mapping musicale dei pattern         | La musica inizia a determinare anche la forma del livello     |
| M4–M5     | Integrazione di segnali ritmici, timbrici e feedback con gameplay, sicurezza e accessibilità | Audio e interazione diventano un sistema coerente             |
| M6        | Direttore adattivo, memoria, gestione di tensione, recupero, novità e risposte al giocatore  | La composizione evolve in funzione della partita              |
| M7        | Rifinitura audiovisiva, identità degli archetipi, sessioni prolungate e playtest umani       | Prima vertical slice con una riconoscibile identità artistica |
| M8        | Quality-diversity, librerie curate, ricerca di varianti e riduzione delle ripetizioni        | Maggiore varietà musicale e ludica verificabile               |
| M9–M13    | Esportazione di brani e livelli, musica importata, strumenti di authoring, porting Android   | Motore utilizzabile in più modalità e contesti                |

Queste evoluzioni sono documentate nella [roadmap §32](https://github.com/naturewhisp/cymatica/blob/b8b811f1ce/CYMATICA_Specifica_Agentica_Sviluppo.md?utm_source=chatgpt.com) e nel [roadmap musicale aggiuntivo](https://github.com/naturewhisp/cymatica/blob/b8b811f1ce/docs/roadmap_music_research_addendum.md?utm_source=chatgpt.com). Le proposte di ricerca non sono automaticamente requisiti obbligatori.

M6 è probabilmente il salto compositivo più importante, perché introduce la gestione dell'esperienza nel tempo: non soltanto scegliere una frase valida, ma decidere quando ripetere, modificare, sorprendere e recuperare un'idea musicale.

M7 è invece il momento naturale per pretendere che questa capacità produca un'esperienza artistica e ludica convincente.

### 2. Qualità da pretendere in M2.5

Distinguerei tre livelli.

Livello 1: correttezza tecnica, obbligatoria

Tre minuti di musica continua senza crackle; sezioni distinguibili e transizione tempo/metrica verificata; sincronismo fra intent e audio; replay deterministico; callback realtime bounded; AC-AO-01A e AC-AO-02 superati. Servono evidenze sulla build finale, comprese le verifiche hardware e quelle percettive pertinenti.

Livello 2: credibilità musicale, obiettivo raccomandato per M2.5

L'ascoltatore riconosce un ritmo, ruoli timbrici diversi, una struttura A–B–A′ e almeno un motivo che ritorna trasformato. Il mix è sufficientemente equilibrato da non rendere fastidioso l'ascolto; le transizioni sono comprensibili, anche se non ancora sofisticate. Nessun difetto evidente viene nascosto dietro un PASS automatico.

Livello 3: eccellenza artistica, obiettivo progressivo M6–M8

Un linguaggio musicale originale, sviluppo motivico espressivo, sorpresa intenzionale, transizioni raffinate, varietà significativa fra run, timbri distintivi e capacità di reagire al giocatore senza perdere identità.

Il livello 1 è il gate normativo. Il livello 2 è quello che raccomando di perseguire concretamente in M2.5 attraverso ascolto e interventi mirati, senza introdurre di nascosto nuovi criteri obbligatori. Il livello 3 rappresenta l'ambizione del progetto.

### 3. La situazione attuale: dove intervenire davvero

L'[ascolto umano del 28 ottobre](https://github.com/naturewhisp/cymatica/blob/b8b811f1ce/docs/m2_4_human_listening_2026-10-28.md?utm_source=chatgpt.com) ha prodotto questi giudizi:

Coerenza

6/7

Interesse

4/7

Ripetitività

7/7

Valutazione esplorativa di un solo esperto, non statisticamente rappresentativa. Un valore alto di ripetitività è negativo.

Questi risultati sono indicativi: la musica ha già una struttura riconoscibile, ma la capacità di sostenere l'interesse rimane limitata.

Il dossier finale conferma inoltre che la diversità fra 64 seed è `NULL_DIVERSITY`. Non è un malfunzionamento del rendering: il planner assegna ai seed un ruolo che, nella fixture esaminata, non produce differenze musicali.

Perciò concentrerei M2.5 sui seguenti interventi, valutandoli prima con una decisione esplicita.

| Priorità   | Problema                                          | Raccomandazione                                                                                                                 |
| ---------- | ------------------------------------------------- | ------------------------------------------------------------------------------------------------------------------------------- |
| Alta       | F2: Trama stabilmente lateralizzata a sinistra    | Valutare correzione del bilanciamento stereo e rifare l'ascolto in cuffia                                                       |
| Alta       | F1: ancoraggio metrico del Pulso                  | Rendere la sincope intenzionale, non conseguenza casuale dell'indice del candidato; preservare la possibilità di ritmi spostati |
| Alta       | F3/F4: transizioni brusche e mix poco equilibrato | Sperimentare poche correzioni controllate, confrontando sempre il WAV precedente                                                |
| Media      | F6: release di 2 ms                               | Scegliere sulla base dell'ascolto A/B, senza cambiare automaticamente il default                                                |
| Strategica | F5: ripetitività e assenza di diversità fra seed  | Definire un piano evolutivo del direttore musicale, principalmente per M6, senza riscrivere ora il planner                      |

Non trasformerei M2.5 in una nuova grande fase di sviluppo compositivo. Proverei prima a ottenere il massimo dall'architettura esistente, privilegiando interventi piccoli, misurabili e reversibili.

### 4. Come decidere se M2.5 può essere chiusa

La regola operativa che raccomando è questa:

| Situazione dopo l'ascolto e i collaudi                                                         | Decisione                                                                              |
| ---------------------------------------------------------------------------------------------- | -------------------------------------------------------------------------------------- |
| Qualsiasi gate tecnico mandatory fallito, non eseguito o bloccato                              | M2.5 NON OK                                                                            |
| Audio tecnicamente corretto, ma sezioni non distinguibili oppure crackle udibili               | M2.5 NON OK, perché sono coinvolti requisiti espliciti                                 |
| Audio corretto e struttura distinguibile, ma elevata ripetitività o timbri ancora migliorabili | M2.5 tecnicamente chiudibile, con limiti artistici registrati e interventi pianificati |
| Audio corretto, struttura convincente, mix equilibrato e ascolto sufficientemente coinvolgente | M2.5 OK, baseline v1 adatta alle evoluzioni successive                                 |

La sessione blind di sensibilità e specificità, rinviata da Inc-5, deve utilizzare gruppi musicali sufficientemente indipendenti. Se la quantità di dati resta insufficiente, bisogna dichiarare `INSUFFICIENT_DATA`, non trasformare un test eseguito in una validazione statistica.

Allo stesso modo, un PASS tecnico non equivale a una certificazione di qualità artistica, e un singolo ascolto negativo non crea automaticamente un nuovo requisito normativo.

### 5. Dove fisserei l'ambizione artistica di CYMATICA

La frase guida già utilizzata nel progetto è particolarmente appropriata:

> Ricordi, sviluppi, sorprenda e ritorni.

La declinerei in questo modo:

- M2.5: la musica deve avere qualcosa da ricordare e una forma da riconoscere.
- M6: deve saper sviluppare quel materiale e sorprendere intenzionalmente.
- M7: deve costruire un'esperienza audiovisiva coinvolgente, riconoscibile e giocabile.
- M8 e oltre: deve poterlo fare ripetutamente, con varietà e qualità, non soltanto con uno specifico seed curato.

Questo evita due errori opposti: dichiarare concluso il motore musicale troppo presto oppure pretendere un capolavoro definitivo prima che esistano gli strumenti per svilupparlo.

La mia raccomandazione è di utilizzare M2.5 per certificare una solida «Music Engine v1», sufficientemente musicale da essere ascoltata e valutata seriamente, ma non necessariamente già straordinaria. L'obiettivo di un'opera d'arte digitale rimane pienamente valido; semplicemente, il suo vero banco di prova sarà la vertical slice M7, quando composizione, gameplay, reattività, grafica e percezione umana potranno essere valutati come un unico sistema.
