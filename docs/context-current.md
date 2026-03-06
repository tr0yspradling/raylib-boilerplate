# Context Current

Last updated: 2026-03-06

## Current Focus
- Phase 5 local dedicated launcher flow on top of the flecs runtime foundation.
- Shrinking the remaining transitional `ClientRuntime` responsibilities after `Start Server` became a real runtime path.

## Recent Completed Work
- Consolidated duplicate client runtime outputs to a single executable: `game_client`.
- Added strict Phase 1 execution plan doc.
- Implemented Phase 1 runtime scaffolding:
  - `RuntimeState` and `RuntimeMode`
  - `MenuAction` and `MenuSelectionState`
  - expanded `SceneKind` set for upcoming menu flow
  - `Application::UpdateScene(...)` now uses runtime state
  - `GameClient` now refreshes and applies runtime-state-driven scenes
- Added CMake preset profile matrix:
  - `debug` -> `build/debug`
  - `release` -> `build/release`
- Updated build/run/test docs (`AGENTS.md`, `README.md`, `LIVE_TESTING_GUIDE.md`, `docs/multiplayer-runbook.md`) to use preset workflow and `build/` root paths.
- Implemented Phase 2A runtime/menu behavior:
  - default startup is splash -> menu (no eager connect)
  - added explicit join entrypoint and action dispatch in client runtime
  - added startup flags: `--auto-join`, `--skip-splash`
  - wired interactive menu navigation (keyboard + gamepad)
  - `Join Server` and `Quit` are fully wired
  - `Start Server`, `Singleplayer`, `Options` route to placeholder scenes/messages
- Added test coverage for:
  - menu model selection wrap + action labels
  - runtime scene transition mapping
  - client argument parsing for new startup flags
- Migrated client CLI parsing from manual `argv` iteration to vendored `argparse`:
  - parser-generated `--help` output
  - consistent parse error handling + usage text
  - vendored header-only dependency under `external/argparse/`
- Implemented Phase 3 join flow depth:
  - main-menu `Join Server` opens join form instead of auto-starting connect
  - join form supports host/port/name editing and connect/back actions
  - runtime now distinguishes `JoinServer` form scene from `Connecting` scene using live connect state
  - connect failures stay in join UX with retry messaging
  - added tests: `test_sim_join_form_model`, expanded `test_sim_runtime_scene_transitions`
- Implemented the Phase 4 flecs runtime foundation slice:
  - added `ClientApp` and `ServerApp` flecs composition roots
  - moved heavy runtime logic into `src/client/runtime/` and `src/server/runtime/`
  - reduced `GameClient` and `GameServer` to thin app shells
  - added explicit client/server flecs phase registration modules
  - promoted flecs to the main client/server build path
  - added world-level pipeline ordering tests for both runtimes
- Implemented the Phase 4 UI/document slice:
  - added flecs-managed UI resources for active client screen state, menu state, join-form state, input snapshots, interaction state, and queued UI commands
  - replaced the old runtime-owned menu/join state structs with `UiDocument`-based menu/join building in the `UiBuild` phase
  - added mouse hover/click support alongside existing keyboard/gamepad navigation
  - delegated menu/join rendering to a dedicated `UiRenderer`
  - added `test_sim_ui_document` and updated menu/join state tests around the new UI model
- Implemented the Phase 4 render decomposition slice:
  - added explicit status presentation state built during `PresentationBuild`
  - split background, splash, centered status, and gameplay world drawing into dedicated renderer helpers
  - reduced `RenderSystem` to a presentation router instead of a monolithic drawing implementation
  - added `test_sim_status_presenter` for status presentation mapping
- Implemented the Phase 5 local dedicated launcher slice:
  - added `core::IServerLauncher` plus the process-based `ServerLauncherProcess` implementation
  - captured the resolved client executable path in `ClientConfig` for sibling `game_server` discovery
  - replaced the `Start Server` placeholder with a real `MainMenu -> StartingLocalServer -> Connecting -> GameplayMultiplayer` flow
  - added localhost readiness retry, timeout/failure messaging, and startup cancel cleanup
  - added `test_sim_server_launcher_process` to cover sibling path resolution and launch command construction
- Fixed CMake vendored dependency gating so `argparse` is only required for client/testing builds.

## Validation Status
- Configure: `cmake --preset debug` passing (`build/debug` generated).
- Build: `cmake --build --preset debug -j` passing.
- Tests: `ctest --preset debug` passing (`16/16`).
- Runtime sanity: `./build/debug/game_server --port 27021 --tick-rate 30 --snapshot-rate 15` starts successfully.
- Client startup sanity: `./build/debug/game_client --host 127.0.0.1 --port 27021 --auto-join --skip-splash` starts successfully alongside the dedicated server.
- Manual GUI smoke: not yet run for the full `Start Server` menu path.

## Open Risks / Gaps
- Client runtime flow still depends on transitional `RuntimeState` + `SceneManager` internals behind the flecs shell.
- Transport/session state and most orchestration logic still live inside `ClientRuntime`.
- The remaining major client/runtime work is state/orchestration decomposition plus implementation of the remaining placeholder flows.
- `Singleplayer` and `Options` still remain placeholders.
- Local dedicated server ownership/retry state is runtime-local today; it has not yet moved into explicit flecs resources/components.
- Post-multiplayer disconnect reason retention after returning to menu is still pending.
- Developers using legacy non-preset IDE profiles can still generate `cmake-build-*` folders unless they switch to preset-backed profiles.

## Active Plan Docs
- `docs/runtime-reshape-plan.md`
- `docs/runtime-phase1-plan.md`
- `docs/runtime-phase2-plan.md`
- `docs/runtime-phase4-plan.md`
- `docs/runtime-phase5-plan.md`

## Next Recommended Step
- Continue decomposing runtime state inside the flecs worlds:
  - move local-start/session ownership and screen flow out of `ClientRuntime` into explicit flecs resources/services
  - implement the remaining real `Singleplayer` and `Options` flows on top of the current UI/document path
  - remove more of the transitional `RuntimeState + SceneManager` dependency from the client shell
