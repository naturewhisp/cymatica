# AGENTS.md — CYMATICA

Operational rules for agents working in this repository.

`CYMATICA_Specifica_Agentica_Sviluppo.md` is the source of truth for product scope, architecture, roadmap and technical decisions. Do not duplicate it here.

## 1. Start here

Before changing files:

1. read the main specification;
2. inspect the files relevant to the task;
3. identify the active milestone and its acceptance criteria.

The initial active milestone is **Milestone 0**. Do not implement later-milestone work unless the user explicitly changes scope or the repository records that decision.

Prefer the smallest reversible change. Report unresolved ambiguity instead of inventing a permanent decision.

## 2. Scope and dependencies

Keep runtime, offline tools and shared formats decoupled as defined in the specification.

Do not add a dependency silently. Update `docs/dependencies.md`, build configuration and setup instructions in the same change. Pin every fetched or vendored dependency to a version, tag or commit and record its license and provenance.

Without explicit approval, do not introduce:

- ONNX Runtime, FFmpeg or stem-separation tooling;
- Android build infrastructure;
- a GUI editor or browser runtime;
- an external physics engine;
- Strudel as a distributable dependency;
- a replacement game engine/framework.

## 3. Build and code

Use C++20 and CMake with out-of-source builds. Keep a clean checkout reproducible from documented commands.

Prefer small modules, explicit ownership, RAII, deterministic updates and bounded work. Avoid hidden globals, unnecessary inheritance, broad rewrites and allocations in hot loops.

The miniaudio callback is a real-time path: no allocation, blocking locks, file I/O, logging, UI access, inference or unbounded work. Keep audio telemetry and game-to-audio control as separate unidirectional, preallocated channels. Use a bounded SPSC queue, seqlock or triple buffer; never rely on an unsafe two-slot swap.

Run gameplay at a fixed timestep, use the audio clock as the authority for musical event timing, and keep rendering variable with interpolation where needed.

Keep gameplay bullets, visual particles and shader effects separate. The shader is not the authoritative collision model.

## 4. Safety

Do not run destructive commands outside the repository, alter unrelated user files, install global software or download unpinned binaries/models without explicit approval.

Do not commit generated build output, caches, large binaries, credentials or ML models.

## 5. Verification

Run the relevant build and tests after changes. Add focused tests for deterministic math, state transitions, data formats, pools and thread-exchange code when applicable.

Do not claim success if commands failed or were skipped. Report:

```text
Summary
Files changed
Build/test commands and results
Milestone impact
Risks or follow-up
```
