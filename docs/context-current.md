# Context Current

Last updated: 2026-03-06

## Current Focus
- Phase 4 flecs runtime foundation on top of the existing menu/join multiplayer flow.
- Preserving current behavior while moving the active client/server entrypoints onto flecs composition roots.

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
- Fixed CMake vendored dependency gating so `argparse` is only required for client/testing builds.

## Validation Status
- Configure: `cmake --preset debug` passing (`build/debug` generated).
- Build: `cmake --build --preset debug -j` passing.
- Tests: `ctest --preset debug` passing (`13/13`).
- Runtime sanity: `./build/debug/game_server --port 27021 --tick-rate 30 --snapshot-rate 15` starts successfully.
- Manual GUI smoke: not run for the new client/runtime path in this slice.

## Open Risks / Gaps
- Client screen state still depends on transitional `RuntimeState` + `SceneManager` internals behind the new flecs shell.
- `menu_model` and input/UI handling still need the planned widget/document redesign for mouse hover/click parity.
- `render_system` is still monolithic and needs to be split into UI, world, and debug presentation modules.
- `Start Server`, `Singleplayer`, and `Options` remain placeholders (no real runtime flows yet).
- Post-multiplayer disconnect reason retention after returning to menu is still pending.
- Developers using legacy non-preset IDE profiles can still generate `cmake-build-*` folders unless they switch to preset-backed profiles.

## Active Plan Docs
- `docs/runtime-reshape-plan.md`
- `docs/runtime-phase1-plan.md`
- `docs/runtime-phase2-plan.md`
- `docs/runtime-phase4-plan.md`

## Next Recommended Step
- Continue Phase 4 with state decomposition inside the flecs worlds:
  - move active screen/runtime state into explicit flecs resources/components
  - replace the current menu model with a UI document/widget interaction layer
  - split presentation/render work out of the monolithic render system
