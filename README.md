# raylib boilerplate

A tiny scene-driven starter kit for raylib + raylib-cpp, flecs, ImGui, and rlImGui.  
The old N-body project has been removed in favor of a reusable structure that makes it easy to
swap scenes, wire up systems, and prototype gameplay.

## Highlights

- **Scene manager:** register scenes once and switch with one line of code.
- **raylib-cpp everywhere:** the window, camera, input, and drawing APIs all flow through the C++ wrappers.
- **ECS-ready:** flecs powers the example sandbox scene with movement, spin, and render systems.
- **UI hook:** rlImGui and ImGui are still configured so overlays can be added immediately.

## Controls

- `Space / Enter` – from the menu, jump into the sandbox scene.
- `WASD` / arrow keys – pan the sandbox camera.
- Mouse wheel – zoom the sandbox camera.
- `Esc` – return to the menu.

## Build

```bash
git submodule update --init --recursive
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j
./build/raylib_boilerplate
```

## Dependencies

- raylib – rendering/windowing
- raylib-cpp – idiomatic C++ bindings for raylib
- flecs – entity component system
- ImGui + rlImGui – immediate mode UI bridge (ready for future panels)

## Next steps

- Add more scenes (loading screen, gameplay, pause/menu, etc.).
- Expand the ECS components/systems to fit your game's needs.
- Hook ImGui panels into the debug overlay or create dedicated UI scenes.
