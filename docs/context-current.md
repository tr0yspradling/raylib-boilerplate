# Context Current

Last updated: 2026-03-05

## Current Focus
- Runtime reshape for client flow: splash -> main menu -> mode-specific paths.
- Phase 1 foundation completed (runtime state + scene taxonomy + transition routing).

## Recent Completed Work
- Consolidated duplicate client runtime outputs to a single executable: `game_client`.
- Added strict Phase 1 execution plan doc.
- Implemented Phase 1 runtime scaffolding:
  - `RuntimeState` and `RuntimeMode`
  - `MenuAction` and `MenuSelectionState`
  - expanded `SceneKind` set for upcoming menu flow
  - `Application::UpdateScene(...)` now uses runtime state
  - `GameClient` now refreshes and applies runtime-state-driven scenes

## Validation Status
- Build: `cmake --build cmake-build-debug -j` passing.
- Tests: `ctest --test-dir cmake-build-debug --output-on-failure` passing (`7/7`).

## Open Risks / Gaps
- Main menu is not interactive yet (Phase 2).
- Join/start-server/singleplayer/options actions are scaffolded but not wired to UI flow.
- No phase-specific tests yet for runtime scene transition logic.

## Active Plan Docs
- `docs/runtime-reshape-plan.md`
- `docs/runtime-phase1-plan.md`

## Next Recommended Step
- Implement Phase 2:
  - interactive main menu render/input
  - action dispatch for `Start Server`, `Join Server`, `Singleplayer`, `Options`, `Quit`
  - `--auto-join` / startup behavior flags for dev flow
