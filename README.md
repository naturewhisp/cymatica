# CYMATICA

Musical bullet hell procedurale: **musica, geometria e minacce derivano da un intento musicale condiviso**, con generazione vincolata, memoria, forma e adattamento graduale. Obiettivo artistico: un'opera audiovisiva con identità compositiva riconoscibile, non un generatore infinito di note casuali.

## Stato reale

Prototipo tecnico Windows sviluppato in C++20 con CMake/Ninja, raylib e miniaudio. M0 e M1 concluse; **M2 attiva**: secondo il [registro milestone](docs/progress.md), M2.1 (decisioni), M2.2 (input/AudioProbe) e M2.3 (transport/scheduler) sono concluse; in **M2.4 (musica procedurale v1)** è stato **completato, bonificato e validato Inc-1** (`VoiceAllocator` a 8 voci, 2/ruolo, steal tails sample-rate adaptive, matrice stati `Pause`/`Resume`/`Stop`/`Start`, CACP `voice_probe_v1` PASS, 91/91 CTest Release/Debug e 90/90 Headless) ed è stato **formalmente chiuso il Gate tecnico pre-Inc-2** (`AUD-D-01` e `AUD-D-02` in [ADR-0004](docs/adr/0004-m2-4-audio-renderer-safety-proposal.md)). Il prossimo passo attivo è **Inc-2 (Sintesi 4 Ruoli, 4 Bus & Protezione Terminale MasterLimiter)**, seguito da Inc-3..Inc-5 e dal gate finale M2.5. Non è ancora una release giocabile.

La [CI GitHub Actions Windows](.github/workflows/windows-ci.yml) verifica automaticamente build/test Headless Release e Debug e il profilo diagnostico CACP offline. Le prove hardware, l'ascolto percettivo, la validazione artistica e il gate indipendente di milestone richiedono evidenze separate; nessun PASS di CI li sostituisce.

## Documentazione

| Documento | Responsabilità |
|---|---|
| [Specifica tecnica](CYMATICA_Specifica_Agentica_Sviluppo.md) | Architettura, contratti, timing, fairness e roadmap normativa (§16, §32) |
| [Design](DESIGN.md) | Identità artistica, gameplay, linguaggio audiovisivo, UX e accessibilità (§8, §16) |
| [ADR-0003](docs/adr/0003-m2-decisions-contracts.md) | Contratti Music Intent, 8 voci 2/ruolo, scheduler e decisioni M2.4 (D-M2-04..07) |
| [ADR-0004](docs/adr/0004-m2-4-audio-renderer-safety-proposal.md) | Renderer nativo C++20, protezione digitale terminale (`MasterLimiter`), domini di monitoring e chiusura Gate pre-Inc-2 |
| [Controllo e osservazione agentica](docs/agent_control_observability.md) | Motivazioni, limiti, CACP, sequenze/streaming e verifiche |
| [Regole degli agenti](AGENTS.md) | Istruzioni operative, controllo scope e milestone gate |
| [Stato e prove milestone](docs/progress.md) | Esecuzione realmente verificata, decisioni d'ingresso, evidenze e blocchi |
| [Indice ricerca musicale M2.4](docs/research/README.md) | Deep Research revisionata, audit delle fonti, studio safe listening, piano di implementazione, evidenze distorsione/aliasing |
| [Roadmap artistica proposta M2.4–M13](docs/roadmap_music_research_addendum.md) | Possibili estensioni di ricerca alle future milestone, **non normative fino all'approvazione** |
| [Build e test](docs/build.md) | Compilazione, CTest, eseguibile e istruzioni locali |

## Direzione tecnica e musicale

Target iniziale Windows x64; Android è un obiettivo successivo. Il CIE iniziale impiega regole, generazione costruttiva, ricerca limitata e validazione: non richiede ML, ONNX o servizi AI remoti.

La sintesi e composizione M2.4 si sviluppano sotto i vincoli già accettati: timeline logica interna a 48 kHz, simulazione a 120 Hz, seed isolati e deterministici, scheduler ahead-of-time e callback audio senza allocazioni o lavoro unbounded. Il pianificatore deve creare **forma, motivi, trasformazioni, funzione armonica e orchestrazione per Pulso/Corpo/Trama/Vettore**, mentre il sintetizzatore esegue gli eventi.

La valutazione distingue qualità tecnica (underrun/tempo/RT), coerenza simbolica (forma/memoria), varietà e **percezione musicale umana**. Le ipotesi cross-modali musica–geometria saranno studiate a partire da M3, sempre subordinando la generazione al fairness validator di M4.

## Prossimo lavoro

Procedere con **M2.4 Inc-2 (Sintesi 4 Ruoli & 4 Bus + `MasterLimiter` zero-latency)** secondo le specifiche chiuse in [ADR-0004](docs/adr/0004-m2-4-audio-renderer-safety-proposal.md) e [ADR-0003](docs/adr/0003-m2-decisions-contracts.md), usando il [piano di implementazione](docs/research/M2_4_Implementation_Plan.md) e `docs/progress.md` come riferimento operativo. Successivamente completare Inc-3 (Music Planner gerarchico), Inc-4 (Fixture A-B-A' 96 battute & profiling $\le 2,0\text{ ms}$) e Inc-5 (integrazione CACP completa e handoff al gate M2.5). Il [protocollo sperimentale](docs/research/M2_4_Evaluation_Protocol.md) propone un confronto A/B compositivo e timbrico; **non sostituisce il gate M2.5**.

Il contratto minimo del giocatore `PlayerProfile_v1` è **già congelato in M1** come `accepted for implementation` ([player contract M1](docs/player_contract_m1.md)). Prima di M3/M4 restano da implementare e verificare movimento, un pattern, telegraph e fallback; il profilo necessita ancora della validazione empirica prevista in M5/M7 e di regressioni del validator in caso di modifiche. Cimatica e adattamento rimangono modelli di design da verificare, non prove acquisite di accuratezza fisica o psicologica.

## Licenza

La licenza del progetto deve essere stabilita esplicitamente prima della distribuzione, qualora non sia già stata definita nel repository al momento dell'integrazione. Questa documentazione non attribuisce una licenza nuova. Dipendenze e asset conservano i rispettivi obblighi di licenza e provenienza.
