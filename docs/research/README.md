# CYMATICA — Ricerca musicale M2.4

Documenti dedicati alla qualità compositiva, timbrica e audiovisiva di CYMATICA. **Research ≠ requisito normativo né feature implementata.** Il primo corpus ha baseline `460bc286378b473218bc07f751e3615964626a11` (2026-10-13); la review audio successiva si riferisce esplicitamente a `090dc2a45cf3785a2055c492415f9151321ea18a` (2026-10-14).

| Documento | Uso |
|---|---|
| [Deep Research revisionata](M2_4_Deep_Research_Reviewed.md) | Stato dell'arte della composizione, confronto tecniche, bibliografia con DOI/link permanenti |
| [Review critica compositiva](M2_4_Research_Review.md) | Correzioni editoriali, forza delle evidenze, fonti NotebookLM, limiti, contratti invariati |
| [Review indipendente architettura audio (2026-10-15)](M2_4_Audio_Renderer_Research_Review.md) | Scelta B motivata: renderer nativo e DSP open source selettivo/auditato; limiti della ricerca, alternative CLAP/VST3, fonti primarie |
| [Studio tecnico protezione audio e LUFS](M2_4_Audio_Safety_Design.md) | Limiter master zero-latency e metering: principi architetturali ratificati con ADR-0004 e Gate tecnico pre-Inc-2 chiuso (2026-10-19) |
| [ADR-0004: renderer/sicurezza (RATIFICATO)](../adr/0004-m2-4-audio-renderer-safety-proposal.md) | **Accepted for implementation**, con emendamenti a D-M2-06/D-M2-07 e Gate tecnico pre-Inc-2 chiuso (AUD-D-01, AUD-D-02) |
| [Piano di implementazione M2.4](M2_4_Implementation_Plan.md) | Roadmap Inc-1..5 (Inc-1 completato, Gate pre-Inc-2 chiuso), contratti voice/epoch, palette, test e differimento delle scelte sperimentali |
| [Analisi scientifica distorsione e aliasing Inc-1](M2_4_Voice_Distortion_Aliasing_Evidence.md) | Diagnosi quantitativa campioni clippati, THD+N, aliasing da hard-clipping e gain staging in `voice_probe_v1` |
| [Protocollo di valutazione](M2_4_Evaluation_Protocol.md) | Esperimenti A/B, test di ascolto, metriche separate e profiling realtime |
| [Roadmap di ricerca futura](../roadmap_music_research_addendum.md) | Conseguenze proposte per M2.4/M2.5 e M3–M13 |

**Gerarchia documentale:** [Specifica](../../CYMATICA_Specifica_Agentica_Sviluppo.md) → [DESIGN](../../DESIGN.md) → [AGENTS](../../AGENTS.md) → [ADR-0003](../adr/0003-m2-decisions-contracts.md) e [ADR-0004 ratificato](../adr/0004-m2-4-audio-renderer-safety-proposal.md) → [piano operativo](../progress.md) → documentazione di ricerca. I documenti di ricerca restano non normativi; l'ADR-0004 **ratificato** è vincolante nei soli punti dichiarati, **non** dimostra implementazione di Inc-2..Inc-5 prima della loro esecuzione.

**Stato operativo M2.4 (2026-10-19):** **Inc-1** (`VoiceAllocator`, 8 voice slot statici, voice stealing deterministico, steal tails 2 ms sample-rate adaptive, `voice_probe_v1`) è **COMPLETATO** e verificato; il **Gate tecnico pre-Inc-2** (`AUD-D-01` qualifica DSP C++20 nativo puro senza dipendenze esterne; `AUD-D-02` topologia e algoritmo limiter terminale zero-latency $L=0$ + verifica true-peak ITU-R BS.1770-5 Annex 2 offline) è **CHIUSO**. H.872 e ulteriori gate di loudness rolling a lungo termine restano differiti a M5+. Non aumentare il budget di otto voci senza ADR e non usare la sola verifica CACP come prova di qualità artistica. M2.1–M2.3 rimangono concluse secondo `docs/progress.md`.
