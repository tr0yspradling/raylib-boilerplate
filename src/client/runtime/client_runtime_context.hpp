#pragma once

#include <memory>
#include <optional>

#include <flecs.h>
#include <raylib-cpp.hpp>

#include "client/core/client_config.hpp"
#include "client/core/server_launcher.hpp"
#include "client/input/input_manager.hpp"
#include "client/runtime/multiplayer_session_service.hpp"
#include "client/runtime/options_service.hpp"
#include "client/runtime/runtime_resources.hpp"
#include "client/runtime/singleplayer_session_service.hpp"
#include "shared/game/fixed_step.hpp"

namespace client::runtime {

struct ClientRuntimeContext {
    flecs::world world;
    ClientConfig& config;
    std::optional<raylib::Window>& window;
    std::unique_ptr<core::IServerLauncher>& serverLauncher;
    shared::game::FixedStep& fixedStep;
    input::InputManager& inputManager;
    MultiplayerSessionService& multiplayerSession;
    OptionsService& optionsService;
    SingleplayerSessionService& singleplayerSession;
    bool& exitRequested;

    [[nodiscard]] ClientFlowState& FlowState() const { return world.get_mut<ClientFlowState>(); }
    [[nodiscard]] LocalServerStartupState& LocalServerState() const { return world.get_mut<LocalServerStartupState>(); }
    [[nodiscard]] ClientSessionState& SessionState() const { return world.get_mut<ClientSessionState>(); }
    [[nodiscard]] core::SceneKind ActiveScene() const { return core::SceneForRuntime(world.get<ClientFlowState>().runtime); }
};

}  // namespace client::runtime
