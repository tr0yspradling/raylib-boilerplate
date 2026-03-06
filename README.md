# raylib boilerplate

A dedicated-authoritative multiplayer prototype built on raylib-cpp, GameNetworkingSockets, and flecs.
The active client and server runtimes now bootstrap through flecs composition roots while the shared
simulation stays plain deterministic C++ under `src/shared/game/`.

## Highlights

- **Authoritative multiplayer:** server owns gameplay truth; client predicts and reconciles.
- **Flecs-first runtime shells:** `game_client` and `game_server` run through explicit flecs world phases.
- **Shared deterministic sim:** gameplay rules remain raylib-free under `src/shared/game/`.
- **Preset-backed workflow:** build, test, and runtime outputs live under `build/<preset>/`.

## Controls

- `W/S` or arrow keys – menu navigation.
- `Enter` / `Space` – menu select.
- `Esc` – menu back / cancel join.
- `Esc` – return from placeholder/disconnected screens.
- `A/D` or arrows – move (multiplayer gameplay).
- `Space` – jump (multiplayer gameplay).
- `Tab` – toggle net debug overlay.

## Build

### 1) Sync submodules

```bash
git submodule update --init --recursive
```

### 2) Configure + build

Preferred (CMake presets):

```bash
cmake --preset debug
cmake --build --preset debug -j
```

Run tests:

```bash
ctest --preset debug
```

Direct equivalent (single-config generators):

```bash
cmake -S . -B build/debug -DCMAKE_BUILD_TYPE=Debug
cmake --build build/debug -j
```

Direct equivalent (multi-config generators like Visual Studio):

```bash
cmake -S . -B build/debug
cmake --build build/debug --config Debug
```

### 3) Run

Single-config:

```bash
./build/debug/game_client
```

By default the client boots into splash/menu and waits for a user menu selection before joining a server.
For direct dev join flow:

```bash
./build/debug/game_client --auto-join --skip-splash
```

Multi-config (Windows/MSVC):

```powershell
.\build\debug\Debug\game_client.exe
```

JetBrains/CLion: use preset-backed CMake profiles (`debug`, `release`) so profile outputs and target matrix data remain under `build/<profile>/`.

Run a dedicated server:

```bash
./build/debug/game_server --port 27020 --tick-rate 30 --snapshot-rate 15
```

## Dependencies

Most runtime libraries are vendored in `external/` (`raylib`, `raylib-cpp`, `flecs`, `imgui`, `rlImGui`, `GameNetworkingSockets`).
System packages are still required for toolchains and a few native dependencies (`OpenSSL`, `Protobuf`, and modern `Protobuf` runtime friends like `abseil`).

### macOS (Homebrew)

```bash
brew install cmake ninja pkg-config openssl protobuf abseil
```

Notes:
- `protobuf` on Homebrew depends on `abseil`; keeping both installed avoids missing link/runtime symbols.
- `raylib` is only needed if you disable vendored deps (`-DUSE_VENDORED_DEPS=OFF`).

### Linux (Ubuntu/Debian)

```bash
sudo apt-get update
sudo apt-get install -y \
  build-essential cmake ninja-build pkg-config \
  libssl-dev protobuf-compiler libprotobuf-dev libabsl-dev \
  libasound2-dev libx11-dev libxrandr-dev libxi-dev libxcursor-dev libxinerama-dev \
  libgl1-mesa-dev libwayland-dev libxkbcommon-dev
```

Notes:
- X11/OpenGL/audio development headers are needed by desktop raylib builds.
- `raylib` dev package is optional when using vendored deps.

### Windows (MSVC + vcpkg recommended)

Install:
- Visual Studio 2022 with `Desktop development with C++`
- CMake and Ninja (or use Visual Studio generator)
- [vcpkg](https://github.com/microsoft/vcpkg)

Then install native deps:

```powershell
vcpkg install openssl protobuf abseil --triplet x64-windows
```

Configure with vcpkg toolchain:

```powershell
cmake -S . -B build `
  -DCMAKE_TOOLCHAIN_FILE=C:/path/to/vcpkg/scripts/buildsystems/vcpkg.cmake `
  -DVCPKG_TARGET_TRIPLET=x64-windows
```

## Current Architecture Direction

- `src/client/app/` and `src/server/app/` contain the flecs composition roots.
- `src/client/modules/` and `src/server/modules/` define explicit runtime phase ordering.
- `src/client/runtime/` and `src/server/runtime/` currently hold the transitional heavyweight runtime logic.
- Menu/UI and rendering decomposition are still in progress; the current menu stack is functional but not the final design.
