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
│   ├── core/           # Transitional runtime state + menu state
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
  mouse hover/click alongside keyboard/gamepad navigation.
- Move client screen/runtime state into explicit flecs resources/components
  instead of relying on transitional scene-switch mapping.
- Split the monolithic render path into separate world, UI, and debug
  presentation modules.
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
