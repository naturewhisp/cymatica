# CYMATICA — Roadmap di integrazione della ricerca musicale (proposta)

- **Data:** 2026-10-13
- **Baseline:** commit [`460bc286378b473218bc07f751e3615964626a11`](https://github.com/naturewhisp/cymatica/commit/460bc286378b473218bc07f751e3615964626a11)
- **Stato:** **proposta di integrazione**, non modifica automatica ai criteri normativi delle milestone.
- **Fonte:** [Deep Research revisionata](research/M2_4_Deep_Research_Reviewed.md), [review](research/M2_4_Research_Review.md), [protocollo sperimentale](research/M2_4_Evaluation_Protocol.md).
- **Authority:** [Spec §32](../CYMATICA_Specifica_Agentica_Sviluppo.md), [DESIGN](../DESIGN.md) e ADR già approvati prevalgono. Questo documento serve a identificare *quando* promuovere le proposte a decisioni d'ingresso e verifiche normative.

## Decisione di metodo

Non rinumerare M2–M13 e **non riaprire M2.3** per ragioni artistiche. M2.4 implementa una composizione v1 realmente musicale e il sintetizzatore 8 voci; M2.5 ne verifica l'integrazione secondo gli attuali criteri obbligatori. L'eccellenza artistica richiede iterazioni progressive e playtest umani durante M3–M7: non può essere certificata una volta per tutte da un prototipo di tre minuti.

La ricerca non autorizza nuove dipendenze, ML, formati o contratti pubblici. Prima di una decisione che modifichi `MusicIntentEvent`, voce DSP o budget scheduler, applicare AGENTS §1.1 e aggiornare l'ADR pertinente con status `proposed`/`accepted`/`validated` distinto.

## Pianificazione per milestone

| Milestone | Adeguamento raccomandato | Tipo / decisione necessaria | Evidenza consigliata |
|---|---|---|---|
| **M2.4 — Musica procedurale v1** | Affiancare al minimo normativo *motif object* relativo, frase con obiettivo armonico, candidate search bounded, ritorno trasformato fra le due sezioni e 4 firme timbriche. Implementare pairing per `(role,eventId)` e lifecycle cross-epoch secondo ADR-0003. | **Piano implementativo nel perimetro vigente**; ricerca non crea nuovi hard gate. | Golden seed/trace, clip ascoltabili, A/B gerarchia vs baseline, DSP profiling, pairing PCM |
| **M2.5 — Integrazione** | Aggiungere dossier diagnostico di qualità: clipping/aliasing/voice stealing, budget callback e feedback di ascolto umano/supplementare dell'agente; mantenere obbligatori tre minuti/due sezioni/sync/AC-AO-02. | **Evidenze additive**, non sostituzione dei gate attuali. | WAV/manifest con commit+profilo+seed, run hardware, note su qualità e incertezze |
| **M3 — Catalogo/geometria** | Definire prima dei pattern che dipendono da motivi un **piano semantico condiviso** o sidecar bounded e versionato (`motifId`, `phraseId`, `transformId`, time bounds) per due proiezioni musica/gioco. Prototipare G1 beat-only e G2 intent-symbolic; G3 strutturale solo dopo contratti. | **Decisione d'ingresso M3 da registrare**; non alterare `MusicIntentEvent` senza D-M2-02 review. | Same-seed plan hash, pattern deterministici, eventi gameplay da shared plan, telegraph invariants |
| **M4 — Fairness validator** | Validare che riflessione, augmentation, densificazione, cadenza e trasformazioni morfologiche **non implicano pericolo maggiore** senza test di raggiungibilità. Ogni trasformazione musicale candidata diventa vincolo o dato, mai eccezione al validator. | Estensione del catalogo dei test quando il mapping è implementato; **validator sempre autoritativo**. | Fixture con G1/G2/G3, composizione di hazard, swept transitions, fallback musicale, batch seed |
| **M5 — Player loop** | Valutare in sessioni controllate se segnali timbrici/ritmici anticipano pericolo e recovery senza sovrastare feedback immediato, telegraph e accessibilità. | Studio di UX e tuning sotto DESIGN, non inferire latenza audio fisica dalla telemetria. | Test con ascolto/visuale insieme, profili reduced intensity, feedback dash e mix priorities |
| **M6 — CIE Director** | Progettare adaptation **identity-preserving** su futuro modificabile: invarianti motivici/cadenzali vs gradi di libertà timbrici e densità. Spikes su *shared surprisal budget* e tracciamento musicale senza sottrarre autorità al System 2. | **Decisione d'ingresso M6 proposta**: pianificazione musicale e gameplay devono condividere l'intenzione senza alterare il già committed. | A/B stesso seed stati giocatore diversi, trace invariants, prova safety guard, leggibilità delle revisioni |
| **M7 — Vertical slice** | Non limitare "identità" a screenshot/archetipi: aggiungere **valutazione umana audiovisiva** della riconoscibilità dei motivi, grammatica spaziale, fatica su lunga run, coerenza e differenziazione timbrica. | Proporre un **checkpoint artistico umano**; se nuovo gate mandatory, deliberare soglie/procedura prima del test e aggiornare Spec/DESIGN. | G1/G2/G3 su livelli fair equivalenti, test riconoscimento A/A', ascolto 10 min, percezione fairness |
| **M8 — AI Lab/quality-diversity** | Estendere la descrizione dei candidati a motivi, trasformazioni, archetipi timbrici e tension curves **se** usate nel search; separare diversità statistica e apprezzamento umano. Nessun candidato sfugge al validator. | Decisione su feature e versioning archive; il System 1 resta retriever, non compositore/validator autonomo. | Coverage musicale+spaziale, near-duplicate detection, trade-off originalità/fairness, Recall@K |
| **M9 — Pacchetti/export** | Se l'authoring richiede preserving semantic plan, definire schema versionato opzionale per motif/phrase/transform IDs e derivazione di audio/gameplay; non introdurlo durante M2.4. | Revisione schema `cymlevel` al momento di M9. | Round-trip/pacchetto riproducibile senza dipendenza da librerie di ricerca |
| **M10 — Custom audio mix-only** | Le metriche di memoria/forma definite sul generatore simbolico **non** si applicano automaticamente a brani importati: distinguere segnale analizzato, confidence e mapping a ruoli. | Decisione separata sulla semantica del custom-audio. | Confronto fra generazione nativa e analisi beat/onset, fallback basso confidence |
| **M11 — NatuStem** | Nessuna modifica necessaria ora: modelli/stem opzionali e tool-only. | Confermare solo dopo M10 quando si verifica utilità artistica e licenze. | Provenance licenze/stem e fallback mix-only |
| **M12 — Android preview** | Rieseguire budget DSP e ascolto sul device target reale, con quantum, thermal/throttling, audio route e sample rate effettivi; nessuna estrapolazione automatica dal runner Windows. | Nuovo profilo device/callback da validare; ADR se il budget richiede modifica. | Deadline misses, underrun, consumo, latency, fallback sonore senza alterare Seed/Intent |
| **M13 — Studio/editor** | Se il sistema compositivo dimostra valore, esporre strumenti di authoring per motives/form/harmonic graph e audition A/B; non scegliere framework/editor ora. | Scelta tool dopo stabilizzazione dei formati M9–M10. | Workflow autoriale e tracciamento/versionamento decisioni artistiche |

## Cosa non introdurre nell'attuale roadmap

- Una milestone aggiuntiva obbligatoria di Deep Research: la ricerca è un input a M2.4, non un blocco indefinito.
- Una riscrittura della sintesi in Wwise/FMOD/Strudel, né neural inference live.
- Un aumento di voci a 12–16, pool dinamico o granular composite senza profiling + approvazione D-M2-03.
- Un `artistic_score` derivato automaticamente da frequenza, entropia o novelty e usato come gate.
- Un nuovo obbligo di PCM bit-exact cross-platform; il replay canonico riguarda eventi/decisioni simbolici finché non viene deciso altro.
- Una promessa che la corrispondenza motivo–geometria migliori fairness: è una **ipotesi** sotto l'autorità di M4.

## Sequenza dei checkpoint

1. **Prima di M2.4 DSP completo:** convalidare il contratto voice identity/sustain/steal già accettato, definire la prima palette e l'esperimento A/B. L'architettura di Research è candidata, non congelata.
2. **Durante M2.4:** produrre stessi seed e trace, render WAV e misure, un primo confronto ascoltato; annotare errori e contesto d'ascolto.
3. **Prima di chiudere M2.5:** completare tutti gli acceptance M2 attuali e allegare evidenza tecnica/percepita distinta. Non dichiarare eccellenza artistica solo perché CTest è verde.
4. **Ingresso M3/M4:** decidere interfaccia strutturale e mappature vincolate; aggiornare documenti normativi se realmente cambia il contratto.
5. **Ingresso M6/M7:** congelare ipotesi e metriche di valutazione umana prima del confronto; registrare anche i risultati negativi e separare preferenza da correttezza/fairness.
6. **Android M12:** ricalibrare/perfilare budget DSP per dispositivo; nessuna traslazione automatica di target Windows.

## Policy documentale

Questa roadmap è un **registro ragionato di proposte future**. Per promuovere una riga a requisito obbligatorio, il responsabile dovrà aggiornare **entrambi** i documenti normativi interessati (Spec e DESIGN se coinvolge esperienza), l'ADR, il piano test e il `docs/progress.md` con decision ID, status, giustificazione e prova. Senza tale promozione, il requisito della milestone rimane quello della Spec §32 al commit di baseline o alle sue successive revisioni approvate.
