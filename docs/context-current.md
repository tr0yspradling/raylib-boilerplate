# Context Current

Last updated: 2026-03-06

## Current Focus
- Runtime/service decomposition after the completed client scene-publication cleanup slice.
- Shrinking the remaining transitional `ClientRuntime` responsibilities now that client flow state, multiplayer session state, transport orchestration, and scene publication are split more cleanly.

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
- Implemented the Phase 6 singleplayer slice:
  - added `core::SingleplayerRuntime` as a transport-free local authoritative wrapper around `shared::game::GameState`
  - replaced the `Singleplayer` placeholder with a real `MainMenu -> GameplaySingleplayer` local sandbox path
  - reused the gameplay world renderer for singleplayer movement/jump visualization
  - added `test_sim_singleplayer_runtime` to cover local spawn, movement, jump, and shutdown semantics
- Implemented the Phase 7 options/config slice:
  - added persisted client preference load/save helpers under `client/core/config.*`
  - client startup now loads `client_data/client.cfg` and applies explicit CLI overrides on top
  - replaced the `Options` placeholder with a real options screen on the UI/document path
  - options now apply player/default-host/default-port, target FPS, window size, interpolation delay, and debug overlay default live
  - added `test_sim_client_config_file` for client preference persistence coverage
- Implemented the Phase 8 client flow-resource slice:
  - added `runtime::ClientFlowState` and `runtime::LocalServerStartupState` as explicit flecs-managed client-world resources
  - moved runtime mode, splash completion, requested actions, runtime/disconnect status text, debug overlay toggle, and local dedicated retry/ownership state out of `ClientRuntime` members
  - fixed the disconnect return path so menu status preserves the last disconnect reason
  - added `test_sim_client_runtime_flow_resources` to cover the new resource-backed transition rules
- Implemented the Phase 9 client session-resource slice:
  - added `runtime::ClientSessionState` as an explicit flecs-managed client-world resource
  - moved multiplayer connection state, handshake/tick metadata, prediction buffers, remote interpolation state, and chunk cache/resync bookkeeping out of `ClientRuntime` members
  - added `test_sim_client_session_resources` to cover the new resource reset semantics
- Implemented the Phase 10 multiplayer-session-service slice:
  - added `runtime::MultiplayerSessionService` as the dedicated client-side boundary for transport bootstrap, connect/disconnect handling, packet decode/dispatch, multiplayer cadence sends, and metrics
  - removed direct transport/message-handling ownership from `ClientRuntime`, which now orchestrates around the service and existing flecs-owned resources
  - added `test_sim_client_multiplayer_session_service` to cover the extracted connect/event/ClientHello path with a fake transport
- Implemented the Phase 11 scene-publication cleanup slice:
  - added pure `core::SceneForRuntime(...)` mapping from runtime flow state to `SceneKind`
  - removed `SceneManager` ownership from the active `ClientRuntime` path
  - updated `ui::ScreenState`, status presentation, debug overlay, and runtime scene-transition tests to use the pure mapping
- Fixed CMake vendored dependency gating so `argparse` is only required for client/testing builds.

## Validation Status
- Configure: `cmake --preset debug` passing (`build/debug` generated).
- Build: `cmake --build --preset debug -j` passing.
- Tests: `ctest --preset debug` passing (`21/21`).
- Runtime sanity: `./build/debug/game_server --port 27021 --tick-rate 30 --snapshot-rate 15` starts successfully.
- Client startup sanity: `./build/debug/game_client --host 127.0.0.1 --port 27021 --auto-join --skip-splash` starts successfully alongside the dedicated server.
- Client startup sanity: `./build/debug/game_client --skip-splash` starts successfully for the local gameplay path.
- Client startup sanity: `timeout 2 ./build/debug/game_client --skip-splash` reaches a live window and frame loop under the new session-resource path.
- Manual GUI smoke: not yet run for the full menu-driven `Start Server`, `Singleplayer`, and `Options` paths.

## Open Risks / Gaps
- Singleplayer stepping and broader local gameplay ownership still live inside `ClientRuntime`.
- Options persistence is still applied from `ClientRuntime`, even though the live control state is now in flecs resources.
- Manual GUI smoke for `Start Server`, `Singleplayer`, and `Options` is still pending.
- Developers using legacy non-preset IDE profiles can still generate `cmake-build-*` folders unless they switch to preset-backed profiles.

## Active Plan Docs
- `docs/runtime-reshape-plan.md`
- `docs/runtime-phase1-plan.md`
- `docs/runtime-phase2-plan.md`
- `docs/runtime-phase4-plan.md`
- `docs/runtime-phase5-plan.md`
- `docs/runtime-phase6-plan.md`
- `docs/runtime-phase7-plan.md`
- `docs/runtime-phase8-plan.md`
- `docs/runtime-phase9-plan.md`
- `docs/runtime-phase10-plan.md`
- `docs/runtime-phase11-plan.md`

## Next Recommended Step
- Continue decomposing `ClientRuntime` behavior now that the world owns control flow and multiplayer state, transport lives behind a service, and scene publication is pure:
  - push singleplayer/runtime-service ownership further out of `ClientRuntime`
  - move options persistence/application behind a clearer service boundary
  - run the pending manual GUI smoke for `Start Server`, `Singleplayer`, and `Options`
