# Runtime Reshape: Phase 8 Execution Plan

## Client Flow Resource Slice (2026-03-06)

### Current Scope
- Move the remaining client flow state out of `ClientRuntime` members and into explicit flecs-owned resources.
- Keep transport, prediction, and rendering internals where they are for now; this slice only changes ownership of runtime flow/control state.
- Close the pending disconnect-return gap so menu/disconnected status is preserved through the new resource-backed path.

### Assumptions
- `core::RuntimeState` remains the transitional flow struct for now, but the active source of truth becomes flecs resources rather than `ClientRuntime` data members.
- Local dedicated startup and runtime status/debug state can move into resources without changing transport or launcher abstractions.
- This slice does not remove `SceneManager`; it only makes the world own the data that currently drives it.

### Concrete File Touch List
- `docs/runtime-phase8-plan.md` (new)
- `docs/runtime-reshape-plan.md`
- `docs/context-current.md`
- `docs/multiplayer-architecture.md`
- `docs/multiplayer-runbook.md`
- `tests/CMakeLists.txt`
- `tests/sim/client_runtime_flow_resources.cpp` (new)
- `src/client/app/client_app.cpp`
- `src/client/modules/client_runtime_module.hpp`
- `src/client/runtime/client_runtime.hpp`
- `src/client/runtime/client_runtime.cpp`
- `src/client/runtime/runtime_resources.hpp` (new)
- `src/client/ui/ui_state.hpp`

### Acceptance Criteria
- The active client flow state is stored in flecs resources rather than `ClientRuntime` fields for:
  - runtime mode / splash / requested actions / disconnect reason
  - runtime status text and debug overlay toggle
  - local dedicated startup ownership/retry state
- Existing menu, join, local dedicated, singleplayer, and options flows still behave the same.
- Disconnect reasons persist through the resource-backed screen state and return-to-menu path.
- Validation gate passes:
  - `cmake --build --preset debug -j`
  - `ctest --preset debug`

### Status
- Client flow resource slice completed on 2026-03-06.

### Progress Update
- Completed work:
  - Added `runtime::ClientFlowState` and `runtime::LocalServerStartupState` as explicit flecs-owned resources on the client world.
  - Moved runtime mode, splash completion, requested actions, disconnect/status text, debug overlay toggle, and local dedicated startup ownership/retry state out of `ClientRuntime` members and into those resources.
  - Switched `ClientRuntime` to read/write the new world-owned resources through helper accessors instead of duplicating local control state.
  - Fixed the disconnect return path so returning from `Disconnected` to the menu preserves the disconnect reason as menu status text.
  - Added `test_sim_client_runtime_flow_resources` to cover boot, join failure, local-start join success, multiplayer disconnect, singleplayer exit, and menu-return status behavior.
- Changed files:
  - `docs/runtime-phase8-plan.md`
  - `docs/runtime-reshape-plan.md`
  - `docs/context-current.md`
  - `docs/multiplayer-architecture.md`
  - `docs/multiplayer-runbook.md`
  - `tests/CMakeLists.txt`
  - `tests/sim/client_runtime_flow_resources.cpp`
  - `src/client/app/client_app.cpp`
  - `src/client/modules/client_runtime_module.hpp`
  - `src/client/runtime/client_runtime.hpp`
  - `src/client/runtime/client_runtime.cpp`
  - `src/client/runtime/runtime_resources.hpp`
- Remaining risks/blockers:
  - `ClientRuntime` still owns transport/session mechanics, singleplayer stepping, and scene-manager bridging; this slice only moved control-state ownership.
  - Full manual GUI smoke for the menu-driven `Start Server`, `Singleplayer`, and `Options` flows is still pending.

### Validation
- `cmake --build --preset debug -j`
- `ctest --preset debug`
- Client startup sanity:
  - `timeout 2 ./build/debug/game_client --skip-splash`
