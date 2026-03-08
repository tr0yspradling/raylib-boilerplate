# Project Feature Guide

This document is the quickest way to evaluate what this boilerplate already provides, what is only partially implemented, and what is still planned. It is written for teams starting a new project on top of this repository.

## Best Fit

Use this repo as a starting point if your project wants:
- a desktop client built with raylib-cpp
- a dedicated authoritative multiplayer server
- a shared deterministic gameplay layer in plain C++
- a menu-driven client flow with local dedicated launch and singleplayer support
- an architecture that already separates client rendering/input from shared simulation and networking

This repo is less complete if your project immediately needs:
- finished content systems such as inventory, crafting, combat, or rich world generation
- production auth/session resume
- completed save/load persistence
- a finished art/atlas pipeline for sprite-driven rendering

## Implemented and Usable Today

### Build, targets, and workflow

- CMake presets are the preferred workflow, with outputs rooted under `build/<preset>`.
- Primary targets are:
  - `shared_game`
  - `shared_net`
  - `client_runtime`
  - `server_runtime`
  - `game_client`
  - `game_server`
- `raylib_boilerplate` exists as a compatibility build target alias to `game_client`; it does not produce a second client binary.
- The repo documents macOS, Linux, and Windows dependency setup and keeps most runtime libraries vendored in `external/`.
- The active developer workflow already assumes configure/build/test through:
  - `cmake --preset debug`
  - `cmake --build --preset debug -j`
  - `ctest --preset debug`

### Client runtime and user flow

- Default startup is `Splash -> Main Menu`.
- Main menu actions are implemented:
  - `Start Server`
  - `Join Server`
  - `Singleplayer`
  - `Options`
  - `Quit`
- `Join Server` opens an editable host/port/name form instead of immediately connecting.
- Keyboard, gamepad, and mouse navigation are implemented for the current menu/join UX.
- `Start Server` launches a sibling `game_server` process, retries localhost until it is ready, and supports cancel/failure return paths with surfaced status text.
- `Singleplayer` runs a local authoritative sandbox without transport or a separate server process.
- `Options` persists client preferences to `client_data/client.cfg` and applies safe live settings immediately where possible.
- Explicit CLI overrides still win over saved preferences at startup.
- Runtime startup helpers are implemented:
  - `--auto-join`
  - `--skip-splash`
- The in-game debug overlay exposes connection and session metrics.

### Multiplayer and networking

- Dedicated authoritative multiplayer is the primary runtime model.
- Valve GameNetworkingSockets is integrated behind `src/shared/net/transport.hpp`.
- Implemented connection/session flow includes:
  - `ClientHello` / `ServerWelcome` handshake
  - server-authenticated player spawn
  - input-frame upload from client to server
  - server-authoritative movement simulation
  - snapshot replication from server to client
  - disconnect and despawn handling
- The client already implements:
  - prediction and reconciliation for the local player
  - interpolation buffers for remote players
  - ping/pong and surfaced connection metrics
- Chunk interest streaming has a working vertical slice:
  - authoritative chunk-interest derivation on the server
  - `WorldMetadata`
  - `ChunkInterestHint`
  - `ChunkBaseline`
  - `ChunkDelta`
  - `ChunkUnsubscribe`
  - `ChunkResyncRequest`
  - `ResyncRequired`

### Shared simulation and server runtime

- Shared gameplay code under `src/shared/game/` remains raylib-free.
- Deterministic simulation foundations already include:
  - fixed-step update helpers
  - player IDs and math/domain types
  - player spawn and input application
  - player movement and jump simulation
  - gameplay snapshot views
  - deterministic chunk-generation helpers and delta helpers
- The dedicated server already has:
  - its own runtime shell under `src/server/`
  - config parsing via `src/server/config/server.cfg`
  - rate-limit knobs for client traffic
  - metrics logging cadence configuration
  - an authoritative simulation/replication loop

### Runtime architecture

- Both active binaries bootstrap through flecs composition roots:
  - `src/client/app/ClientApp`
  - `src/server/app/ServerApp`
- Client phase order is explicit:
  - `InputCapture`
  - `RuntimeIntent`
  - `UiBuild`
  - `UiInteraction`
  - `TransportPoll`
  - `SessionUpdate`
  - `Prediction`
  - `PresentationBuild`
  - `Render`
- Server phase order is explicit:
  - `TransportPoll`
  - `MessageDecode`
  - `AuthAndSession`
  - `InputApply`
  - `Simulation`
  - `Replication`
  - `Persistence`
  - `Metrics`
- Client flow state, local-server startup state, UI state, and multiplayer session state are published through flecs-managed resources.
- `ClientRuntime` already delegates major responsibilities into dedicated services:
  - `MultiplayerSessionService`
  - `SingleplayerSessionService`
  - `OptionsService`
- Rendering is no longer one monolithic path:
  - background, splash, status, and gameplay world rendering use dedicated helpers
  - menu/join rendering uses a `UiDocument` and a dedicated UI renderer

### Validation and operations

- The repo has automated CTest coverage for:
  - serializer and send-policy behavior
  - fixed-step/shared-sim behavior
  - menu and join-form state
  - runtime scene/flow transitions
  - client and server flecs pipeline ordering
  - UI document/state behavior
  - multiplayer, singleplayer, and options services
  - higher-level client runtime acceptance flows
- A windowless runtime acceptance test now covers:
  - `Start Server` launch failure returning to menu with status
  - `Singleplayer` entry and local session publication
  - `Options` save integration and persistence
- Operator guidance exists in:
  - `docs/multiplayer-architecture.md`
  - `docs/multiplayer-runbook.md`
  - `LIVE_TESTING_GUIDE.md`

## Implemented Foundations With Partial Follow-Through

These areas exist in the repo, but should not be treated as finished product features yet.

### Persistence

- Server-side world persistence has a versioned save writer scaffold.
- The live load path is not fully applied back into authoritative runtime state yet.

### Authentication

- Auth is abstracted behind `src/shared/net/auth.hpp`.
- Dev auth works today.
- Production token validation and session resume are scaffolded, not complete.

### Snapshot transport efficiency

- Snapshot delta codec types and message plumbing exist.
- The current online path still sends full snapshot payloads rather than a finished compressed delta stream.

### World/gameplay expansion hooks

- Shared domain files for `world`, `chunk`, `inventory`, `commands`, and `events` exist as foundations.
- Network lane policy already reserves world/gameplay lanes for future traffic.
- This is groundwork for later systems, not evidence that those systems are already complete.

## Planned Follow-On Work

### Asset and rendering pipeline

The main documented content/rendering roadmap is the generic object atlas pipeline described in `docs/object-atlas-pipeline-plan.md`.

Planned work there includes:
- machine-readable object manifests under `assets/specs/objects/`
- validation and code generation tools under `tools/asset_pipeline/`
- generated runtime lookup tables for object specifiers and animation clips
- a generic registry/store for atlas-backed visuals
- render-system integration so gameplay visuals use specifiers instead of primitive placeholders
- initial support for animals, then broader reuse for tiles, props, and interactables
- optional later protocol support for server-authoritative visual IDs

### Gameplay and world systems

Documented future gameplay/world work includes:
- fuller chunk/terrain content workflows beyond the current streaming slice
- more robust resync behavior around chunk delta mismatch scenarios
- inventory, crafting, and building transactions
- combat and projectile authority
- broader local-world and save/load behavior beyond the current singleplayer sandbox

### Persistence, security, and networking depth

Documented future foundation work includes:
- completing persistence load application
- production backend token validation and session resume
- delta compression beyond full snapshot payloads
- broader late-join and replication hardening as world systems grow

### Optional structural cleanup

The planned runtime reshape program itself is complete. Remaining structural cleanup is optional follow-on work, not a blocker for using the repo as-is.

Examples of optional cleanup directions already noted in repo docs:
- further decomposition of transitional runtime classes into narrower services/resources
- continued cleanup of thin render routing as more presentation state becomes explicit
- deeper server-runtime decomposition behind the existing flecs shell

## Current Gaps New Projects Should Plan Around

If you start from this repo today, assume you will still need to own:
- your actual game-specific rules and content
- your art pipeline and runtime asset binding
- your production auth model
- your save/load behavior if persistence matters
- your inventory/combat/building/world-content systems
- any release-style manual GUI acceptance pass you require beyond the current automated runtime coverage

## Suggested Starting Points For New Projects

### Reuse first

- Keep gameplay rules in `src/shared/game/`.
- Add or evolve protocol messages in `src/shared/net/`.
- Treat `src/server/` as the authority boundary.
- Keep raylib-only rendering/input concerns under `src/client/`.

### Customize first

- Replace primitive gameplay rendering with your own asset pipeline or the planned object-specifier pipeline.
- Expand shared simulation types before adding client-only gameplay shortcuts.
- Decide early whether your project needs dedicated-server-only multiplayer, local singleplayer, or both.
- Define your persistence/auth requirements before you treat the existing scaffolds as production-ready.

### Read next

- `docs/multiplayer-architecture.md`
- `docs/multiplayer-runbook.md`
- `docs/runtime-reshape-plan.md`
- `docs/object-atlas-pipeline-plan.md`
