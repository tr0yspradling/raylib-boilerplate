# Multiplayer Architecture

## Scope
This repository now has a dedicated-authoritative multiplayer foundation built around Valve GameNetworkingSockets with a pure shared simulation core.

## Build Targets
- `shared_game` (header-only): deterministic sim types and rules under `src/shared/game/`
- `shared_net` (static): protocol/serialization/transport abstraction + GNS impl under `src/shared/net/`
- `client_runtime` (static): client app/runtime/module layer under `src/client/`
- `server_runtime` (static): server app/runtime/module layer under `src/server/`
- `game_server`: headless authoritative server under `src/server/`
- `game_client`: raylib-cpp client under `src/client/`

## Active Runtime Architecture
- `game_client` boots through `client::app::ClientApp`, which owns a `flecs::world` and a `client::runtime::ClientRuntime`.
- `game_server` boots through `server::app::ServerApp`, which owns a `flecs::world` and a `server::runtime::ServerRuntime`.
- `GameClient` and `GameServer` now act as thin shells over those app roots.
- `ClientRuntime` is now a composition root over focused client helpers rather than a monolithic owner of every phase body.
- `ServerRuntime` is now an orchestration root over focused runtime state/context/ops helpers rather than a single monolithic implementation file.
- Shared gameplay state under `src/shared/game/` remains plain deterministic C++ and is consumed by flecs-driven runtime systems rather than stored directly in ECS.

## Runtime Phase Model
- Client world pipeline:
  - `InputCapture`
  - `RuntimeIntent`
  - `UiBuild`
  - `UiInteraction`
  - `TransportPoll`
  - `SessionUpdate`
  - `Prediction`
  - `PresentationBuild`
  - `Render`
- Server world pipeline:
  - `TransportPoll`
  - `MessageDecode`
  - `AuthAndSession`
  - `InputApply`
  - `Simulation`
  - `Replication`
  - `Persistence`
  - `Metrics`
- Current implementation keeps most business logic inside the runtime service classes while phase ordering and composition are handled by flecs. Deeper state decomposition into ECS resources/components is a planned follow-up.
- Menu/join screen state, input snapshots, UI interaction state, and queued UI commands now live as flecs-managed resources on the client world.
- Runtime control flow now also lives on the client world through explicit resources:
  - `runtime::ClientFlowState` owns runtime mode, splash completion, requested actions, status/disconnect text, and debug overlay toggle
  - `runtime::LocalServerStartupState` owns local dedicated startup ownership/retry timing
- Active client scene publication now derives `core::SceneKind` directly from runtime flow state through a pure helper rather than mutating a `SceneManager`.
- Multiplayer client session state now also lives on the client world through `runtime::ClientSessionState`, which owns the live connection handle/flags, handshake and tick metadata, prediction buffers, remote interpolation state, chunk cache state, and related cadence/debug bookkeeping.
- Multiplayer transport/session orchestration now lives behind `runtime::MultiplayerSessionService`, which owns the transport implementation and mutates the client-world session resource rather than storing duplicate runtime state.
- Singleplayer start/stop/step behavior now lives behind `runtime::SingleplayerSessionService`, which owns the local singleplayer wrapper and publishes local gameplay state through the client-world session resource.
- Options save/apply behavior now lives behind `runtime::OptionsService`, which validates and persists config changes while the UI state remains flecs-managed.
- Client phase orchestration is now split across focused helpers:
  - `ClientRuntimeFlowController`
  - `ClientUiController`
  - `ClientUiDocumentFactory`
  - `ClientPresentationBuilder`
  - `ClientRuntimeContext`
- `ClientRuntime` now exposes a window-independent world-bootstrap seam used by acceptance tests, so runtime flow can be exercised without requiring a live raylib window.
- Non-UI status presentation is now built explicitly during `PresentationBuild`, and concrete drawing is split across dedicated render helpers.
- Local dedicated startup now routes through `src/client/core/server_launcher.*`, which launches a sibling `game_server` process and retries localhost connect until the dedicated server is ready or startup is canceled/timed out.
- Singleplayer now routes through `src/client/core/singleplayer_runtime.*`, which wraps the shared deterministic sim in a transport-free local authoritative sandbox path.
- Options/preferences now route through `src/client/core/config.*` plus UI screen state in the client world, with persisted values loaded from `client_data/client.cfg`.

## Authority Model
- Server owns gameplay truth (`GameState` in `src/shared/game/game_state.hpp`).
- Client sends input/intent only (`InputFrame` currently implemented).
- Server validates input and applies movement in fixed-step.
- Server sends authoritative player kinematics in `ServerWelcome`; client validates and uses that config for prediction/reconciliation.
- Server snapshots override client state; client reconciles by replaying unacked inputs.

## Shared Sim Core
Located in `src/shared/game/`:
- `math_types.hpp`, `ids.hpp`, `fixed_step.hpp`
- `entity.hpp` with deterministic `SimulatePlayerStep`
- `game_state.hpp` with player spawn/input application/tick stepping/snapshot view
- `chunk_streaming.hpp` with deterministic chunk generation and authoritative delta build/apply helpers
- scaffolded domain types: `world.hpp`, `chunk.hpp`, `inventory.hpp`, `commands.hpp`, `events.hpp`

No raylib types exist in shared game/net code.

## Network Stack
Located in `src/shared/net/`:
- `serializer.hpp`: endian-safe, bounds-checked read/write
- `protocol.hpp`: versioned envelope + typed message encode/decode
- `message_ids.hpp`: explicit message ID catalog (implemented + scaffolded)
- `lanes.hpp`: centralized lane policy model
- `send_policy.hpp`: centralized message -> lane/reliability defaults
- `transport.hpp`: transport abstraction
- `transport_gns.hpp/.cpp`: GameNetworkingSockets implementation
- `snapshot.hpp`: snapshot payload encode/decode
- `delta.hpp`: delta codec scaffold (full snapshot passthrough today)
- `auth.hpp`: auth interface with `DevAuthProvider` + production token scaffold
- `net_policy.hpp`: protocol limits, disconnect/resync reason codes, dev transport defaults, and cadence policy

## Policy Catalogs
- Shared gameplay defaults and validation limits are centralized in `src/shared/game/game_policy.hpp`.
- Shared network/protocol limits, dev transport defaults, and disconnect/resync reason codes are centralized in `src/shared/net/net_policy.hpp`.
- Client config/runtime/UI/render/input defaults are centralized in:
  - `src/client/core/client_config_policy.hpp`
  - `src/client/runtime/client_runtime_policy.hpp`
  - `src/client/ui/ui_policy.hpp`
  - `src/client/render/render_policy.hpp`
  - `src/client/render/render_theme.hpp`
  - `src/client/input/input_policy.hpp`
- Server config/runtime/session defaults are centralized in:
  - `src/server/config/server_config_policy.hpp`
  - `src/server/runtime/server_runtime_policy.hpp`
- The cleanup keeps subsystem ownership local instead of creating one cross-repo constants dump.

## Implemented Vertical Slice
- Dedicated server starts and listens.
- Client connects over GNS.
- ClientHello/ServerWelcome handshake.
- Server-authenticated player spawn.
- Input frame send (client -> server).
- Server-authoritative movement.
- Server-configured movement kinematics replicated to clients at handshake.
- Snapshot stream (server -> client).
- Client-side prediction + reconciliation.
- Remote entity interpolation buffer.
- Chunk interest subscription flow:
  - server derives chunk interest from authoritative player position
  - server sends `WorldMetadata` so client can align chunk math
  - client can send `ChunkInterestHint` to request radius tuning
  - client can send `ChunkResyncRequest` for unknown/mismatched chunk deltas
  - server sends `ChunkBaseline` on subscribe, `ChunkDelta` on version advance, and `ChunkUnsubscribe` on interest exit
  - server can issue `ResyncRequired` for invalid chunk-interest input; client clears chunk cache and re-requests
- Despawn/disconnect handling.
- Ping/Pong and runtime connection metrics surfaced in overlay.
- Menu-driven local dedicated flow:
  - `Start Server` launches a sibling `game_server`
  - client remains in `StartingServer` until connect succeeds
  - cancel/failure returns to the menu with surfaced status text
- Menu-driven singleplayer flow:
  - `Singleplayer` starts a local authoritative `GameState`
  - gameplay rendering/input are reused without any transport layer
  - `Esc` returns to the menu
- Menu-driven options flow:
  - `Options` opens a UI-document-based settings screen
  - saved preferences are written to `client_data/client.cfg`
  - explicit CLI flags still override those preferences at startup

## Lanes and Reliability
Lane policy in `src/shared/net/lanes.hpp`:
- Lane 0 `Control`: reliable ordered
- Lane 1 `Input`: unreliable
- Lane 2 `Snapshot`: unreliable
- Lane 3 `World`: reliable ordered (scaffolded traffic)
- Lane 4 `Gameplay`: reliable ordered (scaffolded traffic)

Configured per-connection in `transport_gns.cpp` via `ConfigureConnectionLanes`.

## Runtime Layout
- `src/client/app/`: `ClientApp` composition root and frame loop.
- `src/client/modules/`: client flecs phase declarations and runtime module registration.
- `src/client/runtime/`: heavyweight client runtime behavior, still transitional while state is decomposed further.
- `src/client/runtime/client_runtime_flow.*`: runtime intent/transition handling and local dedicated startup policy.
- `src/client/runtime/client_ui_controller.*`: focus/navigation/edit-command handling.
- `src/client/runtime/client_ui_document_factory.*`: published UI document construction.
- `src/client/runtime/client_presentation_builder.*`: world/status/debug presentation publication.
- `src/client/runtime/client_runtime_context.hpp`: shared runtime dependency/context bundle for the extracted helpers.
- `src/client/runtime/runtime_resources.hpp`: client-world flow/control/session resources and transition helpers.
- `src/client/runtime/multiplayer_session_service.*`: extracted multiplayer transport/session service used by `ClientRuntime`.
- `src/client/runtime/singleplayer_session_service.*`: extracted singleplayer service used by `ClientRuntime`.
- `src/client/runtime/options_service.*`: extracted options validation/persistence/application service used by `ClientRuntime`.
- `src/client/components/`: render/debug presentation state published into the client world.
- `src/client/core/`: transitional runtime state, command enums, and scene mapping.
- `src/client/core/server_launcher.*`: local dedicated launcher abstraction + process implementation used by the current `Start Server` flow.
- `src/client/core/singleplayer_runtime.*`: transport-free local authoritative wrapper used by the current `Singleplayer` flow.
- `src/client/core/config.*`: client preference load/save helpers used by startup and the current `Options` flow.
- `src/client/input/`: input capture producing shared input frames.
- `src/client/physics/`: prediction/reconciliation helpers on shared sim.
- `src/client/scenes/`: scene captions and metadata still used by current presentation flow.
- `src/client/render/`: background, splash, status, and gameplay world render helpers.
- `src/client/systems/`: top-level render routing and related presentation helpers.
- `src/client/ui/`: debug overlay, UI state/document resources, and UI rendering helpers.
- `src/server/app/`: `ServerApp` composition root and process loop.
- `src/server/modules/`: server flecs phase declarations and runtime module registration.
- `src/server/runtime/`: heavyweight authoritative server runtime behavior, still transitional while services are decomposed further.
- `src/server/runtime/server_runtime_state.hpp`: extracted authoritative session/chunk container types.
- `src/server/runtime/server_runtime_context.hpp`: shared runtime dependency/context bundle for extracted server ops.
- `src/server/runtime/server_runtime_ops.*`: focused transport/session/decode/replication/metrics helpers used by `ServerRuntime`.
- `src/server/config/`: config parsing and defaults.

## Validation Coverage
- Existing gameplay/network tests still cover serializer, send policy, fixed step, menu/join state, and runtime scene transitions.
- Policy catalog tests now cover the extracted gameplay/network/client/server defaults directly.
- Runtime scene-transition tests now validate the pure runtime-to-scene mapping directly.
- New world-level tests verify flecs phase ordering for both client and server runtimes.
- UI state/document tests now cover menu state, join-form state, and document hit-testing/focus traversal helpers.
- Status presentation tests now cover non-UI status-screen mapping.
- Client runtime resource tests now cover flow-resource transitions and session-resource reset semantics.
- The extracted multiplayer service now has a focused fake-transport test covering connection events, `ClientHello` dispatch, and protocol-mismatch disconnect handling.
- The extracted singleplayer service now has a focused test covering start/step/stop behavior and session-state publication.
- The extracted options service now has a focused test covering validation, persistence, and live-safe apply behavior.
- A runtime acceptance test now covers menu-driven `Start Server` failure, local-server timeout, `Singleplayer` entry, `Options` save, and UI document policy publication against the real `ClientRuntime` flow without opening a live window.

## Persistence and Security Status
Implemented foundations:
- Config-driven server settings (`src/server/config/server.cfg`)
- Versioned save writer scaffold and load hook (`src/server/world_persistence.cpp`)
- Basic server input sanity checks and flood caps
- Auth interface abstraction with explicit dev vs production modes

Scaffolded/TODO (not claimed complete):
- Full chunk streaming + terrain deltas
- robust resync-on-delta-mismatch path
- Inventory/crafting/build transactions
- Combat/projectile authority model
- Late-join chunk baselines
- Production backend token validation/session resume
- Delta compression beyond full snapshot payloads
