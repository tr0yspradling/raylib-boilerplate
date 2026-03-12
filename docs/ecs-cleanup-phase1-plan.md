# ECS Cleanup: Phase 1 Execution Plan

## Phase 1 Slice (2026-03-12): Policy Catalogs and Client Runtime Decomposition

### Current Scope
- Establish subsystem-owned policy catalogs for shared gameplay, shared networking, client runtime, client UI, and server runtime.
- Decompose the client runtime so `ClientRuntime` stays a composition root while phase work moves into focused helpers.
- Preserve existing behavior for:
  - splash and menu flow
  - join flow and local dedicated startup
  - singleplayer entry/exit
  - options save/apply behavior
  - multiplayer transport cadence and protocol handling
- Start replacing raw disconnect/resync codes with shared named values used by runtime logic and tests.

### Assumptions
- This first cleanup phase may touch both client and shared/server policy surfaces if that is necessary to remove duplicated literals from the active runtime path.
- ECS remains hybrid:
  - flecs owns runtime state/resources and phase ordering
  - helper classes/functions own concentrated logic where direct ECS systems would add ceremony without improving clarity
- The current user-owned worktree changes in `.idea/*`, `external/GameNetworkingSockets`, and `src/client/render/background_renderer.hpp` must be preserved.

### Concrete File Touch List
- `docs/ecs-cleanup-plan.md` (new)
- `docs/ecs-cleanup-phase1-plan.md` (new)
- `docs/context-current.md`
- `docs/multiplayer-architecture.md`
- `docs/multiplayer-runbook.md`
- `src/shared/game/*.hpp` (policy extraction and default alignment)
- `src/shared/net/*.hpp` (policy extraction and named reason codes)
- `src/client/runtime/*.hpp`
- `src/client/runtime/*.cpp`
- `src/client/ui/*.hpp`
- `src/client/render/*.hpp`
- `src/client/input/input_manager.hpp`
- `src/server/runtime/*.hpp`
- `src/server/runtime/*.cpp`
- `src/server/config/*.hpp`
- `src/server/main.cpp`
- `tests/sim/*.cpp`
- `tests/net/*.cpp`

### Acceptance Criteria
- Named policy catalogs exist for shared gameplay, shared networking, client runtime/UI, and server runtime concerns.
- `ClientRuntime` delegates runtime flow, UI interaction, UI document construction, and presentation publication to focused helpers instead of owning the full implementation inline.
- Raw integer disconnect/resync codes used by current runtime flows are replaced by named shared values.
- Existing behavior and public interfaces remain intact:
  - CLI flags
  - config file keys
  - protocol wire shape
  - scene flow
- Validation gate passes:
  - `cmake --preset debug`
  - `cmake --build --preset debug -j`
  - `ctest --preset debug`

### Status
- Complete.

### Progress Update
- Completed work:
  - Created the cleanup roadmap and active phase doc.
  - Switched the living context from the completed runtime-reshape program to the new ECS cleanup program.
  - Added subsystem-owned policy catalogs for:
    - shared gameplay defaults and validation limits
    - shared networking/protocol limits and disconnect/resync reason codes
    - client config/runtime/UI/render/input defaults
    - server config/runtime/session defaults
  - Reduced `ClientRuntime` to a composition/orchestration root and extracted:
    - `ClientRuntimeFlowController`
    - `ClientUiController`
    - `ClientUiDocumentFactory`
    - `ClientPresentationBuilder`
    - `ClientRuntimeContext`
  - Reduced `ServerRuntime` to a thinner orchestration root and extracted:
    - `ServerRuntimeState`
    - `ServerRuntimeContext`
    - `ServerRuntimeOps`
  - Replaced raw disconnect/resync literals on the active runtime path with named shared policy values.
  - Expanded validation coverage with policy-catalog tests, runtime acceptance assertions for UI document policy usage, local-server timeout coverage, and client protocol-mismatch handling coverage.
- Changed files:
  - `docs/ecs-cleanup-plan.md`
  - `docs/ecs-cleanup-phase1-plan.md`
  - `docs/context-current.md`
  - `docs/multiplayer-architecture.md`
  - `docs/multiplayer-runbook.md`
  - `src/shared/game/game_policy.hpp`
  - `src/shared/net/net_policy.hpp`
  - `src/client/core/client_config_policy.hpp`
  - `src/client/runtime/client_runtime_policy.hpp`
  - `src/client/runtime/client_runtime_context.hpp`
  - `src/client/runtime/client_runtime_flow.*`
  - `src/client/runtime/client_ui_controller.*`
  - `src/client/runtime/client_ui_document_factory.*`
  - `src/client/runtime/client_presentation_builder.*`
  - `src/client/ui/ui_policy.hpp`
  - `src/client/render/render_policy.hpp`
  - `src/client/render/render_theme.hpp`
  - `src/client/input/input_policy.hpp`
  - `src/server/config/server_config_policy.hpp`
  - `src/server/runtime/server_runtime_policy.hpp`
  - `src/server/runtime/server_runtime_state.hpp`
  - `src/server/runtime/server_runtime_context.hpp`
  - `src/server/runtime/server_runtime_ops.*`
  - active shared/client/server headers and tests updated to consume the new policies
- Remaining risks/blockers:
  - The largest remaining refinement opportunity is further splitting `MultiplayerSessionService` and `ServerRuntimeOps` if a later cleanup pass wants even narrower files.
  - Full interactive GUI smoke for every menu path still depends on local desktop/input permissions; only automated coverage plus binary boot sanity were rerun in this slice.

### Validation
- `cmake --preset debug`
- `cmake --build --preset debug -j`
- `ctest --preset debug`
- `./build/debug/game_server --port 27021 --tick-rate 30 --snapshot-rate 15`
- `timeout 2 ./build/debug/game_client --skip-splash`
