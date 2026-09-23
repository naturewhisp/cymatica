# CYMATICA

Musical bullet hell procedurale: musica, geometria e minacce condividono un intento musicale, con generazione vincolata e adattamento graduale.

## Stato reale

Pre-produzione documentale. La baseline esaminata è il commit `4aa2d5271aea11ad8aa088932c5afad8d6aaeec7` del 9 settembre 2026. Contiene specifica, design e regole operative; non contiene ancora sorgenti del gioco, CMake, test o una release giocabile. Milestone attiva: **M0**.

Questi documenti revisionati il 23 settembre 2026 sono una proposta di aggiornamento. Non attestano che i requisiti siano già implementati.

## Documenti

| File | Responsabilità |
|---|---|
| [Specifica tecnica](CYMATICA_Specifica_Agentica_Sviluppo.md) | Architettura, contratti, timing, generazione, fairness e roadmap; revisione 0.8 |
| [Design](DESIGN.md) | Gameplay, linguaggio audiovisivo, controlli, UX e accessibilità; revisione 0.2 |
| [Regole agenti](AGENTS.md) | Istruzioni operative e limiti di scope |
| [Rapporto di revisione](RAPPORTO_REVISIONE.md) | Problemi documentali, soluzioni, test richiesti e limiti della verifica |

## Direzione tecnica

Target iniziale Windows x64; C++20, CMake, raylib per finestra/input/render e miniaudio con un solo owner audio. Android è un obiettivo successivo. Il CIE iniziale usa regole, generazione costruttiva, ricerca limitata e validazione: non richiede ML, ONNX o servizi AI remoti.

Priorità M0–M7: Infinite Resonance e vertical slice. Tool CLI, formati di scambio, custom music, stem separation, AI Lab ed editor seguono le milestone della specifica.

## Prossimo lavoro

M0 deve introdurre build minima riproducibile, dipendenze pinned e licenze/provenienza, test framework, finestra/input, tono audio e shader di prova. I comandi di build saranno documentati in `docs/build.md` quando esisteranno e saranno stati verificati. Non ci sono ancora comandi di installazione o eseguibili da proporre.

Prima di M3/M4 definire il contratto minimo del player e costruire uno spike giocabile con un pattern, telegraph e fallback. Cimatica e adattamento sono modelli di design da validare, non prove di accuratezza fisica o psicologica.

## Licenza

Nella baseline esaminata non è presente un file LICENSE. La licenza del progetto deve essere decisa dal titolare prima della distribuzione; questa revisione non ne assegna una. Le dipendenze e gli asset manterranno i propri requisiti di licenza.
