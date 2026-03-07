# Runtime Reshape: Phase 7 Execution Plan

## Options and Client Preferences Slice (2026-03-06)

### Current Scope
- Replace the `Options` placeholder route with a real options screen on the existing UI/document path.
- Add client preference persistence for the planned user-facing settings.
- Apply settings at runtime where the current shell can do so safely without another architecture rewrite.

### Assumptions
- The first options slice uses the current in-process UI document flow; it does not introduce a separate settings subsystem.
- Preferences are stored in a simple local config file under the project working directory for now.
- CLI flags remain startup-only overrides and should still win over persisted values when explicitly provided.

### Concrete File Touch List
- `docs/runtime-phase7-plan.md` (new)
- `docs/runtime-reshape-plan.md`
- `docs/context-current.md`
- `docs/multiplayer-architecture.md`
- `docs/multiplayer-runbook.md`
- `README.md`
- `REFACTORING.md`
- `CMakeLists.txt`
- `tests/CMakeLists.txt`
- `tests/sim/client_config_file.cpp` (new)
- `src/client/core/client_config.hpp`
- `src/client/core/client_args.hpp`
- `src/client/core/config.hpp`
- `src/client/core/config.cpp` (new)
- `src/client/client_entry.cpp`
- `src/client/ui/ui_state.hpp`
- `src/client/ui/ui_document.hpp`
- `src/client/runtime/client_runtime.hpp`
- `src/client/runtime/client_runtime.cpp`
- `src/client/render/status_presenter.hpp`

### Acceptance Criteria
- Selecting `Options` opens a real editable options screen with keyboard/gamepad and mouse support.
- Persisted values load on the next launch unless explicitly overridden by CLI flags.
- Settings that can safely change live in the current shell apply immediately:
  - player name / default host / default port for future flows
  - target FPS
  - window width / height
  - interpolation delay
  - debug overlay default
- Validation gate passes:
  - `cmake --build --preset debug -j`
  - `ctest --preset debug`

### Status
- Options and client preferences slice completed on 2026-03-06.

### Progress Update
- Completed work:
  - Added persisted client preference load/save helpers in `client/core/config.*` with a concrete path at `client_data/client.cfg`.
  - Updated client startup so saved prefs load first and explicit CLI args override them.
  - Added a real `Options` screen on the existing UI/document path with keyboard/gamepad and mouse interaction.
  - Added live runtime application for player name, default host/port, target FPS, window size, interpolation delay, and debug overlay default.
  - Added `test_sim_client_config_file` to cover client preference persistence.
- Changed files:
  - `docs/runtime-phase7-plan.md`
  - `docs/runtime-reshape-plan.md`
  - `docs/context-current.md`
  - `docs/multiplayer-architecture.md`
  - `docs/multiplayer-runbook.md`
  - `README.md`
  - `REFACTORING.md`
  - `CMakeLists.txt`
  - `tests/CMakeLists.txt`
  - `tests/sim/client_args.cpp`
  - `tests/sim/client_config_file.cpp`
  - `src/client/core/client_config.hpp`
  - `src/client/core/client_args.hpp`
  - `src/client/core/config.hpp`
  - `src/client/core/config.cpp`
  - `src/client/client_entry.cpp`
  - `src/client/ui/ui_state.hpp`
  - `src/client/ui/ui_renderer.hpp`
  - `src/client/runtime/client_runtime.hpp`
  - `src/client/runtime/client_runtime.cpp`
  - `src/client/render/status_presenter.hpp`
  - `src/client/systems/render_system.hpp`
- Remaining risks/blockers:
  - Options persistence/state still lives inside the transitional `ClientRuntime` shell rather than explicit flecs resources/components.
  - Full manual GUI coverage of edit/save/reload through the options screen is still pending; current validation covers automated persistence tests and client startup sanity.
  - The deeper remaining planned work is architectural: moving runtime-owned state into flecs resources and tightening transition ownership.

### Validation
- `cmake --build --preset debug -j`
- `ctest --preset debug`
- Client startup sanity:
  - `./build/debug/game_client --skip-splash`
