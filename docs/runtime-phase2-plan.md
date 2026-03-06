# Runtime Reshape: Phase 2A Execution Plan

## Maintenance Slice (2026-03-06): Client CLI Argparse Migration

### Current Scope
- Replace the client's hand-rolled command-line parsing with the `argparse` library.
- Preserve the existing client flags and defaults:
  - `--host`
  - `--port`
  - `--name`
  - `--tick-rate`
  - `--auto-join`
  - `--skip-splash`
  - `--help`
- Keep the Phase 2A runtime/menu behavior unchanged.

### Assumptions
- This slice only changes the client CLI parser; server parsing remains unchanged.
- Vendoring a header-only `argparse` dependency under `external/` is acceptable for this repo.
- Client help output can switch from the current hand-written usage line to library-generated help text.

### Concrete File Touch List
- `docs/runtime-phase2-plan.md`
- `docs/context-current.md`
- `CMakeLists.txt`
- `external/argparse/include/argparse/argparse.hpp` (new)
- `external/argparse/LICENSE` (new)
- `src/client/core/client_args.hpp`
- `src/client/client_entry.cpp`
- `tests/CMakeLists.txt`
- `tests/sim/client_args.cpp`

### Acceptance Criteria
- Client argument parsing uses `argparse` instead of manual `argv` iteration.
- Existing flags continue to populate `ClientConfig` correctly.
- `--help` returns cleanly and prints parser-generated help text.
- Validation gate passes:
  - `cmake --build --preset debug -j`
  - `ctest --preset debug`

### Status
- Completed.

### Progress Update
- Completed work:
  - Scoped the client argparse migration slice and documented intended file touches and acceptance criteria.
  - Vendored the header-only `argparse` dependency under `external/argparse/`.
  - Replaced manual client `argv` iteration with `argparse`-based parsing in `src/client/core/client_args.hpp`.
  - Switched client help output to parser-generated help text and surfaced parse errors with the generated usage text.
  - Updated the client arg parser test to verify generated help content.
  - Validated with:
    - `cmake --build --preset debug -j`
    - `ctest --preset debug`
- Changed files:
  - `docs/runtime-phase2-plan.md`
  - `docs/context-current.md`
  - `CMakeLists.txt`
  - `external/argparse/include/argparse/argparse.hpp`
  - `external/argparse/LICENSE`
  - `src/client/core/client_args.hpp`
  - `src/client/client_entry.cpp`
  - `tests/CMakeLists.txt`
  - `tests/sim/client_args.cpp`
- Remaining risks/blockers:
  - None for this slice.

## Scope
Implement the menu-first runtime slice only:
- startup defaults to splash -> menu (no eager connect)
- optional startup flags (`--auto-join`, `--skip-splash`)
- interactive menu navigation and action dispatch
- `Join Server` and `Quit` fully wired
- `Start Server`, `Singleplayer`, `Options` routed to placeholder scenes/messages

## Assumptions
- Existing multiplayer protocol and server behavior remain unchanged.
- Local server launcher, singleplayer sim runtime, and options persistence are out of scope for this slice.
- Build/test commands use preset workflow rooted under `build/`.

## Concrete File Touch List
- `docs/runtime-phase2-plan.md` (new)
- `docs/runtime-reshape-plan.md`
- `docs/context-current.md`
- `docs/multiplayer-runbook.md`
- `src/client/game_client.hpp`
- `src/client/game_client.cpp`
- `src/client/input/input_manager.hpp`
- `src/client/client_entry.cpp`
- `src/client/core/menu_model.hpp`
- `src/client/components/components.hpp`
- `src/client/systems/render_system.hpp`
- `tests/CMakeLists.txt`
- `tests/sim/menu_model.cpp` (new)
- `tests/sim/runtime_scene_transitions.cpp` (new)
- `tests/sim/client_args.cpp` (new)

## Acceptance Criteria
- Launching client without flags does not connect immediately; flow is splash -> menu.
- Menu is interactive by keyboard and gamepad:
  - `Join Server` starts connect flow.
  - `Quit` exits cleanly.
  - `Start Server` / `Singleplayer` / `Options` show placeholder scene messaging.
- `--auto-join` triggers join flow from startup.
- `--skip-splash` starts directly in menu (or direct join when combined with `--auto-join`).
- Automated tests cover:
  - menu selection wrap + action mapping
  - scene transition mapping
  - client argument parsing for new flags
- Validation gate passes:
  - `cmake --build --preset debug -j`
  - `ctest --preset debug`

## Implementation Steps
1. Add Phase 2 runtime/menu input and join entrypoint scaffolding in `GameClient`.
2. Add startup flags and parsing updates.
3. Add menu/placeholder rendering behavior.
4. Add automated tests and wire into CTest.
5. Run build + tests and update context docs.

## Status
- Completed.

## Progress Update
- Completed work:
  - Added menu-first startup behavior and explicit `BeginJoinServer()` flow.
  - Added startup flags (`--auto-join`, `--skip-splash`) and parser coverage tests.
  - Wired interactive menu input/action dispatch with placeholders for non-Join non-Quit actions.
  - Added scene transition and menu model tests.
  - Updated run/docs context for new startup behavior.
- Changed files:
  - `docs/runtime-phase2-plan.md`
  - `docs/runtime-reshape-plan.md`
  - `docs/context-current.md`
  - `docs/multiplayer-runbook.md`
  - `README.md`
  - `LIVE_TESTING_GUIDE.md`
  - `src/client/core/client_config.hpp`
  - `src/client/core/client_args.hpp`
  - `src/client/core/menu_model.hpp`
  - `src/client/client_entry.cpp`
  - `src/client/game_client.hpp`
  - `src/client/game_client.cpp`
  - `src/client/input/input_manager.hpp`
  - `src/client/components/components.hpp`
  - `src/client/systems/render_system.hpp`
  - `tests/CMakeLists.txt`
  - `tests/sim/menu_model.cpp`
  - `tests/sim/runtime_scene_transitions.cpp`
  - `tests/sim/client_args.cpp`
- Remaining risks/blockers:
  - `Start Server`, `Singleplayer`, and `Options` remain placeholder scenes/messages in this slice.
  - Join form editing UX (host/port/name in-menu) is still pending for the next phase.
