# Live Testing Guide

This repo is intentionally small. Use the checklist below any time you tweak
the boilerplate to make sure the basics still work.

## Build + Run

```bash
git submodule update --init --recursive
cmake --preset debug
cmake --build --preset debug -j
./build/debug/game_client
```

For CLion/IDE builds, use the `debug` preset profile so generated files and target matrix output stay under `build/debug`.

## Manual Test Pass

1. **Menu Scene**
   - App starts on the menu.
   - Press `Space` or `Enter` and confirm the sandbox scene loads.
2. **Sandbox Camera**
   - `WASD` / arrow keys pan the view.
   - Mouse wheel zooms in/out (clamped between 0.25–3.0).
   - `Esc` returns to the menu without crashing or leaking.
3. **ECS Animation**
   - The colored tiles move/rotate smoothly.
   - Pulse animation scales entities and resumes when returning from the menu.
4. **Debug Overlay**
   - Overlay text renders in the top-left corner.
   - Scene name and FPS update every frame.

## Quick Commands

```bash
# Rebuild after a clean-up
cmake --build --preset debug -j --target game_client

# Run tests
ctest --preset debug

# Remove generated build profiles
rm -rf build/debug build/release
```

## Troubleshooting

- **Missing deps** – run `git submodule update --init --recursive`.
- **Window fails to open** – ensure the raylib development headers/library are installed locally.
- **Link errors** – check that cmake found the vendored submodules (see `external/`).
- **Weird input** – reset focus; the InputManager is a thin wrapper over raylib so OS-level shortcuts still apply.
