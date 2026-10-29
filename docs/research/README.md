# CYMATICA — Ricerca musicale M2.4

Documenti dedicati alla qualità compositiva, timbrica e audiovisiva di CYMATICA. **Research ≠ requisito normativo né feature implementata.** Il primo corpus ha baseline `460bc286378b473218bc07f751e3615964626a11` (2026-10-13); la review audio successiva si riferisce esplicitamente a `090dc2a45cf3785a2055c492415f9151321ea18a` (2026-10-14).

| Documento | Uso |
|---|---|
| [Deep Research revisionata](M2_4_Deep_Research_Reviewed.md) | Stato dell'arte della composizione, confronto tecniche, bibliografia con DOI/link permanenti |
| [Review critica compositiva](M2_4_Research_Review.md) | Correzioni editoriali, forza delle evidenze, fonti NotebookLM, limiti, contratti invariati |
| [Review indipendente architettura audio (2026-10-15)](M2_4_Audio_Renderer_Research_Review.md) | Scelta B motivata: renderer nativo e DSP open source selettivo/auditato; limiti della ricerca, alternative CLAP/VST3, fonti primarie |
| [Studio tecnico protezione audio e LUFS](M2_4_Audio_Safety_Design.md) | Limiter master zero-latency e metering: principi architetturali ratificati con ADR-0004 e Gate tecnico pre-Inc-2 chiuso (2026-10-19) |
| [ADR-0004: renderer/sicurezza (RATIFICATO)](../adr/0004-m2-4-audio-renderer-safety-proposal.md) | **Accepted for implementation**, con emendamenti a D-M2-06/D-M2-07, Gate tecnico pre-Inc-2 chiuso (AUD-D-01, AUD-D-02) ed emendamento post-Inc-3 §4.3 |
| [ADR-0005: ingresso Inc-4](../adr/0005-m2-4-inc4-entry-decisions.md) | D-M2-08..11: espansione fixture, orizzonte ed epoch, profiling realtime, scenari CACP (`accepted for implementation`) |
| [Piano di implementazione M2.4](M2_4_Implementation_Plan.md) | Roadmap Inc-1..5 (Inc-1 completato, Gate pre-Inc-2 chiuso), contratti voice/epoch, palette, test e differimento delle scelte sperimentali |
| [Analisi scientifica distorsione e aliasing Inc-1](M2_4_Voice_Distortion_Aliasing_Evidence.md) | Diagnosi quantitativa campioni clippati, THD+N, aliasing da hard-clipping e gain staging in `voice_probe_v1` |
| [DIF-INC4-R2-01: workload W e GR del limiter (2026-10-21)](M2_4_DIF-INC4-R2-01_Limiter_GR_Analysis.md) | Analisi con misure delle alternative per il carico W di D-M2-10 (livelli raggiungibili, costo della GR, subnormali, variabilità delle code); dati grezzi in `experiments/dif_inc4_r2_01/` |
| [Metodi di valutazione CACP (2026-10-21)](M2_4_CACP_Evaluation_Methods.md) | Ingestione audio, spettrogrammi, descrittori e telemetria a confronto con misure e fonti; raccomandazioni per Inc-5 (*proposed*) e ricerche d'ingresso M5/M6; dati in `experiments/cacp_methods/` |
| [Ricerca d'ingresso Inc-5 (2026-10-26)](M2_4_Inc5_Entry_Research.md) | Identità e diversità musicale (fingerprint, metriche, esiti della misura di diversità), separazione della ground truth per agenti valutatori, onset per ruolo, stato dell'arte di giochi con musica procedurale/adattiva; sostiene [ADR-0006](../adr/0006-m2-4-inc5-entry-decisions.md) |
| [Evaluation pack v1 (2026-10-29)](experiments/inc5_evaluation_pack/README.md) | Fase 5d: presenza per frase e ruolo sulla fixture (soglie coerenti ma non validate), manifest dello scenario, baseline banale contro il checker `claims_v0` |
| [Calibrazione AC-AO-02 per ruolo (2026-10-29)](M2_4_Inc5_AC-AO-02_Calibration.md) | Fase 5c: stem per ruolo, latenza intrinseca, calibrazione sul seed 1337, profilo congelato, verifica, soglie proposte, finding F6 (rilascio di 2 ms) e diversità fra seed nulla; dati in `experiments/inc5_calibration/` |
| [ADR-0006: ingresso Inc-5](../adr/0006-m2-4-inc5-entry-decisions.md) | D-M2-12..16: catena degli eventi applicati e fingerprint, stem, descrittori, pacchetto di valutazione, calibrazione (`accepted for implementation`, in review) |
| [Protocollo di valutazione](M2_4_Evaluation_Protocol.md) | Esperimenti A/B, test di ascolto, metriche separate e profiling realtime |
| [Roadmap di ricerca futura](../roadmap_music_research_addendum.md) | Conseguenze proposte per M2.4/M2.5 e M3–M13 |

**Gerarchia documentale:** [Specifica](../../CYMATICA_Specifica_Agentica_Sviluppo.md) → [DESIGN](../../DESIGN.md) → [AGENTS](../../AGENTS.md) → [ADR-0003](../adr/0003-m2-decisions-contracts.md) e [ADR-0004 ratificato](../adr/0004-m2-4-audio-renderer-safety-proposal.md) → [piano operativo](../progress.md) → documentazione di ricerca. I documenti di ricerca restano non normativi; l'ADR-0004 **ratificato** è vincolante nei soli punti dichiarati, **non** dimostra implementazione di Inc-2..Inc-5 prima della loro esecuzione.

**Stato operativo M2.4:** registrato solo in [progress.md](../progress.md). Le decisioni normative correnti sono in ADR-0003, ADR-0004 (emendamento §4.3 del 2026-10-20), [ADR-0005](../adr/0005-m2-4-inc4-entry-decisions.md) (ingresso Inc-4) e [ADR-0006](../adr/0006-m2-4-inc5-entry-decisions.md) (ingresso Inc-5, in review). H.872 e gate di loudness rolling restano differiti a M5+. Non aumentare il budget di otto voci senza ADR e non usare la sola verifica CACP come prova di qualità artistica.
