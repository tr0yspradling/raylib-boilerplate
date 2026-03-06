# AGENTS

Single source of truth for OpenAI agents and Claude Code. Link or symlink `CLAUDE.md` to this file.

---

## Build, Run, Test

**Preferred debug profile (`cmake-build-debug`)**

- Configure: `cmake -S . -B cmake-build-debug -DCMAKE_BUILD_TYPE=Debug`
- Build: `cmake --build cmake-build-debug -j`
- Run client: `./cmake-build-debug/game_client`
- Run server: `./cmake-build-debug/game_server`
- Test: `ctest --test-dir cmake-build-debug --output-on-failure`
- Clean: `cmake --build cmake-build-debug --target clean`

**Canonical out-of-source alternative (`build`)**

- Configure: `cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug`
- Build: `cmake --build build -j`
- Run client: `./build/game_client`
- Run server: `./build/game_server`
- Test: `ctest --test-dir build --output-on-failure`

**Compatibility target**

- `raylib_boilerplate` is a compatibility **build target alias** to `game_client`.
- It must not produce a second client runtime binary.

---

## Strict Development Cycle (Required)

This cycle is mandatory for every phase and every non-trivial change.

1. **Context Sync (before coding)**
   - Read and align with:
     - `docs/context-current.md`
     - `docs/runtime-reshape-plan.md`
     - active phase doc(s), e.g. `docs/runtime-phase1-plan.md`
     - `docs/multiplayer-architecture.md`
     - `docs/multiplayer-runbook.md`

2. **Phase Plan Update (before coding)**
   - Update the active phase doc with:
     - current scope
     - assumptions
     - concrete file touch list
     - acceptance criteria

3. **Implement in Small Compilable Steps**
   - Keep each step buildable.
   - Prefer incremental refactors over large rewrites.

4. **Validation Gate (after each meaningful step)**
   - Build and run relevant tests.
   - Fix regressions before continuing.

5. **Plan Progress Update (continuous)**
   - After each meaningful change, update active phase doc status:
     - completed work
     - changed files
     - remaining risks/blockers

6. **Context Refresh (end of phase slice)**
   - Update `docs/context-current.md` with:
     - latest completed phase/slice
     - current architecture state
     - open risks
     - next recommended step

7. **Operational Docs Refresh (when affected)**
   - Update `docs/multiplayer-architecture.md` for structural/ownership changes.
   - Update `docs/multiplayer-runbook.md` for build/run/operational workflow changes.

8. **Commit (required)**
   - Commit each completed phase slice with a focused message.
   - Do not batch unrelated work in one commit.

9. **Push (required)**
   - Push after each committed phase slice.
   - If push is blocked by environment/auth/remote constraints, record the exact blocker in `docs/context-current.md` and in the handoff.

A phase is not complete until steps 1-9 are satisfied.

---

## Living Context Files

Maintain these as living documents:

- `docs/context-current.md` (current status and next actions)
- `docs/runtime-reshape-plan.md` (multi-phase roadmap)
- `docs/runtime-phase*.md` (phase execution details)
- `docs/multiplayer-architecture.md` (actual architecture)
- `docs/multiplayer-runbook.md` (actual operator workflow)

---

## Project Structure

- Client runtime: `src/client/`
  - `core/`, `input/`, `physics/`, `scenes/`, `systems/`, `ui/`, `components/`
- Dedicated server: `src/server/`
  - `config/`, `game_server.*`, `world_persistence.*`
- Shared simulation/network: `src/shared/`
  - `game/` (pure C++, no raylib)
  - `net/` (protocol, serializer, transport abstraction + GNS impl)
- Tests: `tests/` (`net/`, `sim/`)
- Documentation: `docs/`
- Dependencies: `external/`

---

## Architecture Overview

- Dedicated authoritative multiplayer server is primary.
- Server owns gameplay truth; client sends input/intent.
- Shared gameplay code under `src/shared/game` must remain raylib-free.
- Transport is Valve GameNetworkingSockets behind `shared/net/transport.hpp` abstraction.
- Client-only rendering/input uses raylib-cpp.

---

## Code Style and Naming

- C++ standard: C++23 (project default).
- Format: `.clang-format`.
- New/renamed files: prefer `snake_case`.
- Types: `PascalCase`.
- Functions: `CamelCase` (existing codebase convention).
- Variables: `lowerCamelCase`.
- Config fields/constants: `snake_case`.

---

## Testing Expectations

- Prefer automated checks via CTest targets under `tests/`.
- Minimum phase validation:
  - `cmake --build cmake-build-debug -j`
  - `ctest --test-dir cmake-build-debug --output-on-failure`
- For runtime-flow changes, also perform brief manual smoke checks.

---

## Contribution

- Commits: imperative mood, concise subject, optional scope.
- Keep diffs focused and logically grouped.
- Update docs in the same change when behavior/architecture/workflow changes.

---

## Platform Notes

- Ensure raylib dev files are installed locally.
- Ubuntu example: `sudo apt install libraylib-dev`.
- macOS example: `brew install raylib`.
- Linux links: `m`, `pthread`, `GL`, `dl`, `X11`.
