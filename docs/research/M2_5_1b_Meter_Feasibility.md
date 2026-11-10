# M2.5.1b — Fattibilità del cambio di metro

- **Stato:** evidenza per CP-1. Non è una decisione e non integra nulla: motore e fixture non cambiano. Review indipendente OK su `72a1c8e`, consegnata il 2026-11-10; la sua osservazione OSS-M2.5.1b-01 corregge la proposta finale, vedi [integrazione](#integrazione-dopo-la-review-2026-11-10).
- **Domanda (P-M25-01):** il criterio della specifica chiede «transizione di tempo/metrica via `SetTempoMap`». Basta il cambio di tempo, o la fixture deve cambiare anche metro?
- **Prova:** [`test_meter_feasibility.cpp`](../../tests/unit/test_meter_feasibility.cpp). Una forma di prova di 24 battute, A in 4/4, B in un altro metro, A' in 4/4, percorre orologio, planner e catena offline a 48 kHz.

## Risultati

**Orologio.** Battute e frasi restano esatte attraverso 4/4 → 3/4 → 4/4: battuta 8 al frame 768.000, battuta 16 al frame 1.344.000, battuta 24 al frame 2.112.000. La posizione rilegge indice di frase, battuta e movimento corretti.

**Planner con il motivo attuale di 16 step.** Prima frase della sezione centrale:

| Metro | Step per battuta | Candidati validi | Esito |
|---|---:|---:|---|
| 3/4 | 12 | 0 su 8 | Fallback L1, 12 eventi invece di circa 60 |
| 6/8 | 12 | 0 su 8 | Fallback L1, 12 eventi |
| 7/8 | 14 | 7 su 8 | Piano completo; scartato il candidato con variazione euclidea |
| 5/4 | 20 | 8 su 8 | Piano completo; Vettore tace nell'ultimo movimento |
| 12/8 | 24 | 8 su 8 | Piano completo; Vettore tace nell'ultimo terzo |

In 3/4 e 6/8 il motivo ha una nota sullo step 12, fuori battuta, e ogni candidato diventa `StructuralInvariantViolation`. **Nessun errore arriva al chiamante:** il piano si sfoltisce e basta.

**Con un motivo di 12 step.** Un motivo riscritto per la sezione centrale, passato con `SectionPlan::sectionMotif`, basta in 3/4 e 6/8: 8 candidati validi su 8, senza fallback, senza toccare il planner. In 3/4 ogni NoteOn cade sulla griglia di semicrome, Corpo cade su ogni battere e il piano è deterministico.

**Catena offline.** La forma 4/4 → 3/4 → 4/4 arriva fino al PCM: entrambi i `SetTempoMap` e lo Stop sono `Applied` al frame atteso, 0 eventi persi, scheduler vuoto a fine corsa, campioni finiti, energia in tutte le sezioni.

## Costo di una eventuale integrazione

| Punto | Dove | Lavoro |
|---|---|---|
| Motivi legati alla lunghezza della battuta | `makeDorianBaseMotif`, `makeAeolianBaseMotif` | Un motivo per metro, oppure una regola di adattamento |
| Frame dei pad incrociati con battute fisse | `music_planner.cpp:617-618`, `32 * 4 + 1`, `64 * 4 + 1` | Derivarli dall'orologio; `test_music_planner.cpp:820` e `:824` ripetono la formula |
| Battere calcolato con metro costante | `audio_probe.cpp:1586`, `(startBar + b) * beatsPerBar` | Sbaglia dopo un cambio di metro: nella forma di prova la battuta 16, in 4/4 dopo otto battute di 3/4, darebbe il movimento 64 invece di 56 |
| Silenzio del fallback in un caso di configurazione | `planPhrase` | Decidere se un motivo incompatibile debba essere un errore |
| Golden | PCM, piano, trace | Da rigenerare; M2.5.3 li aggiorna già |

## Limiti

- **Fatto tecnico, non musicale.** Il motore genera in 3/4, ma nulla dimostra che si senta. Pulso usa 4–6 colpi euclidei per battuta qualunque sia il metro e l'accento distingue solo lo step 0: non esiste una gerarchia metrica per metro. Con la forma di prova non c'è stato ascolto.
- **I metri composti non sono modellati.** Il BPM conta sempre le semiminime e il movimento è il denominatore: 6/8 è una battuta di sei crome, non due movimenti puntati. `subdivisionsPerBeat` va scelto per metro.
- **Il cambio di metro resta possibile solo sulla battuta.** Era già la regola di `setTempoMap`.
- La prova copre una forma corta e un solo seed. Non copre i pad incrociati, il device, né le mappe in cui cambia anche `barsPerPhrase` (CL-MM-01).

## Per CP-1

Il cambio di metro è tecnicamente alla portata: non servono modifiche a orologio, scheduler o motore. Il costo è nei motivi, in due formule della fixture e in una sonda.

**Proposta, da decidere a CP-1:** non inserirlo per chiudere una casella. La lettera «tempo/metrica» si presta a leggere il cambio di tempo come sufficiente, ma il dossier delle evidenze segna oggi il criterio come parziale: la scelta va motivata e registrata. *(Corretto dalla review, 2026-11-10: la lettura «basta il tempo» non è approvabile senza una riconciliazione normativa esplicita; vedi l'integrazione qui sotto.)* Un 3/4 privo di accenti metrici propri chiuderebbe la voce senza dare una differenza udibile, e l'ascolto dovrà dirlo. Se invece il titolare vuole un metro diverso come scelta musicale, il lavoro da pianificare a M2.5.3 include la gerarchia degli accenti, che oggi manca, e l'ascolto.

## Integrazione dopo la review (2026-11-10)

La review indipendente di M2.5.1b (verdetto OK, registrata in [progress.md](../progress.md)) osserva con OSS-M2.5.1b-01 che la proposta qui sopra trattava come una sola domanda tre questioni diverse. Verificato sulla specifica §32.

| Aspetto | Cosa chiede la specifica | Stato | Cosa resta a CP-1 |
|---|---|---|---|
| 1. Supporto tecnico del cambio di time signature | Deliverable di M2: canale `SetTempoMap` «con acknowledgement per cambi tempo e time signature»; verifica di M2.3: «cambi tempo/metrica» | **Dimostrato**: test di `MusicClock` in M2.3 (3/8, 6/8, 5/4, 2/2) e catena offline 4/4 → 3/4 → 4/4 di questa prova, con ack `Applied` ai frame attesi | Nulla. Non è materia di interpretazione: è un contratto di M2 |
| 2. Presenza del cambio nel contenuto di qualificazione | Accettazione di M2 e verifica di M2.5: due sezioni distinguibili «inclusa transizione di tempo/metrica via `SetTempoMap`» | **Parziale**: la fixture cambia tempo e resta in 4/4 | Decidere, con una delle opzioni sotto |
| 3. Riconoscibilità musicale del cambio | «Sezioni distinguibili» è un giudizio percettivo | **Non verificata**: nessun ascolto, nessuna gerarchia metrica per metro | Ascolto, solo se il cambio entra nel contenuto |

**Opzioni per l'aspetto 2.** Sono proposte; nessuna è adottata.

| Opzione | Contenuto | Costo | Rischio |
|---|---|---|---|
| A. Cambio di metro nella fixture | La sezione B, o una sua parte, in un metro diverso, con motivo adatto e accenti propri | Motivi per metro, gerarchia degli accenti, le due formule e la sonda del paragrafo sul costo, golden rigenerati, ascolto (aspetto 3). Eventuale ricerca R-M25-11 | Un metro senza accenti propri chiude la voce senza differenza udibile |
| B. Fixture in 4/4, con riconciliazione normativa | La specifica §32 viene emendata per dire che in M2 la transizione obbligatoria nel contenuto è quella di tempo, e che il cambio di metro è dimostrato a livello tecnico | Una modifica della specifica, registrata con motivo e decisione del titolare (AGENTS §5.1) | Senza l'emendamento è un indebolimento silenzioso del criterio: la review non lo approva |
| C. Scenario tecnico dedicato | La forma di prova 4/4 → 3/4 → 4/4 diventa uno scenario di qualificazione ripetibile, distinto dalla fixture musicale | Promuovere il test a scenario, con manifest | Rafforza l'aspetto 1, non l'aspetto 2: da sola non evita la riconciliazione dell'opzione B |

La scelta fra A e B è del titolare. La C è utile insieme a una delle due, non al loro posto.

**Precisazioni sul testo della review.**
- «Fallback silenzioso» per 3/4 e 6/8 significa senza errore per il chiamante, non senza suono: il planner ripiega sul livello L1 e produce 12 eventi invece di circa 60.
- L'hash canonico riproducibile è verificato sul piano in 3/4 con il motivo di 12 step.
- Il revisore dichiara di non avere eseguito la prova: il suo giudizio poggia su sorgenti, dossier e CI.

**Destinazione dei test di caratterizzazione (OSS-M2.5.1b-02).** I test di [`test_latent_faults.cpp`](../../tests/unit/test_latent_faults.cpp) fissano il comportamento attuale, guasti compresi. Ogni coppia ha un test che riproduce il guasto e uno che mostra che la fixture non lo attiva: alla correzione il primo va sostituito con il test del comportamento voluto, il secondo resta.

| Guasto | Se CP-1 include il cambio di metro | Se CP-1 lo esclude |
|---|---|---|
| CL-MM-01, `barsPerPhrase` | M2.5.3, prima di toccare le mappe | M2.5.6, oppure rinvio motivato a M3 |
| AG-MM-02, `phrasePhase` | M2.5.3 | M2.5.6, oppure rinvio motivato |
| CL-MM-02, trasposizione al bordo | M2.5.3 se F1 o F3 cambiano i motivi; altrimenti M2.5.4 | Quando un intervento tocca i motivi; al più tardi M2.5.6 |
| CL-MM-04, pad incrociati | M2.5.3: le formule dei pad vanno comunque derivate dall'orologio | M2.5.3 se F3 tocca i confini di sezione; altrimenti M2.5.6 |

Le destinazioni sono proposte coerenti con ADR-0007 §8 (R-06) e si confermano a CP-1.
