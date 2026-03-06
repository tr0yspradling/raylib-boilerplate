# Runtime Reshape: Phase 2A Execution Plan

## Maintenance Slice (2026-03-06): Cross-Platform GitHub Actions Build

### Current Scope
- Add a production-ready GitHub Actions workflow that configures, builds, and tests the project on Linux, macOS, and Windows.
- Make CI-safe repository adjustments required for clean runner checkout and dependency bootstrap.
- Keep the existing preset-based local developer workflow unchanged.

### Assumptions
- GitHub-hosted runners are the target execution environment for the initial CI workflow.
- The workflow should validate the existing `debug` preset, because that is the documented minimum validation gate for phase slices.
- Windows CI will continue to rely on vcpkg for native dependencies (`openssl`, `protobuf`, `abseil`) while raylib and GameNetworkingSockets remain vendored/submodule-backed.

### Concrete File Touch List
- `.github/workflows/crossplatform-build.yml` (new)
- `.gitmodules`
- `CMakeLists.txt`
- `docs/runtime-phase2-plan.md`
- `docs/context-current.md`
- `docs/multiplayer-runbook.md`

### Acceptance Criteria
- GitHub Actions checks out repository content plus required submodules on clean runners without manual credentials.
- Linux, macOS, and Windows runners all configure and build with the documented `debug` preset.
- `ctest --preset debug` runs in CI on all supported platforms.
- Operational docs describe the CI workflow and any platform-specific dependency/bootstrap behavior it relies on.

### Status
- Completed.

### Progress Update
- Completed work:
  - Added a GitHub Actions workflow that validates the `debug` preset across `ubuntu-24.04`, `macos-14`, and `windows-2022`.
  - Wired CI checkout to use recursive submodules and upload CMake/CTest logs on failures.
  - Switched the `external/GameNetworkingSockets` submodule URL to HTTPS so clean GitHub runners can fetch it without SSH credentials.
  - Hardened top-level CMake Protobuf resolution to prefer package config mode with legacy helper compatibility, which fixes modern Homebrew/vcpkg Abseil linkage for vendored GameNetworkingSockets builds.
  - Validation gate passed locally after the CMake fix:
    - `cmake --preset debug`
    - `cmake --build --preset debug -j`
    - `ctest --preset debug`
- Changed files:
  - `.github/workflows/crossplatform-build.yml`
  - `.gitmodules`
  - `CMakeLists.txt`
  - `docs/runtime-phase2-plan.md`
  - `docs/context-current.md`
  - `docs/multiplayer-runbook.md`
- Remaining risks/blockers:
  - GitHub-hosted runner execution has not been observed yet from this local environment; the workflow has only been validated by static YAML parsing plus local macOS preset build/test coverage.

## Maintenance Slice (2026-03-06): Phase 3 Join Form and Join UX

### Current Scope
- Add editable Join form state (host/port/name) to the menu runtime model.
- Route `Join Server` from main menu into a real join form scene.
- Distinguish `JoinServer` form scene from `Connecting` progress scene.
- Keep connect failure feedback in join UX (instead of immediately dropping into disconnected scene).

### Assumptions
- This slice does not implement Start Server process launch, singleplayer runtime, or options persistence.
- Existing network protocol/handshake/snapshot behavior remains unchanged.
- Argparse-based client CLI remains the source of startup defaults and flags.

### Concrete File Touch List
- `docs/runtime-phase2-plan.md`
- `docs/runtime-reshape-plan.md`
- `docs/context-current.md`
- `docs/multiplayer-runbook.md`
- `src/client/core/runtime_state.hpp`
- `src/client/core/application.hpp`
- `src/client/core/menu_model.hpp`
- `src/client/components/components.hpp`
- `src/client/game_client.hpp`
- `src/client/game_client.cpp`
- `src/client/systems/render_system.hpp`
- `tests/CMakeLists.txt`
- `tests/sim/runtime_scene_transitions.cpp`
- `tests/sim/menu_model.cpp`
- `tests/sim/join_form_model.cpp` (new)

### Acceptance Criteria
- Main menu `Join Server` opens an editable join form scene.
- Join form supports host/port/name edits and connect submit.
- Join scene and connecting scene are distinct:
  - form edit state -> `JoinServer`
  - active network connect -> `Connecting`
- Join connect failures are surfaced in join UX and allow retry.
- Validation gate passes:
  - `cmake --build --preset debug -j`
  - `ctest --preset debug`

### Status
- Completed.

### Progress Update
- Completed work:
  - Routed main-menu `Join Server` into an editable join form scene instead of direct connect side effects.
  - Added join-form editing flow for host/port/name plus connect/back actions.
  - Drove `JoinServer` vs `Connecting` scene transitions from real connection lifecycle via `joiningInProgress`.
  - Kept connection failures in join UX with retry messaging instead of dropping to disconnected scene.
  - Added dedicated join-form model tests and expanded runtime scene transition coverage.
  - Validation gate passed:
    - `cmake --build --preset debug -j`
    - `ctest --preset debug`
- Changed files:
  - `docs/runtime-phase2-plan.md`
  - `docs/runtime-reshape-plan.md`
  - `docs/context-current.md`
  - `docs/multiplayer-runbook.md`
  - `src/client/core/runtime_state.hpp`
  - `src/client/core/application.hpp`
  - `src/client/core/menu_model.hpp`
  - `src/client/components/components.hpp`
  - `src/client/game_client.hpp`
  - `src/client/game_client.cpp`
  - `src/client/systems/render_system.hpp`
  - `tests/CMakeLists.txt`
  - `tests/sim/runtime_scene_transitions.cpp`
  - `tests/sim/join_form_model.cpp`
- Remaining risks/blockers:
  - `Start Server`, `Singleplayer`, and `Options` are still placeholder routes.
  - Post-multiplayer disconnect reason retention after returning to menu remains a follow-up item.

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
