# Runtime Reshape: Phase 10 Execution Plan

## Client Multiplayer Session Service Slice (2026-03-06)

### Current Scope
- Extract the multiplayer transport/session orchestration out of `ClientRuntime` into a dedicated runtime service under `src/client/runtime/`.
- Keep `runtime::ClientSessionState` as the flecs-owned source of truth for live multiplayer state.
- Leave the transitional `SceneManager` bridge, singleplayer stepping, and options persistence inside `ClientRuntime` for the following slice.

### Assumptions
- `net::TransportGns` should move with the new service because it is multiplayer infrastructure, not top-level app/runtime flow.
- The new service can operate on `ClientFlowState`, `LocalServerStartupState`, and `ClientSessionState` passed in from `ClientRuntime` without owning the flecs world directly.
- This slice is architectural only; it should preserve current join/start-local/multiplayer behavior rather than redesign it.

### Concrete File Touch List
- `docs/runtime-phase10-plan.md` (new)
- `docs/runtime-reshape-plan.md`
- `docs/context-current.md`
- `docs/multiplayer-architecture.md`
- `docs/multiplayer-runbook.md`
- `CMakeLists.txt`
- `src/client/runtime/client_runtime.hpp`
- `src/client/runtime/client_runtime.cpp`
- `src/client/runtime/multiplayer_session_service.hpp` (new)
- `src/client/runtime/multiplayer_session_service.cpp` (new)
- `tests/CMakeLists.txt` (if a focused test is added)
- `tests/sim/*` (if a focused test is added)

### Acceptance Criteria
- Transport initialization, connect/disconnect handling, packet decode/dispatch, and multiplayer send helpers live outside `ClientRuntime`.
- `ClientRuntime` remains responsible for top-level flow orchestration, UI/screen behavior, and singleplayer-only behavior.
- Existing join, local dedicated, multiplayer gameplay, and disconnect flows still behave the same.
- Validation gate passes:
  - `cmake --build --preset debug -j`
  - `ctest --preset debug`
  - `timeout 2 ./build/debug/game_client --skip-splash`

### Status
- Completed.

### Progress Update
- Completed work:
  - Added `runtime::MultiplayerSessionService` as the dedicated client-side boundary for transport bootstrap, connect/disconnect handling, packet decode/dispatch, multiplayer cadence sends, and connection metrics.
  - Removed direct transport/message-handling ownership from `ClientRuntime`; it now orchestrates around the service and the existing flecs-owned flow/session resources.
  - Added `test_sim_client_multiplayer_session_service` to cover the extracted connect/event/ClientHello path with a fake transport.
- Changed files:
  - `docs/runtime-phase10-plan.md`
  - `docs/runtime-reshape-plan.md`
  - `docs/context-current.md`
  - `docs/multiplayer-architecture.md`
  - `docs/multiplayer-runbook.md`
  - `CMakeLists.txt`
  - `src/client/runtime/client_runtime.hpp`
  - `src/client/runtime/client_runtime.cpp`
  - `src/client/runtime/multiplayer_session_service.hpp`
  - `src/client/runtime/multiplayer_session_service.cpp`
  - `tests/CMakeLists.txt`
  - `tests/sim/client_multiplayer_session_service.cpp`
- Remaining risks/blockers:
  - The transitional `RuntimeState + SceneManager` bridge is still active inside `ClientRuntime`.
  - Singleplayer stepping and options persistence still live inside `ClientRuntime` and should move behind clearer services/resources in the next slice.

### Validation
- `cmake --build --preset debug -j`
- `ctest --preset debug`
- `timeout 2 ./build/debug/game_client --skip-splash`
