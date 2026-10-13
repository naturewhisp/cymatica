# CYMATICA M2.4 — Revisione critica della Deep Research

- **Data revisione:** 2026-10-13
- **Baseline del codice/documenti:** [`460bc286378b473218bc07f751e3615964626a11`](https://github.com/naturewhisp/cymatica/commit/460bc286378b473218bc07f751e3615964626a11)
- **Origine:** `deep-research-report.md` fornito dal titolare; copia consolidata in [M2_4_Deep_Research_Reviewed.md](M2_4_Deep_Research_Reviewed.md)
- **Stato:** review **documentale approvata con precisazioni**, non approvazione di risultati sonori non ancora prodotti.
- **Autorità:** la ricerca e questa review sono **non normative**; la [specifica](../../CYMATICA_Specifica_Agentica_Sviluppo.md), [DESIGN](../../DESIGN.md) e [ADR-0003](../adr/0003-m2-decisions-contracts.md) restano i contratti vigenti.

## 1. Valutazione

La ricerca è tecnicamente ben orientata e produce una raccomandazione utile: **composizione simbolica gerarchica, memoria dei motivi, armonia funzionale/modale e candidate scoring bounded**, seguiti da sintesi **role-specific** espressiva. Non ha trovato una giustificazione scientifica sufficiente per imporre modelli neurali, factor oracle o solver generali in M2.4. Le opportunità più originali — grammatica audiovisiva condivisa, sorpresa coordinata, memoria cross-modale — sono formulate correttamente come *ipotesi da falsificare*, non come innovazioni già validate.

Il contributo è **una sintesi ragionata di letteratura e proposte progettuali**: i punteggi 5/5 e i costi relativi nel rapporto non provengono da benchmark comparativi; l'eccellenza artistica non è una proprietà certificabile dalla bibliografia. Il test decisivo deve combinare prototipi ascoltabili, metriche tecniche e giudizi umani.

## 2. Correzioni editoriali e metodologiche applicate

| ID | Criticità nel rapporto originale | Correzione nella versione per repository | Motivazione |
|---|---|---|---|
| REV-01 | Riferimenti `cite` / `filecite` legati alla sessione Deep Research | Sostituiti con DOI, pagine di editori/istituti e link relativi alla documentazione CYMATICA | Fonte rintracciabile anche fuori dalla chat |
| REV-02 | Dichiarazione che le fonti NotebookLM non erano disponibili alla ricerca originale | Conservata la provenienza originale; esaminato un **sottoinsieme** dei materiali caricati come contesto e separato dalle evidenze accademiche | Non attribuire una review dell'intero corpus non realmente svolta |
| REV-03 | Score comparativi delle architetture in scala 5/5 | Etichettati come stime editoriali non calibrate | Non convertire opinioni tecniche in evidenza sperimentale |
| REV-04 | Possibile lettura della selezione gerarchica/wavetable/FM come nuova prescrizione normativa | Indicata natura non normativa e creati registri di decisione/esperimenti | M2.4 deve rispettare decisioni ADR vigenti |
| REV-05 | Rappresentazione dei motivi solo interna al planner sufficiente anche per il futuro mapping geometrico | Precisata necessità futura di **piano semantico condiviso o sidecar versionato** prima dell'integrazione M3/M6 | `MusicIntentEvent` oggi non espone `motifId`/`phraseId`/`transformId` |
| REV-06 | Rischio che `NoteOff` di nota precedente interrompa una voce riassegnata | Richiamata l'identità `(role,eventId)` più unicità/generazione slot e gestione pause/stop/overflow | Aderenza al commit propedeutico e protezione del lifecycle polifonico |
| REV-07 | Rischio di confondere benchmark DSP 2 ms e assenza garantita di underrun | Distinti tempo misurato, deadline del device, configurazione di prova e soak real-hardware | Le prestazioni dipendono dal device, dal quantum effettivo e dal scheduling OS |
| REV-08 | Riferimenti scientifici recenti privi di citazione bibliografica portabile (es. Calliphony, adaptive BGM, preferenze AI) | Verificati metadati editoriali selezionati, aggiunti link permanenti e scope dei risultati | Evitare trasferimenti di evidenza non giustificati |

**Limite della verifica bibliografica:** sono stati controllati DOI/metadati e abstract di un insieme rappresentativo di fonti (fra cui Conklin, Rohrmeier, Anders, Assayag, Pearce, Frühauf, Lerch, Tablas de Paula e NIME Calliphony). Non è stata compiuta una lettura integrale indipendente di ogni paper, né replicati gli studi. I riferimenti restanti mantengono una funzione di orientamento e richiedono verifica puntuale prima di diventare fondamento di una nuova decisione normativa.

## 3. Collocazione delle precedenti fonti NotebookLM

Il corpus caricato nel progetto contiene doppioni, testi divulgativi e fonti specialistiche di natura diversa. I seguenti elementi sono stati consultati **a campione** durante la contestualizzazione; le loro conclusioni non sostituiscono pubblicazioni primarie:

| Fonte già presente nel progetto | Tipologia | Uso appropriato |
|---|---|---|
| Nicolas Fournel, [*Procedural Audio for Video Games*](https://media.gdcvault.com/gdc10/slides/Fournel_Nicolas_ProceduralAudioForVideoGames.pdf) | Presentazione tecnica GDC | Scomporre problem definition, DSP design, preproduzione, QA e costi |
| [*Musica generativa: storia, tecniche, compositori*](https://www.musicainformatica.it/musica-generativa-storia-tecniche-compositori-e-i-libri-essenziali-per-capirla/) | Articolo storico/divulgativo | Orientamento su Eno, Reich e process music; risalire a testi originali |
| [*How jams become levels in Just Shapes & Beats*](https://www.gamedeveloper.com/design/how-jams-become-levels-in-the-co-op-bullet-hell-musical-i-just-shapes-beats-i-) | Intervista/postmortem | Vincoli di game feel audiovisivo; *non* dimostra capacità generative realtime |
| Khalifa et al., [*Talakat*](https://doi.org/10.1145/3205455.3205470) | Paper peer-reviewed PCG | Progettare gli esperimenti di generazione/fairness M4/M8 |
| [*Synesthesia in Video Games*](https://www.brothers-in-gaming.com/post/synesthesia-in-video-games) | Analisi di game design | Ispirazione su linguaggio multisensoriale; non sostituisce test utenti |
| Mattia Gravina, [tesi Politecnico di Torino](https://webthesis.biblio.polito.it/30953/1/tesi.pdf) | Tesi applicativa | Workflow di controllo audio/video e gestione dei parametri; non proof of concept di CYMATICA |
| Luca A. Ludovico, [*MIDI: una guida al protocollo*](https://air.unimi.it/retrieve/dfa8b9a8-57d2-748b-e053-3a05fe0a3a96/27-Monografia-507-1-10-20211012.pdf) | Monografia universitaria | Formati/scambio ed eventi MIDI quando necessari, non imposizione del MIDI come IR runtime |

I testi generici su generatori AI commerciali, forum e recensioni non sono fonti sufficienti per selezionare algoritmi DSP o garantire qualità musicale. La duplicazione dei documenti caricati non costituisce corroborazione indipendente.

## 4. Verifica dei vincoli specifici del repository

**Cosa rimane invariato:** 48 kHz logici, fixed simulation a 120 Hz, determinismo di `RandomKey`, ordinamento degli eventi, ownership SPSC, `MusicIntentEvent` congelato, otto voci (2/ruolo), deadline e limiti di callback; CACP resta diagnostica e non un composer autonomo.

**Dove servono decisioni all'ingresso di M2.4:** grammatica minima di frase, identità e serializzabilità dei motivi interni, durata della nota e piano di release, scelta del dialetto timbrico, criteri di voice stealing e tuning del mix. In [M2_4_Implementation_Plan.md](M2_4_Implementation_Plan.md) sono esplicitate come proposte o misure da effettuare, non come decisioni accettate automaticamente.

**Soglia realtime di ADR-0003:** per 512 frame **device** a 48 kHz il quantum audio è `512/48000 ≈ 10,667 ms` e il target di 2 ms ne rappresenta il 18,75%. Un device a 44,1 kHz con 512 frame ha quantum circa 11,61 ms; altri buffer o periodi di callback hanno deadline differenti. Il target di 2 ms è un *obiettivo ingegneristico adottato nell'addendum al precedente ADR*, non una prova di assenza universale di underrun. Profilazione DSP OFF/ON, hardware reale, jitter, tail e stress sono necessari. **Nessuna soglia può essere dichiarata verificata sulla sola base del test di flooding sotto 50 ms**.

**Identity/epoch:** il commit `460bc28637` documenta `(role, eventId)` e sustain cross-epoch. L'implementazione dovrà anche decidere come prevenire riuso di ID lungo la run, note orfane per drop e un `NoteOff` arrivato dopo voice stealing; l'eventuale chiusura di una voce non può dipendere dal solo `transportEpoch`.

## 5. Conseguenza per le milestone

Non serve rinumerare o ampliare i criteri obbligatori M2. La ricerca suggerisce un **checkpoint artistico esplorativo** in M2.4/M2.5 e verifiche più forti di riconoscibilità/transfer musicale-spaziale in M3–M7; queste ultime devono essere accettate esplicitamente nella relativa milestone prima di diventare gate. Il documento [roadmap_music_research_addendum.md](../roadmap_music_research_addendum.md) contiene l'analisi puntuale delle dipendenze, senza falsificare lo stato di avanzamento.

## 6. Verdetto e modalità d'uso

**OK per integrare il rapporto come ricerca non normativa e come input per M2.4.** Non vi è giustificazione per riaprire M2.1–M2.3 o introdurre ML in M2.4. I risultati artistici e di performance rimangono da ottenere. La review è documentale: non sono stati eseguiti qui nuovi build Windows, test device, benchmark DSP o sessioni di ascolto comparative.
