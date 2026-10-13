# AGENTS.md — CYMATICA

Operational rules for agents working in this repository. Updated 2026-10-13. Current milestone status: `docs/progress.md`.

`CYMATICA_Specifica_Agentica_Sviluppo.md` is authoritative for architecture, contracts, roadmap and technical decisions. `DESIGN.md` is authoritative for gameplay, audio-visual language, UX and accessibility. Do not duplicate either document here. If requirements conflict, resolve the conflict in both documents; examples are not executable schemas, benchmark results or approved tuning values.

## 1. Start here

Before changing files:

1. read the main specification;
2. read `DESIGN.md` when the task affects player-facing behavior;
3. inspect current code, tests, CI and recorded evidence; do not invent results or assume that historical test reports validate the current commit;
4. identify the active milestone and its acceptance criteria.

The project began at **Milestone 0**; the current milestone is recorded in `docs/progress.md` (M2 at this revision). Documentation changes do not advance implementation status. The minimal player contract `PlayerProfile_v1` was frozen in M1 (`docs/player_contract_m1.md`) for the dependent M3/M4 work; its empirical tuning and player-feel validation remain for M5/M7. An unresolved choice blocks only the work depending on it. Do not implement later-milestone work unless the user changes scope or the repository records that decision.

Prefer the smallest reversible change. Report unresolved ambiguity instead of inventing a permanent decision.

### 1.1 Milestone entry and open decisions

At the start of each milestone, and before a task that depends on an unresolved choice:

1. Review its deliverables, acceptance criteria, prerequisites and relevant open questions in both normative documents.
2. Identify which decisions are required now, which tasks depend on them and which decisions belong to later milestones. Do not resolve future choices merely for completeness.
3. Resolve required decisions before implementing dependent behavior. An authorized, bounded spike may investigate alternatives before choosing; label its assumptions as experimental and do not treat its results as an accepted contract automatically.
4. Record each resolved decision in the relevant normative document or an ADR referenced from it: decision ID, milestone, chosen option, rationale, alternatives considered, affected contracts, initial parameters with units, verification criteria and status. Keep one authoritative record instead of copying competing versions across documents.
5. Update affected task acceptance criteria and tests in the same change. When player-facing behavior changes, keep specification and design consistent.

Make routine, reversible implementation choices autonomously within the approved scope and document consequential ones. Ask the user only when a decision materially changes scope, established design, architecture or an explicit approval requirement, or when missing information prevents a justified choice. Pause only dependent work and continue independent authorized tasks. Do not repeatedly request approval already granted for the same scope.

Distinguish **deferred**, **proposed**, **accepted for implementation** and **validated** decisions. A proposed option is not an accepted requirement; an accepted initial value is not an experimentally validated value. Tuning may remain open, but implementation must use an explicit, versioned initial profile, not hidden defaults. Define how and when that profile will be evaluated.

M0 established the pinned toolchain, dependencies, test framework, documented setup and a single audio owner. `PlayerProfile_v1` is already `accepted for implementation` in `docs/player_contract_m1.md`; M3 must implement the relevant player primitives and M4 must validate against that profile. Changes during M5 require corresponding M4 validator regression checks.

## 2. Scope and dependencies

Keep runtime, offline tools, AI lab and shared formats decoupled as defined in the specification.

Do not add a dependency silently. Update `docs/dependencies.md`, build configuration and setup instructions in the same change. Pin fetched or vendored dependencies and record license and provenance.

Without explicit approval, do not introduce:

- ONNX Runtime, FFmpeg or stem-separation tooling;
- Android build infrastructure;
- a GUI editor or browser runtime;
- an external physics engine;
- Strudel as a distributable dependency;
- a replacement game engine/framework.

## 3. Build, realtime and generation

Use C++20 and CMake with out-of-source builds. Keep a clean checkout reproducible from documented commands.

Prefer small modules, explicit ownership, RAII, deterministic fixed-step updates and bounded work. Avoid hidden globals, unnecessary inheritance, broad rewrites and allocations in hot loops.

The miniaudio callback is a realtime path: no allocation, blocking locks, file I/O, logging, UI access, inference, planning or unbounded work. Keep audio telemetry and game-to-audio control as separate preallocated channels.

Use the canonical musical transport and its documented presentation mapping, fixed timestep for authoritative gameplay, and variable rendering with interpolation. Do not equate callback-generated frames with already-heard audio or discard authoritative ticks to hide overload.

Use explicit slot ownership for cross-thread snapshots. A trivially-copyable payload is not an atomic snapshot; do not use a naive seqlock over concurrently accessed non-atomic data. The game coordinator accepts plan revision/epoch and director state atomically. The audio event SPSC has exactly one producer.

Pure Seed uses fixed logical work budgets and stable ordering. Wall-clock deadline misses invalidate comparison eligibility; exact replay consumes accepted decisions. Never claim seed alone reproduces an adaptive run.

Validate the composed space-time hazard state, including swept transitions and dash resources. Unknown validation results never pass. Safety fallback is contextual; prevalidated in isolation is not enough.

Candidate retrieval (System 1, spec §11.7) only selects what System 2 examines; it never decides safety and never replaces the validator.

Procedural decisions must be reproducible from seed, policy version and decision trace. Keep random streams isolated. Apply hard fairness constraints before utility or novelty scoring, and never bypass the runtime safety guard.

Keep gameplay bullets, visual particles and shader effects separate. The shader is not the authoritative collision model.

## 4. Safety

Do not run destructive commands outside the repository, alter unrelated user files, install global software or download unpinned binaries/models without explicit approval.

Do not commit generated build output, caches, large binaries, credentials, private telemetry or ML models.

## 5. Verification

Run relevant builds and tests after implementation changes. For documentation-only changes, check links, examples, internal consistency and baseline provenance; report build/tests as not applicable when no build exists. Add focused tests for deterministic math, state transitions, data formats, pools, thread exchange, seed isolation and procedural constraints when applicable.

### 5.1 Milestone completion

Do not declare a milestone complete merely because its files exist or its implementation compiles. Check every mandatory acceptance criterion against recorded evidence:

- identify the criterion and the implementation/build or commit evaluated;
- record the relevant command or manual procedure, environment/profile, result and evidence location;
- distinguish passed, failed, not run and blocked checks; use not applicable only with a documented justification consistent with scope;
- include required runtime, hardware, audio, accessibility or human-playtest evidence when the criterion calls for it; automated tests cannot substitute for those checks;
- verify that decisions required by the milestone are resolved and reflected in code, tests and normative documents.

A missing or failed mandatory check leaves the milestone incomplete. Report the exact gap and next action; do not silently weaken acceptance criteria, change golden outputs to hide a regression or infer completion from a future milestone's plans. A change to agreed scope or acceptance criteria must be explicitly recorded with its rationale and any required user decision.

Maintain `docs/progress.md` as the current milestone status record: active milestone/subphase, entry decisions, acceptance evidence, build/commit provenance and remaining blockers. This record tracks execution; the specification remains authoritative for the roadmap. Update status only when supported by evidence, and do not advance to later work outside the authorized scope.

### 5.1.1 Independent review loop at milestone completion

Before declaring any milestone complete, committing its final closure, or advancing to the next milestone, the agent must execute an iterative independent review loop:

1. **Independent Review Execution:** Delegate a rigorous, critical audit of all milestone deliverables to an independent reviewer role/subagent. The review must independently examine five dimensions:
   - *Normative compliance:* Strict alignment with `CYMATICA_Specifica_Agentica_Sviluppo.md`, `DESIGN.md`, `AGENTS.md`, and relevant ADRs.
   - *Mathematical & algorithmic accuracy:* Analytical correctness of sample boundaries, rational timing, zero drift over extended runs, invariant golden vectors, and absence of tautological tests.
   - *Realtime thread safety & concurrency:* Lock-free/wait-free invariants, bounded loops (no unbounded spinning in realtime callbacks), cache-line isolation, explicit memory ordering, and zero dynamic heap allocations or blocking locks in the audio callback.
   - *Robustness & edge cases:* Signed/unsigned arithmetic safety, bounds checking, defensive input sanitization, error reporting, and zero compiler warnings (`/W4`).
   - *Clean verification:* Successful execution across all required build configurations (Clean Release, Debug, Headless, and interactive Smoke run).
2. **Defect Remediation:** If the review verdict is **NON OK** or identifies any defects, gaps, or normative divergences (`DIF-*`), all findings must be prioritized and resolved in code, contracts, and tests.
3. **Iterative Re-Review:** Repeat the independent review loop against the updated code.
4. **Completion Gate:** The milestone may be declared complete only when an independent review formally concludes with an **OK** verdict, zero remaining defects, zero compiler warnings, and 100% test pass rate across all target configurations.

### 5.1.2 Validazione in prima persona tramite CACP e capacità multimodali

Per ogni deliverable o modifica che influisca su audio, grafica, controlli o comportamento percettivo del giocatore, l'agente non deve affidarsi unicamente a unit test formali o asserzioni booleane isolate. L'agente deve sfruttare attivamente il CACP (**Cymatica Agent Control & Observation Plane**) per verificare in prima persona l'esperienza di gioco tramite le proprie capacità multimodali:

1. **Esecuzione scenari diagnostici:** eseguire il runner locale (`cymatica_game --cacp-run <scenario>`) o il collaudo dal vivo (`scripts/build.ps1 -Run -SmokeSeconds <N>`).
2. **Convalida quantitativa:** verificare la coerenza del manifest JSON generato (`artifacts/cacp/*_manifest.json`), inclusi tick, epoch, tolleranze di pitch/energia, e assenza di frame persi o sospensioni.
3. **Ispezione percettiva multimodale diretta:** acquisire ed esaminare direttamente i file binari audio (`.wav` via `AudioProbe`) e i frame grafici (`.png` via `VisualProbe` da M3) tramite gli strumenti multimodali a disposizione (`view_file`), formulando un giudizio qualitativo esplicito (purezza del timbro, assenza di click o distorsione, correttezza delle transizioni e del feedback visivo).
4. **Skill di riferimento:** seguire la procedura operativa codificata nella skill di progetto locale [`.agents/skills/cacp-testing/SKILL.md`](.agents/skills/cacp-testing/SKILL.md), aggiornandola ed estendendola progressivamente al rilascio di nuovi profili diagnostici.

### 5.2 Task handoff

Do not claim success if commands failed or were skipped. Report:

```text
Summary
Files changed
Build/test commands and results
Milestone impact
Decisions resolved or still open
Risks or follow-up
```
