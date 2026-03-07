# Runtime Reshape: Phase 6 Execution Plan

## Singleplayer Authoritative Slice (2026-03-06)

### Current Scope
- Replace the `Singleplayer` placeholder route with a real local authoritative sim path.
- Reuse the shared deterministic sim without transport or server process dependencies.
- Keep the first slice intentionally narrow: local player spawn, movement/jump, gameplay rendering, and menu return.

### Assumptions
- Singleplayer remains transport-free in this slice; no fake loopback networking is introduced.
- This slice reuses the current gameplay renderer and input capture path instead of building a separate singleplayer presentation stack.
- Persistence/save hooks remain out of scope; the goal is a playable local sandbox path, not save/load.

### Concrete File Touch List
- `docs/runtime-phase6-plan.md` (new)
- `docs/runtime-reshape-plan.md`
- `docs/context-current.md`
- `docs/multiplayer-architecture.md`
- `docs/multiplayer-runbook.md`
- `README.md`
- `REFACTORING.md`
- `CMakeLists.txt`
- `tests/CMakeLists.txt`
- `tests/sim/singleplayer_runtime.cpp` (new)
- `src/client/core/singleplayer_runtime.hpp` (new)
- `src/client/core/singleplayer_runtime.cpp` (new)
- `src/client/runtime/client_runtime.hpp`
- `src/client/runtime/client_runtime.cpp`
- `src/client/render/status_presenter.hpp`
- `src/client/systems/render_system.hpp`

### Acceptance Criteria
- Selecting `Singleplayer` transitions into `GameplaySingleplayer` without starting transport or a server process.
- The local player can move and jump using the shared sim rules.
- `Esc` returns from singleplayer gameplay to the main menu.
- Validation gate passes:
  - `cmake --build --preset debug -j`
  - `ctest --preset debug`

### Status
- Singleplayer authoritative slice completed on 2026-03-06.

### Progress Update
- Completed work:
  - Added `core::SingleplayerRuntime` as a transport-free local authoritative wrapper around `shared::game::GameState`.
  - Replaced the `Singleplayer` placeholder route with a real `GameplaySingleplayer` runtime path.
  - Reused the existing gameplay renderer for singleplayer so the local sandbox draws through the same world presentation path.
  - Added `test_sim_singleplayer_runtime` to cover local player spawn, movement, jump, and shutdown semantics.
- Changed files:
  - `docs/runtime-phase6-plan.md`
  - `docs/runtime-reshape-plan.md`
  - `docs/context-current.md`
  - `docs/multiplayer-architecture.md`
  - `docs/multiplayer-runbook.md`
  - `README.md`
  - `REFACTORING.md`
  - `CMakeLists.txt`
  - `tests/CMakeLists.txt`
  - `tests/sim/singleplayer_runtime.cpp`
  - `src/client/core/singleplayer_runtime.hpp`
  - `src/client/core/singleplayer_runtime.cpp`
  - `src/client/runtime/client_runtime.hpp`
  - `src/client/runtime/client_runtime.cpp`
  - `src/client/render/status_presenter.hpp`
  - `src/client/render/world_renderer.hpp`
  - `src/client/systems/render_system.hpp`
- Remaining risks/blockers:
  - Singleplayer ownership still lives inside the transitional `ClientRuntime` shell rather than explicit flecs resources/components.
  - The first slice is intentionally narrow: no save/load hooks, pause flow, or local world content beyond the player sandbox.
  - Follow-on architectural cleanup is still pending even though the menu/runtime slice is complete.

### Validation
- `cmake --build --preset debug -j`
- `ctest --preset debug`
- Client startup sanity:
  - `./build/debug/game_client --skip-splash`
