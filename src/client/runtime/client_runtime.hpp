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
#include "client/core/menu_model.hpp"
#include "client/core/runtime_state.hpp"
#include "client/core/server_launcher.hpp"
#include "client/core/singleplayer_runtime.hpp"
#include "client/input/input_manager.hpp"
#include "client/physics/movement_system.hpp"
#include "client/runtime/multiplayer_session_service.hpp"
#include "client/runtime/runtime_resources.hpp"
#include "client/systems/render_system.hpp"
#include "client/ui/ui_document.hpp"
#include "client/ui/ui_state.hpp"
#include "shared/game/chunk.hpp"
#include "shared/game/entity.hpp"
#include "shared/game/fixed_step.hpp"
#include "shared/game/interpolation.hpp"
namespace client {

namespace game = shared::game;
namespace net = shared::net;

namespace runtime {
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
        void ConsumeUiCommands(flecs::world world);
        void HandlePlaceholderScreenInput(flecs::world world);
        void UpdateLocalServerStartup(flecs::world world, std::chrono::steady_clock::time_point now);
        void HandleUiPointerFocus(flecs::world world, const ui::UiDocument& document,
                                  const ui::UiInputState& inputState);
        void HandleMenuInteraction(flecs::world world, const ui::UiDocument& document,
                                   const ui::UiInputState& inputState);
        void HandleJoinFormInteraction(flecs::world world, const ui::UiDocument& document,
                                       const ui::UiInputState& inputState);
        void HandleOptionsInteraction(flecs::world world, const ui::UiDocument& document,
                                      const ui::UiInputState& inputState);
        void ApplyJoinFormTextInput(ui::JoinServerScreenState& joinScreenState,
                                    const ui::UiInputState& inputState) const;
        void ApplyOptionsTextInput(ui::OptionsScreenState& optionsScreenState,
                                   const ui::UiInputState& inputState) const;
        void ActivateMenuAction(flecs::world world, core::MenuAction action);
        void BeginSingleplayer(flecs::world world);
        bool BeginJoinServer(flecs::world world);
        [[nodiscard]] bool EnsureTransportInitialized(std::string& error);
        [[nodiscard]] bool BeginConnectionAttempt(std::string& error);
        bool ApplyJoinFormToConfig(const ui::JoinServerScreenState& joinScreenState);
        bool ApplyOptionsToConfig(flecs::world world, ui::OptionsScreenState& optionsScreenState);
        void ReturnToMenu(flecs::world world, std::string statusMessage = {});
        void FailLocalServerStartup(flecs::world world, const std::string& message);
        void StopOwnedLocalServer();
        void PublishScreenState(flecs::world world);
        [[nodiscard]] ui::UiDocument BuildMenuDocument(const ui::ScreenState& screenState,
                                                       const ui::MenuScreenState& menuScreenState,
                                                       const ui::UiInteractionState& interactionState) const;
        [[nodiscard]] ui::UiDocument BuildJoinDocument(const ui::ScreenState& screenState,
                                                       const ui::JoinServerScreenState& joinScreenState,
                                                       const ui::UiInteractionState& interactionState) const;
        [[nodiscard]] ui::UiDocument BuildOptionsDocument(const ui::ScreenState& screenState,
                                                          const ui::OptionsScreenState& optionsScreenState,
                                                          const ui::UiInteractionState& interactionState) const;

        void ResetSessionState();
        void RefreshRuntimeState();
        [[nodiscard]] ClientFlowState& FlowState();
        [[nodiscard]] const ClientFlowState& FlowState() const;
        [[nodiscard]] LocalServerStartupState& LocalServerState();
        [[nodiscard]] const LocalServerStartupState& LocalServerState() const;
        [[nodiscard]] ClientSessionState& SessionState();
        [[nodiscard]] const ClientSessionState& SessionState() const;
        [[nodiscard]] flecs::world& RuntimeWorld();
        [[nodiscard]] const flecs::world& RuntimeWorld() const;
        [[nodiscard]] core::SceneKind ActiveScene() const;

        void StepSimulation();

        [[nodiscard]] components::WorldRenderState BuildWorldRenderState() const;
        [[nodiscard]] components::StatusRenderState BuildStatusRenderState() const;
        [[nodiscard]] components::NetworkDebugState BuildDebugState() const;

        [[nodiscard]] bool IsLocalPlayerReady() const;

        ClientConfig config_;

        std::optional<raylib::Window> window_;
        std::optional<flecs::world> world_;

        bool exitRequested_ = false;
        std::unique_ptr<core::IServerLauncher> serverLauncher_;

        game::FixedStep fixedStep_;
        MultiplayerSessionService multiplayerSession_;
        input::InputManager inputManager_{};
        core::SingleplayerRuntime singleplayerRuntime_{};
};

}  // namespace runtime

}  // namespace client
