# CYMATICA

Musical bullet hell procedurale: **musica, geometria e minacce derivano da un intento musicale condiviso**, con generazione vincolata, memoria, forma e adattamento graduale. Obiettivo artistico: un'opera audiovisiva con identità compositiva riconoscibile, non un generatore infinito di note casuali.

## Stato reale

Prototipo tecnico Windows sviluppato in C++20 con CMake/Ninja, raylib e miniaudio. M0 e M1 concluse; **M2 attiva**: secondo il [registro milestone](docs/progress.md), M2.1 (decisioni), M2.2 (input/AudioProbe) e M2.3 (transport/scheduler) sono concluse; **M2.4 (musica procedurale v1) è la prossima sottofase**, poi M2.5 (integrazione e accettazione complessiva). La conclusione di M2.3 non certifica la musica polifonica futura né chiude M2. Non è ancora una release giocabile.

La [CI GitHub Actions Windows](.github/workflows/windows-ci.yml) verifica automaticamente build/test Headless Release e Debug e il profilo diagnostico CACP offline. Le prove hardware, l'ascolto percettivo, la validazione artistica e il gate indipendente di milestone richiedono evidenze separate; nessun PASS di CI li sostituisce.

**Baseline documentale di questa integrazione ricerca:** commit [`460bc286378b473218bc07f751e3615964626a11`](https://github.com/naturewhisp/cymatica/commit/460bc286378b473218bc07f751e3615964626a11), 13 ottobre 2026. La ricerca e gli esperimenti proposti **non sono ancora codice implementato o risultati musicali validati**.

## Documentazione

| Documento | Responsabilità |
|---|---|
| [Specifica tecnica](CYMATICA_Specifica_Agentica_Sviluppo.md) | Architettura, contratti, timing, fairness e roadmap normativa (§16, §32) |
| [Design](DESIGN.md) | Identità artistica, gameplay, linguaggio audiovisivo, UX e accessibilità (§8, §16) |
| [ADR-0003](docs/adr/0003-m2-decisions-contracts.md) | Contratti Music Intent, 8 voci 2/ruolo, scheduler e addendum M2.4 per note sustain/deadline |
| [Controllo e osservazione agentica](docs/agent_control_observability.md) | Motivazioni, limiti, CACP, sequenze/streaming e verifiche |
| [Regole degli agenti](AGENTS.md) | Istruzioni operative, controllo scope e milestone gate |
| [Stato e prove milestone](docs/progress.md) | Esecuzione realmente verificata, decisioni d'ingresso, evidenze e blocchi |
| [Indice ricerca musicale M2.4](docs/research/README.md) | Deep Research revisionata, audit delle fonti, piano di implementazione, test di qualità |
| [Roadmap artistica proposta M2.4–M13](docs/roadmap_music_research_addendum.md) | Possibili estensioni di ricerca alle future milestone, **non normative fino all'approvazione** |
| [Build e test](docs/build.md) | Compilazione, CTest, eseguibile e istruzioni locali |

## Direzione tecnica e musicale

Target iniziale Windows x64; Android è un obiettivo successivo. Il CIE iniziale impiega regole, generazione costruttiva, ricerca limitata e validazione: non richiede ML, ONNX o servizi AI remoti.

La sintesi e composizione M2.4 si sviluppano sotto i vincoli già accettati: timeline logica interna a 48 kHz, simulazione a 120 Hz, seed isolati e deterministici, scheduler ahead-of-time e callback audio senza allocazioni o lavoro unbounded. Il pianificatore deve creare **forma, motivi, trasformazioni, funzione armonica e orchestrazione per Pulso/Corpo/Trama/Vettore**, mentre il sintetizzatore esegue gli eventi. L'architettura compositiva suggerita dalla ricerca è **una proposta**, non una modifica tacita di `MusicIntentEvent` o del limite a otto voci.

La valutazione distingue qualità tecnica (underrun/tempo/RT), coerenza simbolica (forma/memoria), varietà e **percezione musicale umana**. Le ipotesi cross-modali musica–geometria saranno studiate a partire da M3, sempre subordinando la generazione al fairness validator di M4.

## Prossimo lavoro

Attivare M2.4 secondo la [specifica §32](CYMATICA_Specifica_Agentica_Sviluppo.md), usando il [piano di implementazione](docs/research/M2_4_Implementation_Plan.md) come supporto. Dopo il voice allocator a 8 voci, il generatore simbolico e il DSP, verificare due sezioni musicalmente distinguibili, trace same-seed, WAV/CACP e profilo realtime. Il [protocollo sperimentale](docs/research/M2_4_Evaluation_Protocol.md) propone un confronto A/B compositivo e timbrico; **non sostituisce il gate M2.5**.

Prima di M3/M4 rimangono da completare i contratti di giocatore e la prova di spike con movimento, pattern, telegraph e fallback; riferimento [player contract M1](docs/player_contract_m1.md). Cimatica e l'adattamento sono modelli di design da verificare, non dimostrazioni già acquisite di accuratezza fisica o psicologica.

## Licenza

La licenza del progetto deve essere stabilita esplicitamente prima della distribuzione, qualora non sia già stata definita nel repository al momento dell'integrazione. Questa documentazione non attribuisce una licenza nuova. Dipendenze e asset conservano i rispettivi obblighi di licenza e provenienza.
