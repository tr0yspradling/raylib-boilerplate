# Runtime Reshape: Phase 4 Execution Plan

## Foundation Slice (2026-03-06): Flecs Runtime Composition Root

### Current Scope
- Make flecs the active composition root for both `game_client` and `game_server`.
- Add explicit flecs runtime phases/modules for client and server without changing the shared deterministic sim model.
- Shrink `GameClient` and `GameServer` into thin shells backed by dedicated runtime service classes.
- Preserve the current gameplay/network/menu behavior for this slice; mouse-first UI redesign and deeper module decomposition remain follow-up work.

### Assumptions
- `src/shared/game/` remains plain deterministic C++ and is not converted to ECS storage in this slice.
- Existing protocol/transport behavior remains unchanged.
- Current menu/join-form UX remains functionally intact while the composition root changes underneath it.
- Vendored `flecs` is the active ECS dependency for both client and server targets, not just the legacy sample path.

### Concrete File Touch List
- `docs/runtime-phase4-plan.md` (new)
- `docs/runtime-reshape-plan.md`
- `docs/context-current.md`
- `docs/multiplayer-architecture.md`
- `docs/multiplayer-runbook.md`
- `README.md`
- `REFACTORING.md`
- `CMakeLists.txt`
- `src/client/app/client_app.hpp` (new)
- `src/client/app/client_app.cpp` (new)
- `src/client/modules/client_phases.hpp` (new)
- `src/client/modules/client_runtime_module.hpp` (new)
- `src/client/runtime/client_runtime.hpp` (new)
- `src/client/runtime/client_runtime.cpp` (new)
- `src/client/game_client.hpp`
- `src/client/game_client.cpp`
- `src/server/app/server_app.hpp` (new)
- `src/server/app/server_app.cpp` (new)
- `src/server/modules/server_phases.hpp` (new)
- `src/server/modules/server_runtime_module.hpp` (new)
- `src/server/runtime/server_runtime.hpp` (new)
- `src/server/runtime/server_runtime.cpp` (new)
- `src/server/game_server.hpp`
- `src/server/game_server.cpp`
- `tests/CMakeLists.txt`
- `tests/sim/client_pipeline.cpp` (new)
- `tests/sim/server_pipeline.cpp` (new)

### Acceptance Criteria
- `game_client` boots through a `flecs::world` and explicit client runtime phases.
- `game_server` runs through a `flecs::world` and explicit server runtime phases.
- `GameClient` and `GameServer` act as thin shells delegating runtime work to dedicated services.
- Existing client menu/join flow and multiplayer loop still build and test cleanly.
- Added tests verify client and server phase ordering through flecs worlds.
- Validation gate passes:
  - `cmake --build --preset debug -j`
  - `ctest --preset debug`

### Status
- Foundation slice completed on 2026-03-06. Follow-on Phase 4 slices remain in progress.

### Progress Update
- Completed work:
  - Added `ClientApp` and `ServerApp` as the active composition roots for `game_client` and `game_server`.
  - Moved heavyweight client/server runtime logic into dedicated `runtime/` services while keeping `GameClient` and `GameServer` as thin shells.
  - Added explicit flecs phase registration and runtime modules for both runtimes.
  - Promoted flecs to an active dependency for the primary client/server build path, not only the legacy sample path.
  - Added world-level tests validating client and server phase ordering.
  - Fixed CMake vendored dependency gating so `argparse` is only required for client/testing builds.
- Changed files:
  - `docs/runtime-phase4-plan.md`
  - `docs/runtime-reshape-plan.md`
  - `docs/context-current.md`
  - `docs/multiplayer-architecture.md`
  - `docs/multiplayer-runbook.md`
  - `README.md`
  - `REFACTORING.md`
  - `CMakeLists.txt`
  - `src/client/app/client_app.hpp`
  - `src/client/app/client_app.cpp`
  - `src/client/modules/client_phases.hpp`
  - `src/client/modules/client_runtime_module.hpp`
  - `src/client/runtime/client_runtime.hpp`
  - `src/client/runtime/client_runtime.cpp`
  - `src/client/game_client.hpp`
  - `src/client/game_client.cpp`
  - `src/server/app/server_app.hpp`
  - `src/server/app/server_app.cpp`
  - `src/server/modules/server_phases.hpp`
  - `src/server/modules/server_runtime_module.hpp`
  - `src/server/runtime/server_runtime.hpp`
  - `src/server/runtime/server_runtime.cpp`
  - `src/server/game_server.hpp`
  - `src/server/game_server.cpp`
  - `tests/CMakeLists.txt`
  - `tests/sim/client_pipeline.cpp`
  - `tests/sim/server_pipeline.cpp`
- Remaining risks/blockers:
  - Client screen state still flows through the transitional `RuntimeState + SceneManager` model behind the new flecs shell.
  - Menu/UI remains keyboard/gamepad-first and does not yet provide the planned widget/document model or mouse hover/click parity.
  - Rendering is still concentrated in the existing render system; presentation/renderer decomposition remains a follow-up slice.
  - No interactive GUI smoke test was performed in this slice; validation is currently build + automated tests.

### Validation
- `cmake --preset debug`
- `cmake --build --preset debug -j`
- `ctest --preset debug`
- `./build/debug/game_server --port 27021 --tick-rate 30 --snapshot-rate 15`
