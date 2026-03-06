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
- `src/client/components/`: render/debug presentation state published into the client world.
- `src/client/core/`: transitional runtime state, command enums, and scene mapping.
- `src/client/input/`: input capture producing shared input frames.
- `src/client/physics/`: prediction/reconciliation helpers on shared sim.
- `src/client/scenes/`: scene captions and metadata still used by current presentation flow.
- `src/client/systems/`: current renderer and related presentation helpers.
- `src/client/ui/`: debug overlay, UI state/document resources, and UI rendering helpers.
- `src/server/app/`: `ServerApp` composition root and process loop.
- `src/server/modules/`: server flecs phase declarations and runtime module registration.
- `src/server/runtime/`: heavyweight authoritative server runtime behavior, still transitional while services are decomposed further.
- `src/server/config/`: config parsing and defaults.

## Validation Coverage
- Existing gameplay/network tests still cover serializer, send policy, fixed step, menu/join state, and runtime scene transitions.
- New world-level tests verify flecs phase ordering for both client and server runtimes.
- UI state/document tests now cover menu state, join-form state, and document hit-testing/focus traversal helpers.

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
