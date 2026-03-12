# ECS Cleanup Plan

## Objective
Reshape the first-party runtime code so the flecs pipeline remains the application backbone while oversized orchestration logic is split into focused helpers and subsystem-owned policy catalogs.

The cleanup program preserves the current multiplayer, singleplayer, menu, config, and protocol behavior while making these improvements:
- move named tunables and policy literals out of implementation bodies
- keep ECS as the owner of runtime state and phase boundaries
- reduce monolithic runtime files into phase-aligned helpers
- keep shared gameplay and networking defaults centralized and reusable

## Constant Policy
- Extract named defaults, bounds, timers, layout metrics, colors, text limits, status copy, disconnect/resync codes, and transport/gameplay policy values into subsystem-owned headers.
- Keep incidental control-flow literals inline when a named constant would not improve clarity.
- Prefer the owning subsystem for each constant catalog instead of a global constants dump.

## Current Baseline
- Runtime reshape is complete through the Phase 14 acceptance/testability slice.
- Client and server both run through flecs worlds with explicit phase ordering.
- The ECS cleanup refactor is now implemented on the active runtime path:
  - subsystem-owned policy catalogs exist under `src/shared/game/`, `src/shared/net/`, `src/client/`, and `src/server/`
  - `ClientRuntime` now delegates flow control, UI interaction, UI document construction, and presentation publication to focused helpers
  - `ServerRuntime` now delegates transport/message/session/replication/metrics work through focused runtime ops and context/state helpers
- Current automated baseline is green:
  - `cmake --preset debug`
  - `cmake --build --preset debug -j`
  - `ctest --preset debug`
- Current runtime sanity checks are green:
  - `./build/debug/game_server --port 27021 --tick-rate 30 --snapshot-rate 15`
  - `timeout 2 ./build/debug/game_client --skip-splash`
- The largest remaining cleanup hotspots are:
  - `src/client/runtime/multiplayer_session_service.cpp`
  - `src/server/runtime/server_runtime_ops.cpp`
  - `src/client/ui/ui_state.hpp`
  - `src/client/ui/ui_renderer.hpp`

## Planned Slices

### Phase 1: Policy Catalogs and Client Runtime Decomposition
- Status: completed on 2026-03-12
- Add shared/client/server policy headers for named runtime, UI, gameplay, and network defaults.
- Split client runtime behavior into phase-aligned helpers:
  - runtime flow control
  - UI interaction handling
  - UI document construction
  - presentation publication
- Keep ECS resources as the source of truth and leave flecs phase wiring intact.

### Phase 2: Server Runtime Decomposition
- Status: completed on 2026-03-12
- Split server runtime behavior into focused helpers for:
  - session/bootstrap handling
  - packet decode/dispatch
  - chunk replication
  - metrics and persistence coordination
- Replace raw disconnect and resync codes with shared named policy values.

### Phase 3: Repo Cleanup Sweep and Validation Closure
- Status: completed on 2026-03-12 for the active runtime path
- Sweep remaining first-party files for policy extraction opportunities.
- Expand focused tests around extracted policies and decomposed helpers.
- Refresh architecture/runbook docs to match the post-cleanup ownership model.
- Validate, commit, and push each completed slice.
