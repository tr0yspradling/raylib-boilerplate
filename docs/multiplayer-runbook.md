# Multiplayer Runbook

Runtime/menu reshape implementation plan: `docs/runtime-reshape-plan.md`.
Object atlas pipeline implementation plan: `docs/object-atlas-pipeline-plan.md`.

## Prerequisites

### macOS (Homebrew)

```bash
brew install cmake ninja pkg-config openssl protobuf abseil
```

### Linux (Ubuntu/Debian)

```bash
sudo apt-get update
sudo apt-get install -y \
  build-essential cmake ninja-build pkg-config \
  libssl-dev protobuf-compiler libprotobuf-dev libabsl-dev \
  libasound2-dev libx11-dev libxrandr-dev libxi-dev libxcursor-dev libxinerama-dev \
  libgl1-mesa-dev libwayland-dev libxkbcommon-dev
```

### Windows (MSVC + vcpkg)

Install:
- Visual Studio 2022 (`Desktop development with C++`)
- CMake + Ninja (or Visual Studio generator)
- vcpkg

Install native deps:

```powershell
vcpkg install openssl protobuf abseil --triplet x64-windows
```

Configure with toolchain:

```powershell
cmake -S . -B build/debug `
  -DCMAKE_TOOLCHAIN_FILE=C:/path/to/vcpkg/scripts/buildsystems/vcpkg.cmake `
  -DVCPKG_TARGET_TRIPLET=x64-windows
```

## Build (Preferred Profile)
Configure (CMake preset):
```bash
cmake --preset debug
```
Build (single-config generators):
```bash
cmake --build --preset debug -j
```
Build (multi-config generators like Visual Studio):
```bash
cmake --build --preset debug --config Debug
```
Run tests:
```bash
ctest --preset debug
```
JetBrains/CLion:
- Use preset-backed CMake profiles (`debug`, `release`) to keep profile target matrices and generated files under `build/<profile>/`.

## Run Dedicated Server
Default config file: `src/server/config/server.cfg`

Start server:
```bash
./build/debug/game_server --port 27020 --tick-rate 30 --snapshot-rate 15
```

## Run Local Clients
Client 1:
```bash
./build/debug/game_client --host 127.0.0.1 --port 27020 --name alice
```
Client 2:
```bash
./build/debug/game_client --host 127.0.0.1 --port 27020 --name bob
```
After launch, use the main menu:
- Select `Join Server` to open the join form.
- Edit host/port/name as needed, then select `Connect`.

Direct auto-join (dev shortcut):
```bash
./build/debug/game_client --host 127.0.0.1 --port 27020 --name alice --auto-join --skip-splash
```

## Runtime Notes
- Both `game_client` and `game_server` now bootstrap through `flecs::world` composition roots.
- Client runtime phase order is validated by `test_sim_client_pipeline`.
- Server runtime phase order is validated by `test_sim_server_pipeline`.
- Menu/join UI state now lives in flecs-managed resources and is rendered from a built `UiDocument`.
- The current UI/presentation stack is still transitional overall: gameplay/status/debug presentation remains on the older render path behind the new flecs shell.

## Controls
- Move: `A/D` or arrows
- Jump: `Space`
- Toggle net debug overlay: `Tab`
- Menu navigate: `W/S` or arrows (gamepad D-pad up/down)
- Menu select: `Enter`/`Space` (gamepad south face button)
- Menu back: `Esc` (gamepad east face button)
- Menu / join mouse support: hover and left-click
- Join form editing: select `Host`/`Port`/`Name`, type text, `Backspace` to erase, `Enter`/`Esc` to stop editing
- While connecting: `Esc` cancels and returns to menu
- Return from placeholder/disconnected screens: `Enter` or `Esc`

## Smoke Test Checklist
1. Start `game_server`.
2. Start two clients, open `Join Server`, verify join-form navigation/editing, then connect both clients.
3. Confirm each client sees both players moving.
4. Confirm local movement remains responsive (prediction).
5. Confirm remote movement is smoothed (interpolation).
6. Confirm overlay shows ping/queue/throughput values.
7. Confirm overlay chunk counters update (loaded chunks, version conflicts).
8. Close one client and confirm despawn/disconnect behavior on the other.

If you are validating the Phase 4 flecs foundation specifically, also confirm:
1. `game_server` starts and accepts connections with no runtime regressions.
2. `game_client` still reaches splash/menu and the join form through the new app shell.
3. Automated phase-order tests pass before manual GUI checks.
4. Menu and join controls work by keyboard/gamepad and by mouse hover/click.

## Config Notes
`src/server/config/server.cfg` supports:
- `listen_port`, `simulation_tick_hz`, `snapshot_rate_hz`
- `max_clients`
- per-client rate caps: `max_input_frames_per_second`, `max_chunk_hints_per_second`, `max_chunk_resync_requests_per_second`
- server metrics logging cadence: `metrics_log_interval_seconds` (`0` disables periodic logs)
- world/chunk tuning: `world_chunk_width_tiles`, `world_chunk_height_tiles`, `world_tile_size`, `world_interest_radius_chunks`
- `auth_mode` (`dev` currently functional, `token` scaffold)
- `persistence_path`
- player kinematics: `player_max_move_speed`, `player_jump_speed`, `player_gravity`, `player_max_fall_speed`, `player_ground_y`, `world_min_x`, `world_max_x`
- local net sim knobs: `fake_lag_ms`, `fake_jitter_ms`, `fake_loss_send_pct`, `fake_loss_recv_pct`

## Known Current Limitations
- Persistence load is scaffolded (save implemented, load hook TODO).
- Snapshot transport currently sends full payloads (delta codec scaffold).
- Chunk/terrain/inventory/crafting/combat authority paths are scaffolded interfaces, not final gameplay implementations.
