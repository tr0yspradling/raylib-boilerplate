# Context Current

Last updated: 2026-03-06

## Current Focus
- Runtime reshape for client flow: splash -> main menu -> mode-specific paths.
- Build/tooling layout standardized so profile outputs live under `build/`.

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

## Validation Status
- Configure: `cmake --preset debug` passing (`build/debug` generated).
- Build: `cmake --build --preset debug -j` passing.
- Tests: `ctest --preset debug` passing (`7/7`).

## Open Risks / Gaps
- Main menu is not interactive yet (Phase 2).
- Join/start-server/singleplayer/options actions are scaffolded but not wired to UI flow.
- No phase-specific tests yet for runtime scene transition logic.
- Developers using legacy non-preset IDE profiles can still generate `cmake-build-*` folders unless they switch to preset-backed profiles.

## Active Plan Docs
- `docs/runtime-reshape-plan.md`
- `docs/runtime-phase1-plan.md`

## Next Recommended Step
- Implement Phase 2:
  - interactive main menu render/input
  - action dispatch for `Start Server`, `Join Server`, `Singleplayer`, `Options`, `Quit`
  - `--auto-join` / startup behavior flags for dev flow
