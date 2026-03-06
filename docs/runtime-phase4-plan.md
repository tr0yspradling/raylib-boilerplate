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

## UI State Slice (2026-03-06): Flecs-Managed Screen/UI Resources

### Current Scope
- Move client menu/join screen state out of the old menu model structs and into flecs-managed resources.
- Add a plain `UiDocument` / widget model plus interaction resources for keyboard, gamepad, and mouse.
- Make the client UI phases (`UiBuild`, `UiInteraction`) do real work and emit typed commands back to the runtime.
- Split menu/join rendering out of the monolithic render switchboard into a dedicated UI renderer while preserving gameplay rendering and overlay behavior.

### Assumptions
- Transport, prediction, reconciliation, and multiplayer session logic remain in `ClientRuntime` for this slice.
- `RuntimeState + SceneManager` can remain transitional as long as menu/join screen state and input handling move into world resources.
- The first UI-document slice only needs to cover active menu/join screens plus hover/focus/pressed/editing states; placeholder screens can remain status-card based.

### Concrete File Touch List
- `docs/runtime-phase4-plan.md`
- `docs/runtime-reshape-plan.md`
- `docs/context-current.md`
- `docs/multiplayer-architecture.md`
- `docs/multiplayer-runbook.md`
- `src/client/components/components.hpp`
- `src/client/input/input_manager.hpp`
- `src/client/modules/client_runtime_module.hpp`
- `src/client/runtime/client_runtime.hpp`
- `src/client/runtime/client_runtime.cpp`
- `src/client/systems/render_system.hpp`
- `src/client/ui/ui_document.hpp` (new)
- `src/client/ui/ui_renderer.hpp` (new)
- `src/client/ui/ui_state.hpp` (new)
- `tests/CMakeLists.txt`
- `tests/sim/menu_model.cpp`
- `tests/sim/join_form_model.cpp`
- `tests/sim/ui_document.cpp` (new)

### Acceptance Criteria
- Menu and join-form state live in flecs-managed resources instead of the old `MenuSelectionState` / `JoinServerFormState` runtime members.
- `UiBuild` produces a document with widget bounds/state for the active menu/join screen.
- `UiInteraction` supports:
  - keyboard navigation/select/back
  - gamepad navigation/select/back
  - mouse hover
  - mouse click activation
- Menu and join-form rendering consume the `UiDocument` through a dedicated UI renderer path.
- Validation gate passes:
  - `cmake --build --preset debug -j`
  - `ctest --preset debug`

### Status
- UI/document slice completed on 2026-03-06. Follow-on Phase 4 slices remain in progress.

### Progress Update
- Completed work:
  - Added flecs-managed UI resources for active screen state, menu state, join-form state, input snapshots, interaction state, and queued UI commands.
  - Replaced the old menu/join runtime-owned state structs with `UiDocument`-driven screen building in the `UiBuild` phase.
  - Implemented keyboard/gamepad navigation plus mouse hover/click handling in `UiInteraction`.
  - Moved menu and join rendering onto a dedicated `UiRenderer` that consumes the built document.
  - Added coverage for the new UI state/document helpers and preserved the existing runtime/build validation gates.
- Changed files:
  - `docs/runtime-phase4-plan.md`
  - `docs/runtime-reshape-plan.md`
  - `docs/context-current.md`
  - `docs/multiplayer-architecture.md`
  - `docs/multiplayer-runbook.md`
  - `README.md`
  - `REFACTORING.md`
  - `CMakeLists.txt`
  - `src/client/app/client_app.cpp`
  - `src/client/app/client_app.hpp`
  - `src/client/components/components.hpp`
  - `src/client/core/menu_model.hpp`
  - `src/client/input/input_manager.hpp`
  - `src/client/modules/client_runtime_module.hpp`
  - `src/client/runtime/client_runtime.cpp`
  - `src/client/runtime/client_runtime.hpp`
  - `src/client/systems/render_system.hpp`
  - `src/client/ui/ui_document.hpp`
  - `src/client/ui/ui_renderer.hpp`
  - `src/client/ui/ui_state.hpp`
  - `tests/CMakeLists.txt`
  - `tests/sim/menu_model.cpp`
  - `tests/sim/join_form_model.cpp`
  - `tests/sim/ui_document.cpp`
- Remaining risks/blockers:
  - Transport/session state and most runtime flow logic still live in `ClientRuntime`.
  - `RuntimeState + SceneManager` remains the transitional screen-state mapping layer under the flecs resources.
  - Gameplay, placeholder status screens, and debug/world rendering are not yet split into dedicated presentation modules.

### Validation
- `cmake --build --preset debug -j`
- `ctest --preset debug`
- `./build/debug/game_client --skip-splash`

## Render Split Slice (2026-03-06): Presentation/Renderer Decomposition

### Current Scope
- Split the remaining client render path into dedicated background, splash, status, and gameplay render helpers.
- Move non-UI status-screen text selection out of `RenderSystem` into explicit presentation state built during `PresentationBuild`.
- Keep `RenderSystem` only as the top-level router/orchestrator across UI, gameplay, splash, status, and debug overlay rendering.

### Assumptions
- The current `UiDocument` path for menu/join screens remains the UI presentation contract for this slice.
- `DebugOverlay` can remain its own renderer; the slice only needs to remove gameplay/status/splash drawing from the monolithic switchboard.
- No gameplay/network behavior changes are intended; this is a presentation-structure refactor.

### Concrete File Touch List
- `docs/runtime-phase4-plan.md`
- `docs/runtime-reshape-plan.md`
- `docs/context-current.md`
- `docs/multiplayer-architecture.md`
- `docs/multiplayer-runbook.md`
- `README.md`
- `REFACTORING.md`
- `CMakeLists.txt`
- `src/client/app/client_app.cpp`
- `src/client/components/components.hpp`
- `src/client/runtime/client_runtime.cpp`
- `src/client/runtime/client_runtime.hpp`
- `src/client/systems/render_system.hpp`
- `src/client/render/background_renderer.hpp` (new)
- `src/client/render/splash_renderer.hpp` (new)
- `src/client/render/status_presenter.hpp` (new)
- `src/client/render/status_renderer.hpp` (new)
- `src/client/render/world_renderer.hpp` (new)
- `tests/CMakeLists.txt`
- `tests/sim/status_presenter.cpp` (new)

### Acceptance Criteria
- `RenderSystem` no longer owns concrete gameplay/status/splash draw implementations.
- Non-UI status screens use explicit presentation state built during `PresentationBuild`.
- Gameplay, splash, and centered status rendering each live in dedicated renderer headers.
- Validation gate passes:
  - `cmake --build --preset debug -j`
  - `ctest --preset debug`

### Status
- Render decomposition slice completed on 2026-03-06. Follow-on Phase 4 slices remain in progress.

### Progress Update
- Completed work:
  - Added explicit non-UI status presentation state built during `PresentationBuild`.
  - Split splash, centered status, gameplay world, and background drawing into dedicated renderer headers.
  - Reduced `RenderSystem` to a top-level router across UI, splash, status, gameplay, and debug overlay rendering.
  - Added coverage for the status presentation mapping and preserved client runtime validation.
- Changed files:
  - `docs/runtime-phase4-plan.md`
  - `docs/runtime-reshape-plan.md`
  - `docs/context-current.md`
  - `docs/multiplayer-architecture.md`
  - `docs/multiplayer-runbook.md`
  - `README.md`
  - `REFACTORING.md`
  - `CMakeLists.txt`
  - `src/client/app/client_app.cpp`
  - `src/client/components/components.hpp`
  - `src/client/runtime/client_runtime.cpp`
  - `src/client/runtime/client_runtime.hpp`
  - `src/client/systems/render_system.hpp`
  - `src/client/render/background_renderer.hpp`
  - `src/client/render/splash_renderer.hpp`
  - `src/client/render/status_presenter.hpp`
  - `src/client/render/status_renderer.hpp`
  - `src/client/render/world_renderer.hpp`
  - `tests/CMakeLists.txt`
  - `tests/sim/status_presenter.cpp`
- Remaining risks/blockers:
  - This slice does not yet move transport/session logic out of `ClientRuntime`.
  - `RuntimeState + SceneManager` remains a transitional screen-state layer.
  - The real `Start Server`, `Singleplayer`, and `Options` flows still need implementation on top of the cleaner presentation split.

### Validation
- `cmake --build --preset debug -j`
- `ctest --preset debug`
- `./build/debug/game_client --skip-splash`
