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

1. **Startup Flow**
   - App starts on splash, then transitions to main menu without connecting.
   - `--skip-splash` starts directly at main menu.
2. **Menu Navigation**
   - `W/S` or arrows (or gamepad D-pad) changes selection.
   - `Enter`/`Space` (or gamepad south button) activates selected action.
3. **Join + Quit**
   - Selecting `Join Server` begins connection flow.
   - Selecting `Quit` exits cleanly.
4. **Placeholder Routes**
   - Selecting `Start Server`, `Singleplayer`, or `Options` shows placeholder messaging.
   - `Enter` or `Esc` returns to main menu.
5. **Debug Overlay**
   - Overlay text renders in the top-left corner.
   - Scene/state text updates as you switch splash/menu/join/placeholder states.

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
