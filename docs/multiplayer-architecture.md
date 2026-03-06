# Multiplayer Architecture

## Scope
This repository now has a dedicated-authoritative multiplayer foundation built around Valve GameNetworkingSockets with a pure shared simulation core.

## Build Targets
- `shared_game` (header-only): deterministic sim types and rules under `src/shared/game/`
- `shared_net` (static): protocol/serialization/transport abstraction + GNS impl under `src/shared/net/`
- `game_server`: headless authoritative server under `src/server/`
- `game_client`: raylib-cpp client under `src/client/`

## Thread Model
- Single-threaded networking + simulation loop on both client and server.
- Server: `Poll transport -> process messages -> fixed-step sim -> snapshot broadcast`.
- Client: `input -> poll transport -> fixed-step prediction -> render`.

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

## Re-Architecture of Original Folders
Original folder set is now network-client oriented:
- `src/client/components/`: render/debug view components for net client state
- `src/client/core/`: client scene-phase state and runtime phase updates
- `src/client/input/`: input capture producing shared input frames
- `src/client/physics/`: prediction/reconciliation helpers on shared sim
- `src/client/scenes/`: client scene caption/state metadata for overlay flow
- `src/client/systems/`: frame renderer consuming authoritative/predicted/interpolated state
- `src/client/ui/`: multiplayer debug overlay (scene/net/tick/metrics/disconnect)

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
