# Runtime Reshape: Phase 1 Execution Plan

## Scope
Implement the runtime/scene foundation only:
- explicit runtime state model
- expanded scene taxonomy for upcoming menu flow
- scene updates driven from runtime state (not raw connection booleans)
- no menu UI yet (that is Phase 2)

## Goals
1. Keep current multiplayer behavior working.
2. Introduce clean state primitives for splash/menu/singleplayer/multiplayer expansion.
3. Land in small compile-safe steps.

## Implementation Steps

## Step 1: Expand Scene Model
- Update `src/client/core/scene.hpp`:
  - add new scene kinds:
    - `MainMenu`
    - `JoinServer`
    - `StartingServer`
    - `GameplayMultiplayer`
    - `GameplaySingleplayer`
    - `Options`
  - keep existing `Splash`, `Connecting`, `Disconnected`
- Ensure `SceneName(...)` covers all enum values.

## Step 2: Add Runtime State Type
- Create `src/client/core/runtime_state.hpp` with:
  - `RuntimeMode` enum:
    - `Boot`
    - `Menu`
    - `JoiningServer`
    - `StartingLocalServer`
    - `Multiplayer`
    - `Singleplayer`
    - `Options`
    - `Disconnected`
  - `RuntimeState` struct:
    - `mode`
    - `splashCompleted`
    - `requestedJoin`
    - `requestedLocalServerStart`
    - `requestedSingleplayer`
    - `requestedOptions`
    - `disconnectReason`
- Keep fields minimal and neutral to avoid locking UI design prematurely.

## Step 3: Add Menu Action Model
- Create `src/client/core/menu_model.hpp` with:
  - `MenuAction` enum (`StartServer`, `JoinServer`, `Singleplayer`, `Options`, `Quit`, `None`)
  - `MenuSelectionState` struct (selected index + action list helpers)
- This is logic-only scaffolding for Phase 2 rendering/input hookup.

## Step 4: Refactor Scene Transition Logic
- Refactor `src/client/core/application.hpp`:
  - replace current `UpdateScene(SceneManager&, bool, bool, bool, const std::string&)`
  - with `UpdateScene(SceneManager&, const RuntimeState&)`
  - map runtime mode -> scene deterministically.

## Step 5: Wire GameClient to RuntimeState
- Update `src/client/game_client.hpp/.cpp`:
  - add `core::RuntimeState runtimeState_{};`
  - mirror current behavior into runtime state values:
    - bootstrap -> connecting -> multiplayer/disconnected
  - call `Application::UpdateScene(sceneManager_, runtimeState_)`
- Keep network connect behavior as-is in this phase (still eager), but route scene state through `RuntimeState`.

## Step 6: Build + Test + Smoke
- Build:
  - `cmake --build cmake-build-debug -j`
- Tests:
  - `ctest --test-dir cmake-build-debug --output-on-failure`
- Manual smoke:
  - launch client and verify scene text changes still work for connect/disconnect flow.

## Deliverables
- New files:
  - `src/client/core/runtime_state.hpp`
  - `src/client/core/menu_model.hpp`
- Modified files:
  - `src/client/core/scene.hpp`
  - `src/client/core/application.hpp`
  - `src/client/game_client.hpp`
  - `src/client/game_client.cpp`
  - `CMakeLists.txt` (add new headers to `client_runtime` list)

## Out of Scope (Phase 2+)
- interactive main menu rendering/layout
- join form editing UX
- local server process launch
- singleplayer runtime execution path
- options persistence
