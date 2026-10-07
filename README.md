# CYMATICA

Musical bullet hell procedurale: musica, geometria e minacce condividono un intento musicale, con generazione vincolata e adattamento graduale.

## Stato reale

Prototipo tecnico Windows con build CMake, audio miniaudio, finestra/input raylib, clock e contratti deterministici M1. Non è ancora una release giocabile. [docs/progress.md](docs/progress.md) registra M0/M1 concluse e **M2 attiva**; le evidenze storiche non sono state rieseguite nella revisione documentale del 7 ottobre 2026.

La specifica 0.8.3 integra l'infrastruttura progressiva di controllo agentico e osservazione audiovisiva richiesta dal titolare. Sono pianificati test spazio/tono sulla baseline M1, sequenze, clip e osservazione live; questi nuovi strumenti non sono ancora implementati. Parametri di tuning e scelte di trasporto/codec restano proposti o differiti.

## Documenti

| File | Responsabilità |
|---|---|
| [Specifica tecnica](CYMATICA_Specifica_Agentica_Sviluppo.md) | Architettura, contratti, timing, generazione, fairness e roadmap; revisione 0.8.3 |
| [Design](DESIGN.md) | Gameplay, linguaggio audiovisivo, controlli, UX e accessibilità; revisione 0.3.1 |
| [Analisi controllo e osservazione agentica](docs/agent_control_observability.md) | Motivazioni, limiti, sequenze/streaming e piano delle verifiche |
| [Regole agenti](AGENTS.md) | Istruzioni operative e limiti di scope |
| [Stato milestone](docs/progress.md) | Milestone attiva, decisioni d'ingresso, evidenze e blocchi |

## Direzione tecnica

Target iniziale Windows x64; C++20, CMake, raylib per finestra/input/render e miniaudio con un solo owner audio. Android è un obiettivo successivo. Il CIE iniziale usa regole, generazione costruttiva, ricerca limitata e validazione: non richiede ML, ONNX o servizi AI remoti.

Priorità M0–M7: Infinite Resonance e vertical slice. Tool CLI, formati di scambio, custom music, stem separation, AI Lab ed editor seguono le milestone della specifica.

## Prossimo lavoro

M2 è organizzata in cinque sottofasi: decisioni/contratti, controllo e osservazione del tono, transport/scheduler, musica procedurale, integrazione/verifica finale. La [specifica §32](CYMATICA_Specifica_Agentica_Sviluppo.md) definisce dipendenze e verifiche; [progress.md](docs/progress.md) ne traccia l'esecuzione. Rimane un unico gate finale M2 con tutti i criteri esistenti e la review indipendente.

M2 deve introdurre Music Intent, scheduler e musica procedurale v1. Setup e comandi esistenti sono in [docs/build.md](docs/build.md). L'estensione agentica parte dal test 220 → 440 → 220 Hz sulla baseline M1 e da AudioProbe, con contratti e capacità da congelare prima dell'implementazione (specifica §31.5/§32).

Il contratto minimo del player è congelato in [docs/player_contract_m1.md](docs/player_contract_m1.md); prima di M3/M4 costruire e verificare lo spike con movimento, un pattern, telegraph e fallback. Cimatica e adattamento sono modelli di design da validare, non prove di accuratezza fisica o psicologica.

## Licenza

Nella baseline esaminata non è presente un file LICENSE. La licenza del progetto deve essere decisa dal titolare prima della distribuzione; questa revisione non ne assegna una. Le dipendenze e gli asset manterranno i propri requisiti di licenza.
