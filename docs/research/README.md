# CYMATICA — Ricerca musicale M2.4

Documenti dedicati alla qualità compositiva, timbrica e audiovisiva di CYMATICA. **Research ≠ requisito normativo né feature implementata.** Il primo corpus ha baseline `460bc286378b473218bc07f751e3615964626a11` (2026-10-13); la review audio successiva si riferisce esplicitamente a `090dc2a45cf3785a2055c492415f9151321ea18a` (2026-10-14).

| Documento | Uso |
|---|---|
| [Deep Research revisionata](M2_4_Deep_Research_Reviewed.md) | Stato dell'arte della composizione, confronto tecniche, bibliografia con DOI/link permanenti |
| [Review critica compositiva](M2_4_Research_Review.md) | Correzioni editoriali, forza delle evidenze, fonti NotebookLM, limiti, contratti invariati |
| [Review indipendente architettura audio (2026-10-15)](M2_4_Audio_Renderer_Research_Review.md) | Scelta B motivata: renderer nativo e DSP open source selettivo/auditato; limiti della ricerca, alternative CLAP/VST3, fonti primarie |
| [Studio tecnico protezione audio e LUFS](M2_4_Audio_Safety_Design.md) | Limiter master e metering: principi architetturali ratificati con ADR-0004, algoritmi/soglie ancora sperimentali |
| [ADR-0004: renderer/sicurezza (RATIFICATO)](../adr/0004-m2-4-audio-renderer-safety-proposal.md) | **Accepted for implementation**, con emendamenti mirati a D-M2-06/D-M2-07 e gate locale pre-Inc-2 |
| [Piano di implementazione M2.4](M2_4_Implementation_Plan.md) | Roadmap Inc-1..5, contratti voice/epoch, palette, test e differimento delle scelte sperimentali |
| [Protocollo di valutazione](M2_4_Evaluation_Protocol.md) | Esperimenti A/B, test di ascolto, metriche separate e profiling realtime |
| [Roadmap di ricerca futura](../roadmap_music_research_addendum.md) | Conseguenze proposte per M2.4/M2.5 e M3–M13 |

**Gerarchia documentale:** [Specifica](../../CYMATICA_Specifica_Agentica_Sviluppo.md) → [DESIGN](../../DESIGN.md) → [AGENTS](../../AGENTS.md) → [ADR-0003](../adr/0003-m2-decisions-contracts.md) e [ADR-0004 ratificato](../adr/0004-m2-4-audio-renderer-safety-proposal.md) → [piano operativo](../progress.md) → documentazione di ricerca. I documenti di ricerca restano non normativi; l'ADR-0004 **ratificato** è vincolante nei soli punti dichiarati, **non** dimostra implementazione o validazione di DSP/limiter/LUFS.

Prima di Inc-1 l'utente deve autorizzare separatamente l'avvio operativo. Prima di Inc-2 devono essere scelti algoritmo/topologia/latency del limiter e qualificati gli eventuali DSP esterni; ceiling numerico, H.872 e ulteriori gate LUFS restano differiti. Non aumentare il budget di otto voci senza ADR e non usare la sola verifica CACP come prova di qualità artistica. M2.1–M2.3 rimangono concluse secondo `docs/progress.md`, senza riaperture formali in questi documenti.
