# Runtime Reshape: Phase 12 Execution Plan

## Client Singleplayer Session Service Slice (2026-03-07)

### Current Scope
- Extract singleplayer runtime ownership and stepping out of `ClientRuntime` into a dedicated runtime service under `src/client/runtime/`.
- Keep `runtime::ClientSessionState` as the published state contract consumed by rendering, debug, and runtime flow.
- Leave options persistence/application in `ClientRuntime` for the following slice.

### Assumptions
- `core::SingleplayerRuntime` remains the low-level transport-free sim wrapper; this slice is about moving ownership and session publication behind a cleaner runtime boundary.
- The new service can mutate `ClientSessionState` directly without owning the flecs world.
- This slice is architectural only and should preserve the current `Singleplayer` behavior rather than expand feature scope.

### Concrete File Touch List
- `docs/runtime-phase12-plan.md` (new)
- `docs/runtime-reshape-plan.md`
- `docs/context-current.md`
- `docs/multiplayer-architecture.md`
- `docs/multiplayer-runbook.md`
- `CMakeLists.txt`
- `src/client/runtime/client_runtime.hpp`
- `src/client/runtime/client_runtime.cpp`
- `src/client/runtime/singleplayer_session_service.hpp` (new)
- `src/client/runtime/singleplayer_session_service.cpp` (new)
- `tests/CMakeLists.txt`
- `tests/sim/singleplayer_session_service.cpp` (new)

### Acceptance Criteria
- Singleplayer start, stop, step, and session-state publication live outside `ClientRuntime`.
- `ClientRuntime` remains responsible for top-level flow orchestration and shared UI behavior, not low-level singleplayer simulation ownership.
- Existing singleplayer flow still behaves the same and continues to publish gameplay state through `ClientSessionState`.
- Validation gate passes:
  - `cmake --build --preset debug -j`
  - `ctest --preset debug`
  - `timeout 2 ./build/debug/game_client --skip-splash`

### Status
- Completed.

### Progress Update
- Completed work:
  - Added `runtime::SingleplayerSessionService` as the dedicated boundary for singleplayer start/stop/step behavior and session-state publication.
  - Removed direct `SingleplayerRuntime` ownership from `ClientRuntime`; it now delegates the singleplayer path through the service and existing `ClientSessionState`.
  - Added `test_sim_singleplayer_session_service` to cover the extracted singleplayer start/step/stop behavior.
- Changed files:
  - `docs/runtime-phase12-plan.md`
  - `docs/runtime-reshape-plan.md`
  - `docs/context-current.md`
  - `docs/multiplayer-architecture.md`
  - `docs/multiplayer-runbook.md`
  - `CMakeLists.txt`
  - `src/client/runtime/client_runtime.hpp`
  - `src/client/runtime/client_runtime.cpp`
  - `src/client/runtime/singleplayer_session_service.hpp`
  - `src/client/runtime/singleplayer_session_service.cpp`
  - `tests/CMakeLists.txt`
  - `tests/sim/singleplayer_session_service.cpp`
- Remaining risks/blockers:
  - Options persistence/application still lives inside `ClientRuntime` and should move behind a dedicated service in the next slice.
  - Manual GUI smoke for `Start Server`, `Singleplayer`, and `Options` remains pending.

### Validation
- `cmake --build --preset debug -j`
- `ctest --preset debug`
- `timeout 2 ./build/debug/game_client --skip-splash`
