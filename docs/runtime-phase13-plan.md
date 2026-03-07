# Runtime Reshape: Phase 13 Execution Plan

## Client Options Service Slice (2026-03-07)

### Current Scope
- Extract options validation, config mutation, persistence, and live-safe runtime application out of `ClientRuntime` into a dedicated service under `src/client/runtime/`.
- Keep `ui::OptionsScreenState` and other UI resources flecs-managed on the client world.
- Leave the pending manual GUI smoke work for the follow-on polish phase.

### Assumptions
- Options save/apply behavior can be isolated without changing the current screen UX or config-file format.
- The new service should not depend directly on a live raylib window; live window updates can be injected through a callback from `ClientRuntime`.
- This slice is architectural only and should preserve the current `Options` behavior and CLI-override expectations.

### Concrete File Touch List
- `docs/runtime-phase13-plan.md` (new)
- `docs/runtime-reshape-plan.md`
- `docs/context-current.md`
- `docs/multiplayer-architecture.md`
- `docs/multiplayer-runbook.md`
- `CMakeLists.txt` (if a new runtime file is added)
- `src/client/runtime/client_runtime.hpp`
- `src/client/runtime/client_runtime.cpp`
- `src/client/runtime/options_service.hpp` (new)
- `src/client/runtime/options_service.cpp` (new)
- `tests/CMakeLists.txt`
- `tests/sim/options_service.cpp` (new)

### Acceptance Criteria
- `ClientRuntime` no longer owns the low-level options validation/persistence/application implementation directly.
- Saving options still updates `ClientConfig`, resets join defaults, applies live-safe settings, and persists the same config file format.
- Validation gate passes:
  - `cmake --build --preset debug -j`
  - `ctest --preset debug`
  - `timeout 2 ./build/debug/game_client --skip-splash`

### Status
- Completed.

### Progress Update
- Completed work:
  - Added `runtime::OptionsService` as the dedicated boundary for options validation, config mutation, persistence, join-default refresh, and live-safe window setting application.
  - Removed direct options save/apply ownership from `ClientRuntime`; it now delegates saved-options behavior through the service.
  - Added `test_sim_options_service` to cover the extracted options-service behavior, including persistence and validation failures.
- Changed files:
  - `docs/runtime-phase13-plan.md`
  - `docs/runtime-reshape-plan.md`
  - `docs/context-current.md`
  - `docs/multiplayer-architecture.md`
  - `docs/multiplayer-runbook.md`
  - `CMakeLists.txt`
  - `src/client/runtime/client_runtime.hpp`
  - `src/client/runtime/client_runtime.cpp`
  - `src/client/runtime/options_service.hpp`
  - `src/client/runtime/options_service.cpp`
  - `tests/CMakeLists.txt`
  - `tests/sim/options_service.cpp`
- Remaining risks/blockers:
  - The remaining planned work is manual GUI smoke/polish rather than another architectural service extraction.
  - Full hands-on validation for the menu-driven `Start Server`, `Singleplayer`, and `Options` paths is still pending.

### Validation
- `cmake --build --preset debug -j`
- `ctest --preset debug`
- `timeout 2 ./build/debug/game_client --skip-splash`
