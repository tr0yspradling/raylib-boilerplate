# Boilerplate Architecture Notes

The repository now ships as a lightweight template for future raylib projects.
This document captures the moving pieces that replaced the original
N-body project.

## High-Level Goals

- Use **raylib-cpp** for all engine-level interactions (window, camera, input, drawing).
- Keep **flecs** in the stack so entities/systems scale with the project.
- Provide two tiny scenes (menu + sandbox) to demonstrate scene switching.
- Leave ImGui/rlImGui configured so UI layers can be dropped in without
  touching the build.

## Directory Guide

```
src/
├── components/         # Shared ECS components (Transform, Velocity, etc.)
├── core/               # Application, config, scene contracts, scene manager
├── input/              # InputManager snapshotting keyboard + mouse state
├── physics/            # ECS systems that modify world data
├── scenes/             # Concrete scenes (MenuScene, SandboxScene)
├── systems/            # Rendering logic that reads ECS data
└── ui/                 # Debug overlay + future UI helpers
```

## Core Flow

1. `Application` boots a `raylib::Window`, owns the `InputManager`, and
   runs the main loop.
2. `SceneManager` stores `Scene` instances and exposes `SwitchTo`.
3. Each `Scene` gets a `SceneContext` containing references to the window,
   input manager, and the scene manager itself (so scenes can request transitions).
4. Scenes own any state they need—in the sandbox that means a `flecs::world`
   plus a `raylib::Camera2D`.
5. Systems (movement, render, overlay) are just header-only helpers that
   operate on these worlds/cameras.

## Adding a Scene

1. Create `src/scenes/MyScene.hpp` and inherit from `Scene`.
2. Override `OnEnter`, `Update`, `Draw`, and `Name`.
3. Register the scene in `main.cpp`:
   ```cpp
   app.RegisterScene<MyScene>("my_scene");
   ```
4. Switch to it from anywhere via `context.manager.SwitchTo("my_scene")`.

## Extending ECS Data

- Add new structs to `src/components/Components.hpp`.
- Register systems in `physics::MovementSystem` or create new headers under
  `src/physics/` or `src/systems/`.
- Make sure scenes call your register functions during `OnEnter`.

## Input Tips

- `InputManager` intentionally exposes only a handful of helpers. Extend it
  when you need buffered states, rebinding, or action maps.
- Raylib key/mouse enumerations are available via `raylib-cpp.hpp`.

## UI / Debug Panels

- `ui::DebugOverlay` draws the FPS + current scene. Hook ImGui panels here or
  create scene-specific UI once rlImGui is initialized.

## Build + Run

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j
./build/raylib_boilerplate
```

The `external/` directory still expects initialized submodules for
raylib, raylib-cpp, flecs, ImGui, and rlImGui.
