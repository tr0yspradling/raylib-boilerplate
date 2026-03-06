# Runtime Reshape Plan

Phase 1 execution breakdown: `docs/runtime-phase1-plan.md`.

## Objective
Reshape the client runtime flow from the current network-first bootstrap into a game-first flow:

1. `Splash`
2. `Main Menu`
3. User chooses:
   - `Start Server` (local dedicated)
   - `Join Server`
   - `Singleplayer`
   - `Options`
   - `Quit`

This plan preserves the authoritative multiplayer architecture and keeps raylib usage client-only.

## Current Baseline
- Phase 1 foundation is complete:
  - expanded scene taxonomy (`Splash`, `MainMenu`, `JoinServer`, `StartingServer`, `Connecting`,
    `GameplayMultiplayer`, `GameplaySingleplayer`, `Options`, `Disconnected`)
  - `RuntimeMode` / `RuntimeState` and `MenuAction` / `MenuSelectionState` scaffolding are in place
  - scene transitions route through `Application::UpdateScene(SceneManager&, const RuntimeState&)`
- Build workflow is standardized on presets under `build/<preset>`.
- Phase 2A is complete:
  - startup defaults to splash -> main menu (no eager connect)
  - interactive menu navigation/action dispatch is wired
  - `Join Server` and `Quit` are fully wired
  - `Start Server` / `Singleplayer` / `Options` currently route to placeholders
- Phase 3 join-form slice is complete:
  - menu `Join Server` opens editable host/port/name form state
  - `JoinServer` (form) and `Connecting` (active network attempt) are now distinct runtime scenes
  - connect failures return to join UX for retry instead of forcing disconnected scene
- Phase 4 flecs-foundation slice is complete:
  - active client and server runtimes now boot through `flecs::world` composition roots
  - `GameClient` / `GameServer` are now thin shells over dedicated app/runtime services
  - shared deterministic sim remains non-ECS for this program
  - explicit world-level tests validate client/server phase ordering
- Phase 4 UI/document slice is complete:
  - menu/join screen state now lives in flecs-managed UI resources
  - `UiBuild` and `UiInteraction` phases now produce/consume a real `UiDocument`
  - keyboard/gamepad navigation now has mouse hover/click parity for those screens
  - menu/join rendering is delegated through a dedicated UI renderer path
- Phase 4 render decomposition slice is complete:
  - splash, centered status, gameplay world, and background rendering now live in dedicated render helpers
  - non-UI status screens use explicit presentation state built during `PresentationBuild`
  - `RenderSystem` now routes between specialized renderers instead of owning those draw implementations

## Target Runtime Shape

### High-Level States
- `Splash`
- `MainMenu`
- `JoinServer`
- `StartingServer` (local dedicated startup flow)
- `Connecting`
- `GameplayMultiplayer`
- `GameplaySingleplayer`
- `Options`
- `Disconnected`

### Runtime Principle
- Decouple runtime mode from transport state.
- Drive scene transitions from explicit user intent + runtime results, not only socket status.

## Phased Implementation

## Phase 1: Scene and Runtime State Foundation
Goal: introduce explicit runtime modes and scene kinds without changing gameplay behavior yet.

### Changes
- Expand scene enum and naming:
  - `src/client/core/scene.hpp`
  - `src/client/core/scene_manager.hpp`
- Add runtime mode/state container:
  - `src/client/core/runtime_state.hpp` (new)
  - `src/client/core/menu_model.hpp` (new)
- Refactor `Application::UpdateScene` to operate on runtime state, not raw booleans.

### Acceptance
- Builds cleanly with no behavior regressions.
- Existing multiplayer connect path still works when launched with explicit `--auto-join` (added in Phase 2).

## Phase 2: Main Menu UI and Navigation
Goal: present interactive menu options in client runtime.

### Current Status (2026-03-06)
- Phase 2A completed:
  - menu-first startup is now default
  - `--auto-join` / `--skip-splash` flags are implemented
  - interactive menu navigation + action dispatch is wired
  - `Join Server` and `Quit` are fully wired
  - `Start Server` / `Singleplayer` / `Options` route to placeholders
- Remaining for full Phase 2:
  - replace placeholder routes with real flows (Phase 2B+ / Phase 3+)

### Changes
- Add menu rendering + keyboard/gamepad navigation:
  - `src/client/systems/render_system.hpp`
  - `src/client/input/input_manager.hpp`
  - `src/client/scenes/menu_scene.hpp`
- Add actions:
  - `Start Server`
  - `Join Server`
  - `Singleplayer`
  - `Options`
  - `Quit`
- Add client args for startup behavior:
  - `--auto-join`, `--skip-splash`, optional dev helpers.
  - Files: `src/client/client_entry.cpp`, `src/client/game_client.hpp/.cpp`.

### Acceptance
- App boots to splash, then main menu.
- User can navigate/select items by keyboard and gamepad.
- Selecting `Quit` exits cleanly.

## Phase 3: Join Server Flow
Goal: user-driven connection flow from menu.

### Current Status (2026-03-06)
- Completed in the current implementation:
  - join form model and editable fields (`host`, `port`, `name`)
  - explicit `MainMenu -> JoinServer -> Connecting -> GameplayMultiplayer` transitions
  - join failure feedback in join UX with retry path
- Remaining follow-up:
  - retain disconnect reason after leaving the disconnected screen and returning to menu

### Changes
- Add join form model (host/port/name) with defaults from config:
  - `src/client/core/menu_model.hpp` (new/extended)
  - `src/client/game_client.cpp`
- Add explicit join UX validation/error display for failed host/port/name inputs.
- Harden disconnect return path back to menu with preserved reason text.
- Preserve existing handshake/prediction/interpolation pipeline after successful join.

### Acceptance
- Join form supports editing host/port/name before connection attempt.
- Joining transitions: `MainMenu -> JoinServer -> Connecting -> GameplayMultiplayer`.
- Disconnect returns to menu with reason preserved.

## Phase 4: Flecs Runtime Foundation
Goal: make flecs the active runtime architecture for both client and server before deeper feature work continues.

### Current Status (2026-03-06)
- Foundation slice completed:
  - `ClientApp` / `ServerApp` composition roots are active
  - client/server phase registration modules exist and are wired into the primary binaries
  - heavyweight runtime logic moved into dedicated runtime service classes
  - phase ordering tests were added for both worlds
- UI/document slice completed:
  - menu/join screen state moved into flecs-managed resources
  - `UiBuild`/`UiInteraction` phases now do real work
  - menu/join rendering now consumes a `UiDocument` through `UiRenderer`
- Render decomposition slice completed:
  - non-UI status presentation is built explicitly during `PresentationBuild`
  - gameplay/status/splash drawing moved out of `RenderSystem` into dedicated helpers
- Remaining Phase 4 work:
  - move more runtime/session state into explicit flecs resources/components
  - continue removing the transitional `RuntimeState + SceneManager` layer
  - implement the real `Start Server`, `Singleplayer`, and `Options` flows on top of the new UI/presentation structure

### Changes
- Introduce `ClientApp` / `ServerApp` composition roots backed by `flecs::world`.
- Add explicit client/server runtime phases and module registration.
- Move heavy runtime logic into dedicated service classes while keeping current gameplay/network/menu behavior intact.
- Update build wiring so flecs is an active dependency for the primary client/server targets.

### Acceptance
- Client and server both run through flecs worlds with explicit phase ordering.
- Current runtime behavior remains intact through the new shells.
- World-level tests validate client/server phase ordering.

## Phase 5: Start Server (Local Dedicated) Flow
Goal: menu option starts local dedicated server for quick local play.

### Changes
- Add local server launcher abstraction:
  - `src/client/core/server_launcher.hpp` (new interface)
  - `src/client/core/server_launcher_process.cpp/.hpp` (new process-based implementation)
- Initial implementation: spawn `game_server` process with configured port.
- Transition flow:
  - `MainMenu -> StartingServer -> Connecting -> GameplayMultiplayer`.
- Add robust failure handling and surfaced errors.

### Acceptance
- Selecting `Start Server` launches local dedicated server and auto-joins localhost.
- Failures return to menu with actionable reason string.

### Notes
- In-process embedded server is intentionally not first pass; process-based isolation is safer and keeps dedicated threading boundaries clear.

## Phase 6: Singleplayer Runtime (Authoritative Local Sim Path)
Goal: provide a no-network singleplayer mode while preserving authoritative logic principles.

### Changes
- Add singleplayer runtime path using shared simulation types:
  - `src/client/core/singleplayer_runtime.hpp/.cpp` (new)
  - `src/shared/game/*` reused directly
- Keep this mode transport-free by default.
- Reuse sandbox rendering/controls where practical.

### Acceptance
- `MainMenu -> GameplaySingleplayer` works without server process.
- Local save/load hook points defined (no fake persistence claims).

## Phase 7: Options Screen and Config Persistence
Goal: expose runtime/network/video/gameplay options.

### Changes
- Options menu scene/state:
  - `src/client/scenes/options_scene.hpp` (new)
  - `src/client/core/config.hpp`
- Persist user preferences to a client config file:
  - `assets/config/client.cfg` or `user_data/client.cfg` (final path chosen during implementation).
- Include:
  - display/window settings
  - default player name
  - last host/port
  - debug overlay default
  - interpolation delay

### Acceptance
- Updated settings apply at runtime where possible.
- Persisted settings load on next launch.

## Phase 8: Runtime Polish, Safety, and Testability
Goal: stabilize flows and prevent regressions.

### Changes
- Add tests:
  - scene transition tests
  - menu action mapping tests
  - join/start-server flow state tests
- Expand runbook for new user flows.
- Add structured logging for runtime transitions.

### Acceptance
- Deterministic, logged scene transitions.
- Manual smoke path passes:
  1. splash -> menu
  2. join remote server
  3. start local server and join
  4. singleplayer launch
  5. options save/reload

## File Impact Map (Planned)
- Client runtime core:
  - `src/client/game_client.hpp`
  - `src/client/game_client.cpp`
  - `src/client/core/application.hpp`
  - `src/client/core/scene.hpp`
  - `src/client/core/scene_manager.hpp`
  - `src/client/core/config.hpp`
- Client presentation/input:
  - `src/client/systems/render_system.hpp`
  - `src/client/input/input_manager.hpp`
  - `src/client/scenes/splash_scene.hpp`
  - `src/client/scenes/menu_scene.hpp`
  - `src/client/scenes/sandbox_scene.hpp`
  - `src/client/ui/debug_overlay.hpp`
- New runtime modules (planned):
  - `src/client/core/runtime_state.hpp`
  - `src/client/core/menu_model.hpp`
  - `src/client/core/server_launcher.hpp`
  - `src/client/core/server_launcher_process.hpp`
  - `src/client/core/server_launcher_process.cpp`
  - `src/client/core/singleplayer_runtime.hpp`
  - `src/client/core/singleplayer_runtime.cpp`
  - `src/client/scenes/options_scene.hpp`

## Risks and Mitigations
- Risk: menu/scene growth turns into ad-hoc conditionals.
  - Mitigation: explicit runtime state machine + action dispatcher.
- Risk: local server startup race before join.
  - Mitigation: startup timeout, connection retry window, explicit failure reason.
- Risk: singleplayer diverges from authoritative multiplayer sim.
  - Mitigation: keep gameplay rules in `src/shared/game` and avoid duplicate sim code.

## Definition of Done (This Plan)
- Client defaults to splash + interactive menu.
- Main menu exposes start server, join server, singleplayer, options, quit.
- Multiplayer path remains authoritative and functional.
- New flow is documented and test-smoked in runbook.
