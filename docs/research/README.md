# CYMATICA — Ricerca musicale M2.4

Documenti dedicati alla qualità compositiva, timbrica e audiovisiva di CYMATICA. **Research ≠ requisito normativo né feature implementata.** Il primo corpus ha baseline `460bc286378b473218bc07f751e3615964626a11` (2026-10-13); la review audio successiva si riferisce esplicitamente a `090dc2a45cf3785a2055c492415f9151321ea18a` (2026-10-14).

| Documento | Uso |
|---|---|
| [Deep Research revisionata](M2_4_Deep_Research_Reviewed.md) | Stato dell'arte della composizione, confronto tecniche, bibliografia con DOI/link permanenti |
| [Review critica compositiva](M2_4_Research_Review.md) | Correzioni editoriali, forza delle evidenze, fonti NotebookLM, limiti, contratti invariati |
| [Review indipendente architettura audio (2026-10-15)](M2_4_Audio_Renderer_Research_Review.md) | Scelta B motivata: renderer nativo e DSP open source selettivo/auditato; limiti della ricerca, alternative CLAP/VST3, fonti primarie |
| [Proposta tecnica di protezione audio e LUFS](M2_4_Audio_Safety_Design.md) | Limiter master, loudness, true peak, riferimenti ITU/EBU, limiti delle garanzie e implementazione progressiva **non ratificata** |
| [ADR-0004: renderer/sicurezza (PROPOSTA)](../adr/0004-m2-4-audio-renderer-safety-proposal.md) | Decisioni sottoposte al titolare; **nessuna modifica automatica** a D-M2-06/D-M2-07 |
| [Piano di implementazione M2.4](M2_4_Implementation_Plan.md) | Roadmap Inc-1..5, contratti voice/epoch, palette, test e differimento delle scelte sperimentali |
| [Protocollo di valutazione](M2_4_Evaluation_Protocol.md) | Esperimenti A/B, test di ascolto, metriche separate e profiling realtime |
| [Roadmap di ricerca futura](../roadmap_music_research_addendum.md) | Conseguenze proposte per M2.4/M2.5 e M3–M13 |

**Gerarchia documentale:** [Specifica](../../CYMATICA_Specifica_Agentica_Sviluppo.md) → [DESIGN](../../DESIGN.md) → [AGENTS](../../AGENTS.md) → [ADR-0003](../adr/0003-m2-decisions-contracts.md) → [piano operativo](../progress.md) → documentazione di ricerca. I documenti di ricerca e l'ADR-0004 **proposto** supportano le decisioni ma **non** le promuovono automaticamente a `accepted for implementation` o `validated`.

Prima di Inc-1 l'utente deve autorizzare l'avvio operativo; l'eventuale ampliamento del contratto per limiter, ceiling true-peak o monitoring LUFS deve essere approvato prima del codice che ne dipende. Non aumentare il budget di otto voci senza ADR e non usare la sola verifica CACP come prova di qualità artistica. M2.1–M2.3 rimangono concluse secondo `docs/progress.md`, senza riaperture formali in questi documenti.
