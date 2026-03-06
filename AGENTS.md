# AGENTS

Single source of truth for OpenAI agents and Claude Code. Link or symlink `CLAUDE.md` to this file.

---

## Build and Run

**Canonical out-of-source flow**

- Configure: `cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug`
- Build: `cmake --build build -j`
- Run: `./build/game_client`
- Clean: `cmake --build build --target clean`

**IDE sandbox or existing in-source build**

- Build: `cmake --build cmake-build-debug`
- Run: `./cmake-build-debug/game_client`
- Clean: `cmake --build cmake-build-debug --target clean`

**Notes**

- Works with Ninja and default CMake generators. Prefer the canonical out-of-source flow when possible.
- Ensure submodules are present: `git submodule update --init --recursive`.
- raylib development files must be available locally. Other deps (raylib-cpp, ImGui, rlImGui, flecs) are vendored.

---

## Project Structure

- Source entry: `src/main.cpp` (C++23).
- ECS components live in `src/components/`. Core app + scene plumbing is under `src/core/`.
- Scene implementations live in `src/scenes/` (menu + sandbox examples).
- Systems (movement, rendering, overlays) live under `src/physics/`, `src/systems/`, and `src/ui/`.
- Dependencies are managed via vendored submodules in `external/`.

---

## Architecture Overview

**Libraries and language**

- raylib for windowing and rendering (via raylib-cpp wrapper).
- flecs for the ECS world.
- ImGui + rlImGui for UI.
- C++23 with STL containers.

**Core boilerplate**

- ECS components: `Transform`, `Velocity`, `Spin`, `ColorTint`, `Drawable`, `Pulse`.
- Systems: `physics::MovementSystem` (translation + spin + pulse) and `systems::RenderSystem` (batched rectangle drawing).
- Two starter scenes: a minimal menu and a sandbox scene with animated tiles.
- Debug overlay reports FPS, delta time, and active scene.

**Key patterns**

- `SceneManager` controls lifetime/activation for scenes.
- Scenes own their own flecs world and camera state.
- Input is centralized via `InputManager` so future bindings can share one interface.
- Rendering uses raylib-cpp cameras/rectangles; UI overlay is ready for ImGui panels.

---

## Code Style and Naming

- Format with `.clang-format` (LLVM base, 4 spaces, 120 columns).
- Run: `clang-format -i src/*.cpp src/*.h*`
- Naming:
  - Types and ECS components: `PascalCase` (e.g., `Transform`, `SceneManager`).
  - Functions: `CamelCase` (e.g., `Register`, `SwitchTo`).
  - General variables: `lowerCamelCase`.
  - Constants and Config fields: `snake_case` (e.g., `target_fps`, `window_flags`).
- Keep physics, UI, and rendering concerns in separate files.

---

## Testing

- No automated tests yet; validate by building and running.
- Manual checks: switch between menu and sandbox scenes, pan/zoom the sandbox camera, confirm entities animate and the overlay stays responsive.
- If adding tests, prefer lightweight CTest targets via CMake.

---

## Contribution

- Commits: imperative mood, concise subject, optional scope. Example: `physics: cap velocity in Verlet step`.
- Group related changes and keep diffs focused; include brief rationale if behavior changes.
- PRs: include build/run steps, platform notes, and screenshots or gifs for UI changes.
- Ensure code is formatted, builds cleanly, and runs without runtime errors before review.

---

## Platform Notes

- Install raylib. Examples: macOS `brew install raylib`; Ubuntu `sudo apt install libraylib-dev`.
- macOS frameworks are handled in CMake. Linux links `m`, `pthread`, `GL`, `dl`, and `X11`.
