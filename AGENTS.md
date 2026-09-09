# AGENTS.md — CYMATICA

Operational rules for agents working in this repository.

`CYMATICA_Specifica_Agentica_Sviluppo.md` is authoritative for architecture, contracts, roadmap and technical decisions. `DESIGN.md` is authoritative for gameplay, audio-visual language, UX and accessibility. Do not duplicate either document here.

## 1. Start here

Before changing files:

1. read the main specification;
2. read `DESIGN.md` when the task affects player-facing behavior;
3. inspect the relevant code and tests;
4. identify the active milestone and its acceptance criteria.

The initial active milestone is **Milestone 0**. Do not implement later-milestone work unless the user changes scope or the repository records that decision.

Prefer the smallest reversible change. Report unresolved ambiguity instead of inventing a permanent decision.

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

Use the audio clock for musical timing, fixed timestep for authoritative gameplay, and variable rendering with interpolation where needed.

Procedural decisions must be reproducible from seed, policy version and decision trace. Keep random streams isolated. Apply hard fairness constraints before utility or novelty scoring, and never bypass the runtime safety guard.

Keep gameplay bullets, visual particles and shader effects separate. The shader is not the authoritative collision model.

## 4. Safety

Do not run destructive commands outside the repository, alter unrelated user files, install global software or download unpinned binaries/models without explicit approval.

Do not commit generated build output, caches, large binaries, credentials, private telemetry or ML models.

## 5. Verification

Run relevant builds and tests after changes. Add focused tests for deterministic math, state transitions, data formats, pools, thread exchange, seed isolation and procedural constraints when applicable.

Do not claim success if commands failed or were skipped. Report:

```text
Summary
Files changed
Build/test commands and results
Milestone impact
Risks or follow-up
```
