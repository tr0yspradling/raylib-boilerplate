# Runtime Reshape: Phase 11 Execution Plan

## Client Scene Publication Cleanup Slice (2026-03-06)

### Current Scope
- Remove the active `RuntimeState + SceneManager` bridge from `ClientRuntime`.
- Make the published client scene a pure function of world-owned flow state.
- Leave singleplayer and options ownership inside `ClientRuntime` for the following slice.

### Assumptions
- `core::RuntimeState` remains the flow contract for this slice; the change is about how scene selection is derived and published, not about removing runtime mode/state itself.
- A pure `RuntimeState -> SceneKind` helper is enough to preserve current behavior for UI/document rendering, status cards, and debug overlay labels.
- The old `Application`/`SceneManager` types can remain as compatibility wrappers or test scaffolding if needed, as long as they are no longer part of the active client runtime path.

### Concrete File Touch List
- `docs/runtime-phase11-plan.md` (new)
- `docs/runtime-reshape-plan.md`
- `docs/context-current.md`
- `docs/multiplayer-architecture.md`
- `docs/multiplayer-runbook.md`
- `src/client/core/application.hpp`
- `src/client/core/scene.hpp`
- `src/client/core/scene_manager.hpp` (if touched or removed from the active path)
- `src/client/runtime/client_runtime.hpp`
- `src/client/runtime/client_runtime.cpp`
- `tests/sim/runtime_scene_transitions.cpp`

### Acceptance Criteria
- The active client runtime no longer owns or mutates a `SceneManager`.
- `ui::ScreenState.activeScene` is derived directly from runtime flow state via a pure helper.
- Status presentation and debug overlay still surface the same scenes/messages as before.
- Validation gate passes:
  - `cmake --build --preset debug -j`
  - `ctest --preset debug`
  - `timeout 2 ./build/debug/game_client --skip-splash`

### Status
- Completed.

### Progress Update
- Completed work:
  - Added a pure `core::SceneForRuntime(...)` mapping and switched the active client runtime path to publish scenes directly from world-owned flow state.
  - Removed `SceneManager` ownership from `ClientRuntime`; UI screen state, status presentation, and debug overlay now use the pure mapping.
  - Updated `test_sim_runtime_scene_transitions` to validate the pure runtime-to-scene mapping directly.
- Changed files:
  - `docs/runtime-phase11-plan.md`
  - `docs/runtime-reshape-plan.md`
  - `docs/context-current.md`
  - `docs/multiplayer-architecture.md`
  - `docs/multiplayer-runbook.md`
  - `src/client/core/application.hpp`
  - `src/client/core/scene.hpp`
  - `src/client/runtime/client_runtime.hpp`
  - `src/client/runtime/client_runtime.cpp`
  - `tests/sim/runtime_scene_transitions.cpp`
- Remaining risks/blockers:
  - Singleplayer stepping and options persistence/application remain in `ClientRuntime` and are intentionally out of scope for this slice.

### Validation
- `cmake --build --preset debug -j`
- `ctest --preset debug`
- `timeout 2 ./build/debug/game_client --skip-splash`
