# CYMATICA

Musical bullet hell procedurale: **musica, geometria e minacce derivano da un intento musicale condiviso**, con generazione vincolata, memoria, forma e adattamento graduale. Obiettivo artistico: un'opera audiovisiva con identità compositiva riconoscibile, non un generatore infinito di note casuali.

## Stato reale

Prototipo tecnico Windows sviluppato in C++20 con CMake/Ninja, raylib e miniaudio. M0 e M1 concluse; **M2 attiva**: M2.1–M2.4 concluse (M2.4, musica procedurale v1, chiusa il 2026-11-03); **M2.5 (integrazione e verifica finale) in corso** a sotto-fasi secondo [ADR-0007](docs/adr/0007-m2-5-entry-decisions.md), con la prima verifica su device eseguita e gli interventi su mix e struttura non ancora autorizzati. Stato dettagliato, conteggi dei test ed evidenze sono solo nel [registro milestone](docs/progress.md). Non è ancora una release giocabile.

La [CI GitHub Actions Windows](.github/workflows/windows-ci.yml) verifica automaticamente build/test Headless Release e Debug e il profilo diagnostico CACP offline. Le prove hardware, l'ascolto percettivo, la validazione artistica e il gate indipendente di milestone richiedono evidenze separate; nessun PASS di CI li sostituisce.

## Documentazione

| Documento | Responsabilità |
|---|---|
| [Specifica tecnica](CYMATICA_Specifica_Agentica_Sviluppo.md) | Architettura, contratti, timing, fairness e roadmap normativa (§16, §32) |
| [Design](DESIGN.md) | Identità artistica, gameplay, linguaggio audiovisivo, UX e accessibilità (§8, §16) |
| [ADR-0003](docs/adr/0003-m2-decisions-contracts.md) | Contratti Music Intent, 8 voci 2/ruolo, scheduler e decisioni M2.4 (D-M2-04..07) |
| [ADR-0004](docs/adr/0004-m2-4-audio-renderer-safety-proposal.md) | Renderer nativo C++20, protezione digitale terminale (`MasterLimiter`), domini di monitoring e chiusura Gate pre-Inc-2; emendamento post-Inc-3 §4.3 |
| [ADR-0005](docs/adr/0005-m2-4-inc4-entry-decisions.md) | Decisioni d'ingresso M2.4 Inc-4: espansione fixture, orizzonte di pianificazione ed epoch, profiling realtime, scenari CACP |
| [ADR-0006](docs/adr/0006-m2-4-inc5-entry-decisions.md) | Decisioni d'ingresso M2.4 Inc-5: catena degli eventi applicati, stem, descrittori, pacchetto di valutazione, calibrazione |
| [ADR-0007](docs/adr/0007-m2-5-entry-decisions.md) | M2.5: sotto-fasi, checkpoint CP-1..CP-5, calendario delle decisioni, regola di chiusura |
| [Controllo e osservazione agentica](docs/agent_control_observability.md) | Motivazioni, limiti, CACP, sequenze/streaming e verifiche |
| [Regole degli agenti](AGENTS.md) | Istruzioni operative, controllo scope e milestone gate |
| [Stato e prove milestone](docs/progress.md) | Esecuzione realmente verificata, decisioni d'ingresso, evidenze e blocchi |
| [Indice della ricerca M2.4 e M2.5](docs/research/README.md) | Ricerca compositiva e audio di M2.4, dossier di M2.5 (direttore musicale, glitch su Windows, baseline su device), [revisione d'insieme](docs/research/CYMATICA_Analysis_Consolidation_2026-11-09.md) |
| [Roadmap artistica proposta M2.4–M13](docs/roadmap_music_research_addendum.md) | Possibili estensioni di ricerca alle future milestone, **non normative fino all'approvazione** |
| [Build e test](docs/build.md) | Compilazione, CTest, eseguibile e istruzioni locali |

## Direzione tecnica e musicale

Target iniziale Windows x64; Android è un obiettivo successivo. Il CIE iniziale impiega regole, generazione costruttiva, ricerca limitata e validazione: non richiede ML, ONNX o servizi AI remoti.

La sintesi e composizione M2.4 si sviluppano sotto i vincoli già accettati: timeline logica interna a 48 kHz, simulazione a 120 Hz, seed isolati e deterministici, scheduler ahead-of-time e callback audio senza allocazioni o lavoro unbounded. Il pianificatore deve creare **forma, motivi, trasformazioni, funzione armonica e orchestrazione per Pulso/Corpo/Trama/Vettore**, mentre il sintetizzatore esegue gli eventi.

La valutazione distingue qualità tecnica (underrun/tempo/RT), coerenza simbolica (forma/memoria), varietà e **percezione musicale umana**. Le ipotesi cross-modali musica–geometria saranno studiate a partire da M3, sempre subordinando la generazione al fairness validator di M4.

## Prossimo lavoro

Completare **M2.5.1** (strumenti offline, loudness per stem, viste leggibili) e tenere il checkpoint **CP-1** di [ADR-0007](docs/adr/0007-m2-5-entry-decisions.md): cambio di metro, criterio PASS del device, protocollo d'ascolto. Seguono mix e immagine stereo (M2.5.2), transizioni e struttura (M2.5.3), congelamento e ricalibrazione di AC-AO-02 (M2.5.5) e la qualificazione finale (M2.5.6). Lo stato aggiornato è solo nel [registro milestone](docs/progress.md). **Si riparte dal «Punto di ripartenza» della [revisione d'insieme](docs/research/CYMATICA_Analysis_Consolidation_2026-11-09.md)**, che ordina i lavori fino a CP-1.

Il contratto minimo del giocatore `PlayerProfile_v1` è **già congelato in M1** come `accepted for implementation` ([player contract M1](docs/player_contract_m1.md)). Prima di M3/M4 restano da implementare e verificare movimento, un pattern, telegraph e fallback; il profilo necessita ancora della validazione empirica prevista in M5/M7 e di regressioni del validator in caso di modifiche. Cimatica e adattamento rimangono modelli di design da verificare, non prove acquisite di accuratezza fisica o psicologica.

## Licenza

La licenza del progetto deve essere stabilita esplicitamente prima della distribuzione, qualora non sia già stata definita nel repository al momento dell'integrazione. Questa documentazione non attribuisce una licenza nuova. Dipendenze e asset conservano i rispettivi obblighi di licenza e provenienza.
