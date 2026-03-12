#pragma once

#include <chrono>
#include <cstdint>
#include <deque>
#include <flecs.h>
#include <memory>
#include <optional>
#include <raylib-cpp.hpp>
#include <string>
#include <unordered_map>
#include <vector>

#include "client/components/components.hpp"
#include "client/core/client_config.hpp"
#include "client/core/server_launcher.hpp"
#include "client/input/input_manager.hpp"
#include "client/physics/movement_system.hpp"
#include "client/runtime/client_runtime_context.hpp"
#include "client/runtime/multiplayer_session_service.hpp"
#include "client/runtime/options_service.hpp"
#include "client/runtime/singleplayer_session_service.hpp"
#include "client/systems/render_system.hpp"
#include "client/ui/ui_document.hpp"
#include "client/ui/ui_state.hpp"
#include "shared/game/fixed_step.hpp"
namespace client {

namespace game = shared::game;
namespace net = shared::net;

namespace runtime {
    struct ClientRuntimeTestHooks;

    class ClientRuntime {
    public:
        explicit ClientRuntime(ClientConfig config);

        [[nodiscard]] bool Initialize(flecs::world world);
        void Shutdown();

        void CaptureInput(flecs::world world);
        void ProcessRuntimeIntent(flecs::world world);
        void BuildUiState(flecs::world world);
        void HandleUiInteraction(flecs::world world);
        void PollTransport();
        void RefreshSessionState(flecs::world world);
        void AdvancePrediction(float frameSeconds);
        void PublishPresentation(flecs::world world, float frameSeconds);
        void RenderPublishedFrame(const flecs::world& world);

        [[nodiscard]] bool ShouldExit() const;

    private:
        void ReturnToMenu(flecs::world world, std::string statusMessage = {});
        void InitializeWorldState(flecs::world world);
        [[nodiscard]] ClientRuntimeContext MakeContext(flecs::world world);

        void StepSimulation();

        ClientConfig config_;

        std::optional<raylib::Window> window_;
        std::optional<flecs::world> world_;

        bool exitRequested_ = false;
        std::unique_ptr<core::IServerLauncher> serverLauncher_;

        game::FixedStep fixedStep_;
        MultiplayerSessionService multiplayerSession_;
        OptionsService optionsService_;
        input::InputManager inputManager_{};
        SingleplayerSessionService singleplayerSession_;

        friend struct ClientRuntimeTestHooks;
    };

}  // namespace runtime

}  // namespace client
