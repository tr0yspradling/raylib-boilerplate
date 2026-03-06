# Runtime Reshape: Phase 5 Execution Plan

## Local Dedicated Launcher Slice (2026-03-06)

### Current Scope
- Replace the `Start Server` placeholder flow with a real process-based local dedicated launcher.
- Launch `game_server` as a sibling process, then auto-join localhost with retry/readiness gating.
- Surface launch failure, startup timeout, and user-cancel paths back into the existing menu/UI model with actionable status text.

### Assumptions
- Process-based launch remains the first pass; no embedded/in-process dedicated server is introduced in this slice.
- The launcher may keep the spawned dedicated server alive after a successful connection; this slice only guarantees cleanup on startup cancel and client shutdown.
- The existing flecs UI/document and render structure remains intact; this slice focuses on real local-server flow behavior.

### Concrete File Touch List
- `docs/runtime-phase5-plan.md` (new)
- `docs/runtime-reshape-plan.md`
- `docs/context-current.md`
- `docs/multiplayer-architecture.md`
- `docs/multiplayer-runbook.md`
- `README.md`
- `REFACTORING.md`
- `CMakeLists.txt`
- `src/client/core/client_config.hpp`
- `src/client/client_entry.cpp`
- `src/client/runtime/client_runtime.hpp`
- `src/client/runtime/client_runtime.cpp`
- `src/client/core/server_launcher.hpp` (new)
- `src/client/core/server_launcher_process.hpp` (new)
- `src/client/core/server_launcher_process.cpp` (new)
- `tests/CMakeLists.txt`
- `tests/sim/server_launcher_process.cpp` (new)

### Acceptance Criteria
- Selecting `Start Server` launches a sibling `game_server` process.
- Client flow transitions through:
  - `MainMenu -> StartingLocalServer -> Connecting -> GameplayMultiplayer`
- Canceling during local startup stops the launched server process and returns to menu.
- Launch/readiness failure returns to menu with a useful status message.
- Validation gate passes:
  - `cmake --build --preset debug -j`
  - `ctest --preset debug`

### Status
- Local dedicated launcher slice completed on 2026-03-06.

### Progress Update
- Completed work:
  - Added `core::IServerLauncher` and the process-based `ServerLauncherProcess` implementation for sibling `game_server` launch.
  - Captured the resolved client executable path in `ClientConfig` so the launcher can derive the matching `game_server` binary.
  - Replaced the `Start Server` placeholder route with a real local dedicated startup flow in `ClientRuntime`.
  - Added retry/readiness gating so startup remains in `StartingLocalServer` until the dedicated server is ready to accept a connection.
  - Added cancel/failure cleanup that returns to the menu with actionable status text.
  - Added `test_sim_server_launcher_process` for sibling path resolution and launch command construction.
- Changed files:
  - `docs/runtime-phase5-plan.md`
  - `docs/runtime-reshape-plan.md`
  - `docs/context-current.md`
  - `docs/multiplayer-architecture.md`
  - `docs/multiplayer-runbook.md`
  - `README.md`
  - `REFACTORING.md`
  - `CMakeLists.txt`
  - `tests/CMakeLists.txt`
  - `tests/sim/server_launcher_process.cpp`
  - `src/client/client_entry.cpp`
  - `src/client/core/client_config.hpp`
  - `src/client/core/server_launcher.hpp`
  - `src/client/core/server_launcher_process.hpp`
  - `src/client/core/server_launcher_process.cpp`
  - `src/client/runtime/client_runtime.hpp`
  - `src/client/runtime/client_runtime.cpp`
- Remaining risks/blockers:
  - Local dedicated ownership still lives inside the transitional `ClientRuntime` service rather than explicit flecs resources/components.
  - Full manual GUI validation of the `Start Server` menu path is still pending; the current validation covers build/tests plus concurrent client/server startup sanity.
  - `Singleplayer` and `Options` remain placeholder flows for later phases.

### Validation
- `cmake --build --preset debug -j`
- `ctest --preset debug`
- Concurrent startup sanity:
  - `./build/debug/game_server --port 27021 --tick-rate 30 --snapshot-rate 15`
  - `./build/debug/game_client --host 127.0.0.1 --port 27021 --auto-join --skip-splash`
