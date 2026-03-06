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

Multi-config (Windows/MSVC):

```powershell
.\build\debug\Debug\game_client.exe
```

JetBrains/CLion: use preset-backed CMake profiles (`debug`, `release`) so profile outputs and target matrix data remain under `build/<profile>/`.

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

## Next steps

- Add more scenes (loading screen, gameplay, pause/menu, etc.).
- Expand the ECS components/systems to fit your game's needs.
- Hook ImGui panels into the debug overlay or create dedicated UI scenes.
