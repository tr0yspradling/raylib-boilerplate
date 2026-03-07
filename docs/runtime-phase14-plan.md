# Runtime Reshape: Phase 14 Execution Plan

## Runtime Acceptance and Testability Slice (2026-03-07)

### Current Scope
- Finish the planned runtime program with an acceptance-focused slice rather than another architectural decomposition.
- Add windowless runtime acceptance coverage for the remaining menu-driven flows:
  - `Start Server` failure/cancel-safe return path
  - `Singleplayer` entry/public session publication
  - `Options` save/integration path
- Keep behavior unchanged unless a concrete acceptance/testability gap is found while adding coverage.

### Assumptions
- Full hands-on GUI validation is still useful, but the repo should not rely only on manual smoke for the remaining critical menu paths.
- `ClientRuntime` can gain a small testability-oriented refactor if it stays runtime-neutral and does not expand user-facing scope.
- This slice should not redesign UI, networking, or persistence again; it should harden and validate what already shipped.

### Concrete File Touch List
- `docs/runtime-phase14-plan.md` (new)
- `docs/runtime-reshape-plan.md`
- `docs/context-current.md`
- `docs/multiplayer-architecture.md`
- `docs/multiplayer-runbook.md`
- `CMakeLists.txt` (if new test wiring is needed)
- `tests/CMakeLists.txt`
- `src/client/runtime/client_runtime.hpp`
- `src/client/runtime/client_runtime.cpp`
- `tests/sim/client_runtime_acceptance.cpp` (new)

### Acceptance Criteria
- `ClientRuntime` supports a windowless acceptance path for tests without changing shipped runtime behavior.
- Automated coverage exists for:
  - menu-driven `Start Server` launch failure returning to the menu with actionable status
  - menu-driven `Singleplayer` entry publishing local gameplay state
  - menu-driven `Options` save updating config/join defaults and persisting to disk
- Validation gate passes:
  - `cmake --build --preset debug -j`
  - `ctest --preset debug`
  - `timeout 2 ./build/debug/game_client --skip-splash`

### Status
- Completed.

### Progress Update
- Completed work:
  - Scoped the final planned runtime slice around acceptance hardening and testability rather than additional decomposition.
  - Extracted window-independent client-world initialization into `ClientRuntime::InitializeWorldState(...)` so acceptance tests can exercise runtime behavior without opening a raylib window.
  - Added `test_sim_client_runtime_acceptance` to cover:
    - menu-driven `Start Server` launch failure returning to the menu with actionable status text
    - menu-driven `Singleplayer` entry plus gameplay-state publication
    - menu-driven `Options` save integration, config persistence, and join-default refresh
- Changed files:
  - `docs/runtime-phase14-plan.md`
  - `docs/runtime-reshape-plan.md`
  - `docs/context-current.md`
  - `docs/multiplayer-architecture.md`
  - `docs/multiplayer-runbook.md`
  - `tests/CMakeLists.txt`
  - `src/client/runtime/client_runtime.hpp`
  - `src/client/runtime/client_runtime.cpp`
  - `tests/sim/client_runtime_acceptance.cpp`
- Remaining risks/blockers:
  - Full hands-on GUI validation for `Start Server`, `Singleplayer`, and `Options` still cannot be fully replaced by terminal-only automation.
  - The planned runtime reshape program is complete, but final release-style visual/input smoke remains a separate manual validation activity.

### Validation
- `cmake --build --preset debug -j`
- `ctest --preset debug`
- `timeout 2 ./build/debug/game_client --skip-splash`
