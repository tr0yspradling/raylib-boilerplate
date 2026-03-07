# Runtime Reshape: Phase 9 Execution Plan

## Client Session Resource Slice (2026-03-06)

### Current Scope
- Move the remaining multiplayer session/prediction/presentation state out of `ClientRuntime` members and into an explicit flecs-owned client-world resource.
- Keep the live transport object, window/app shell, launcher abstraction, and singleplayer runtime local for now.
- Preserve the current client phases and gameplay/network behavior while reducing `ClientRuntime` down toward orchestration only.

### Assumptions
- `net::TransportGns` stays as a `ClientRuntime` member in this slice because it is infrastructure, not shared session state.
- The new session resource can safely own prediction buffers, remote-player interpolation state, chunk cache state, and handshake/tick metadata.
- This slice does not remove `SceneManager`; it narrows `ClientRuntime` and makes more of its state world-visible first.

### Concrete File Touch List
- `docs/runtime-phase9-plan.md` (new)
- `docs/runtime-reshape-plan.md`
- `docs/context-current.md`
- `docs/multiplayer-architecture.md`
- `docs/multiplayer-runbook.md`
- `tests/CMakeLists.txt`
- `tests/sim/client_session_resources.cpp` (new)
- `src/client/app/client_app.cpp`
- `src/client/modules/client_runtime_module.hpp`
- `src/client/runtime/client_runtime.hpp`
- `src/client/runtime/client_runtime.cpp`
- `src/client/runtime/runtime_resources.hpp`

### Acceptance Criteria
- The active multiplayer session state is stored on the client flecs world rather than `ClientRuntime` fields for:
  - connect/welcome/connection-handle state
  - client/server tick counters and prediction buffers
  - local player predicted state and remote-player interpolation state
  - chunk cache/resync bookkeeping and debug counters
  - ping/chunk-interest cadence metadata
- Existing join, local dedicated, multiplayer gameplay, and singleplayer flows still behave the same.
- Validation gate passes:
  - `cmake --build --preset debug -j`
  - `ctest --preset debug`

### Status
- Completed.

### Progress Update
- Completed work:
  - Added `runtime::ClientSessionState` as a flecs-managed client-world resource for multiplayer connection state, prediction buffers, interpolation state, chunk cache state, and cadence/debug metadata.
  - Moved the remaining multiplayer session fields out of `ClientRuntime` and onto the client world while keeping `net::TransportGns` local infrastructure.
  - Seeded/registered the new session resource through the client app/runtime module path and added reset coverage in `test_sim_client_session_resources`.
- Changed files:
  - `docs/runtime-phase9-plan.md`
  - `docs/runtime-reshape-plan.md`
  - `docs/context-current.md`
  - `docs/multiplayer-architecture.md`
  - `src/client/app/client_app.cpp`
  - `src/client/modules/client_runtime_module.hpp`
  - `src/client/runtime/client_runtime.hpp`
  - `src/client/runtime/client_runtime.cpp`
  - `src/client/runtime/runtime_resources.hpp`
  - `tests/CMakeLists.txt`
  - `tests/sim/client_session_resources.cpp`
- Remaining risks/blockers:
  - Transport ownership, message polling, and most multiplayer orchestration still live inside transitional `ClientRuntime`, even though their mutable session state is now world-owned.
  - Manual GUI smoke for the menu-driven `Start Server`, `Singleplayer`, and `Options` flows is still pending.

### Validation
- `cmake --build --preset debug -j`
- `ctest --preset debug`
- `timeout 2 ./build/debug/game_client --skip-splash`
