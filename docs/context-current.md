# Context Current

Last updated: 2026-03-06

## Current Focus
- Runtime reshape for client flow: splash -> main menu -> mode-specific paths.
- Phase 2A menu-first runtime slice completed.

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

## Validation Status
- Configure: `cmake --preset debug` passing (`build/debug` generated).
- Build: `cmake --build --preset debug -j` passing.
- Tests: `ctest --preset debug` passing (`10/10`).

## Open Risks / Gaps
- `Start Server`, `Singleplayer`, and `Options` remain placeholders (no real runtime flows yet).
- Join form editing UX (host/port/name in-menu) is still pending.
- Developers using legacy non-preset IDE profiles can still generate `cmake-build-*` folders unless they switch to preset-backed profiles.

## Active Plan Docs
- `docs/runtime-reshape-plan.md`
- `docs/runtime-phase1-plan.md`
- `docs/runtime-phase2-plan.md`

## Next Recommended Step
- Implement Phase 2B / Phase 3 join flow depth:
  - join form editing UX (host/port/name) from main menu
  - connect/disconnect lifecycle polish and menu return paths
  - replace placeholder `Start Server` / `Singleplayer` / `Options` with real implementations
