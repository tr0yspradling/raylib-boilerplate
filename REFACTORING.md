# Runtime Architecture Notes

This repository is no longer a generic scene-driven starter kit. It is an
authoritative multiplayer prototype whose active client and server entrypoints
now bootstrap through flecs worlds.

## High-Level Goals

- Keep the server authoritative and the shared simulation deterministic.
- Use flecs as the active runtime composition layer for both client and server.
- Keep rendering/input raylib-only on the client.
- Continue decomposing menu/UI and rendering responsibilities out of the
  current transitional runtime classes.

## Directory Guide

```
src/
├── client/
│   ├── app/            # ClientApp flecs composition root
│   ├── modules/        # Client runtime phases and module registration
│   ├── runtime/        # Transitional heavyweight client runtime logic
│   ├── core/           # Transitional runtime state, menu state, local server launcher
│   ├── components/     # Client presentation/debug state
│   ├── input/          # Input capture helpers
│   ├── physics/        # Prediction/reconciliation helpers
│   ├── scenes/         # Transitional scene metadata/captions
│   ├── systems/        # Current render path
│   └── ui/             # Debug overlay helpers
├── server/
│   ├── app/            # ServerApp flecs composition root
│   ├── modules/        # Server runtime phases and module registration
│   ├── runtime/        # Transitional heavyweight server runtime logic
│   └── config/         # Dedicated server config
└── shared/
    ├── game/           # Deterministic simulation, no raylib types
    └── net/            # Protocol, serializer, transport abstraction, GNS
```

## Core Flow

1. `ClientApp` owns a `flecs::world`, registers client runtime phases, and
   drives `world.progress(...)` each frame.
2. `ServerApp` owns a `flecs::world`, registers server runtime phases, and
   drives `world.progress(...)` in the dedicated server loop.
3. Runtime modules invoke the current `ClientRuntime` / `ServerRuntime`
   services in explicit phase order.
4. Shared simulation and protocol logic remain plain C++ and are consumed by
   those runtime services.
5. The old `RuntimeState + SceneManager` model remains in place only as a
   transitional client screen-state layer and should continue shrinking.
6. Menu/join UI state now lives in flecs-managed resources and is rendered
   through a built `UiDocument`.
7. Splash, centered status, and gameplay world drawing now live in dedicated
   render helpers with a thin render router above them.
8. `Start Server` now launches a sibling dedicated server process through the
   current `server_launcher` abstraction and retries localhost connect until
   the server is ready.

## Current Client Phase Order

- `InputCapture`
- `RuntimeIntent`
- `UiBuild`
- `UiInteraction`
- `TransportPoll`
- `SessionUpdate`
- `Prediction`
- `PresentationBuild`
- `Render`

## Current Server Phase Order

- `TransportPoll`
- `MessageDecode`
- `AuthAndSession`
- `InputApply`
- `Simulation`
- `Replication`
- `Persistence`
- `Metrics`

## Immediate Follow-Up Work

- Replace the current menu model with a UI document/widget layer that supports
  full state decomposition into flecs resources/components rather than the
  remaining transitional runtime ownership.
- Move more client runtime/session state into explicit flecs resources/components
  instead of relying on transitional scene-switch mapping.
- Replace the remaining placeholder runtime flows (`Singleplayer`, `Options`)
  with real implementations on top of the current UI/document path.
- Continue removing the remaining transitional routing from the thin render
  coordinator once more presentation state is explicit.
- Decompose the server runtime into narrower session/router/replication-style
  services behind the flecs shell.

## Build + Run

```bash
cmake --preset debug
cmake --build --preset debug -j
ctest --preset debug
```

Run the active binaries from `build/debug/`:

```bash
./build/debug/game_server
./build/debug/game_client
```
