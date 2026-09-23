# AGENTS.md — CYMATICA

Operational rules for agents working in this repository. Updated 2026-09-23.

`CYMATICA_Specifica_Agentica_Sviluppo.md` is authoritative for architecture, contracts, roadmap and technical decisions. `DESIGN.md` is authoritative for gameplay, audio-visual language, UX and accessibility. Do not duplicate either document here. If requirements conflict, resolve the conflict in both documents; examples are not executable schemas, benchmark results or approved tuning values.

## 1. Start here

Before changing files:

1. read the main specification;
2. read `DESIGN.md` when the task affects player-facing behavior;
3. inspect the relevant code and tests if present; the reviewed baseline is documentation-only, so do not invent build/test results;
4. identify the active milestone and its acceptance criteria.

The initial active milestone is **Milestone 0**. Documentation changes do not advance implementation status. Before M3/M4, freeze the minimal player contract required by the validator; M5 completes it. An unresolved choice blocks only tasks depending on that choice, not unrelated M0 work. Do not implement later-milestone work unless the user changes scope or the repository records that decision.

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

M0 includes choosing and pinning the toolchain, dependencies and test framework, documenting setup and establishing a single audio owner. Do not block M0 on later gameplay decisions. Freeze the minimal movement/dash contract before dependent M3 gameplay and M4 validation; changes during M5 require corresponding validator regression checks.

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

Keep a concise milestone status record in the repository, initially `docs/progress.md` when M0 creates it: active milestone, entry decisions, acceptance evidence and remaining blockers. This record tracks execution; the specification remains authoritative for the roadmap. Until the record exists, M0 is the initial active milestone. Update status only when supported by evidence, and do not advance to later work outside the authorized scope.

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
