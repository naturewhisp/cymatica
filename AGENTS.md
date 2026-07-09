# AGENTS.md — CYMATICA

Operational rules for agentic IDEs working on the CYMATICA repository.

The product, architecture, roadmap and technical rationale live in `CYMATICA_Specifica_Agentica_Sviluppo.md`. This file only defines how agents must operate in the repository.

---

## 1. Required startup

Before changing code, read:

1. `CYMATICA_Specifica_Agentica_Sviluppo.md`
2. `AGENTS.md`
3. existing repository files relevant to the requested change

Then identify the active milestone and keep changes aligned with it.

When uncertain, prefer the smallest reversible change and report the ambiguity.

---

## 2. Current scope guard

The initial focus is Milestone 0/1: Windows vertical slice with procedural music, C++20/CMake, raylib, miniaudio, Chladni rendering, Seed movement, Quantum Dash, Dissonance, bullets and custom particles.

Do not introduce these without explicit approval:

- ONNX Runtime
- FFmpeg
- stem separation
- imported music pipeline
- GUI level editor
- Android build pipeline
- external physics engine
- Strudel runtime dependency
- Unity, Unreal, Godot, Flutter or browser runtime for core gameplay

---

## 3. Repository and target policy

Use a modular monorepo. Keep game runtime, tools and shared code separate.

Expected target direction:

```text
cymatica_shared
cymatica_engine
cymatica_game
cymatica_tool_cli
```

The game runtime must not depend on offline toolchain code, ML tooling, FFmpeg, ONNX, Strudel packages or editor UI code.

`cymatica_studio` is future scope unless explicitly requested.

---

## 4. Dependency policy

Do not add dependencies silently.

Every dependency must be documented in `docs/dependencies.md` with:

- purpose
- version/tag/commit
- license
- acquisition method
- runtime/tool/test/research scope
- Windows status
- future Android status, if relevant
- risks/notes

Allowed acquisition classes:

- pinned CMake `FetchContent`
- vendored small source/single-header libraries
- documented manual/system install
- research-only dependency isolated outside runtime builds

---

## 5. Build policy

Use C++20 and CMake.

Prefer out-of-source builds:

```bash
cmake -S . -B build
cmake --build build --config Debug
```

If MSVC or Visual Studio-specific tooling is required, document the exact commands in `docs/build.md`.

Keep build instructions reproducible from a clean checkout.

---

## 6. Real-time audio rules

The miniaudio callback is a real-time path.

Never do this inside the audio callback:

- heap allocation/free
- blocking locks
- file I/O
- logging
- UI/render access
- ML inference
- expensive or unbounded work

Allowed:

- generate procedural samples
- mix preloaded buffers
- update preallocated DSP state
- write compact telemetry through a lock-free or double-buffered exchange

The render/game thread may read audio telemetry, but must not block the audio thread.

---

## 7. Gameplay/rendering boundaries

Keep these systems separate:

- bullets: gameplay entities with collision, damage, graze and deterministic behavior
- particles: visual resonance artifacts with pooling, budgets and optional audio/cymatic forces
- Chladni shader: visual/field representation driven by explicit uniforms

Do not use an external physics engine in Milestone 1.

---

## 8. Coding rules

Prefer:

- small focused files
- explicit ownership
- RAII
- deterministic updates
- data-oriented hot paths
- object pools for bullets/particles
- tests for math, formats and state transitions

Avoid:

- large inheritance hierarchies
- hidden mutable globals
- allocations in hot loops
- blocking synchronization in real-time paths
- broad rewrites not required by the task

Keep style consistent within each module.

---

## 9. Tests and verification

Add tests where practical for:

- Euclidean rhythm generation
- modal quantization
- Chladni math helpers
- audio frame exchange
- object pools
- particle lifetime/budgets
- bullet collision/graze
- Quantum Dash landing
- level format validation

Report exactly which build/test commands were run. Do not hide failed or skipped tests.

---

## 10. Documentation update rules

Update docs in the same change when behavior changes:

- `docs/build.md` for build/setup changes
- `docs/dependencies.md` for dependency changes
- `docs/architecture.md` for module/API changes
- `docs/level-format.md` for data format changes
- roadmap/spec only when milestone scope changes

Do not duplicate the main analysis/roadmap inside this file.

---

## 11. Agent report format

When reporting completed work, use:

```text
Summary
- What changed.

Files changed
- path: reason.

Build/test
- Commands run and result.

Milestone impact
- Which milestone this advances.

Risks/follow-up
- Remaining issues or decisions needed.
```

