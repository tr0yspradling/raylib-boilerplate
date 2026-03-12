#include "client/runtime/client_runtime.hpp"

#include <algorithm>
#include <utility>

#include "client/core/server_launcher_process.hpp"
#include "client/runtime/client_presentation_builder.hpp"
#include "client/runtime/client_runtime_flow.hpp"
#include "client/runtime/client_runtime_policy.hpp"
#include "client/runtime/client_ui_controller.hpp"
#include "client/runtime/client_ui_document_factory.hpp"
#include "shared/game/game_policy.hpp"

namespace client {

namespace runtime {

    ClientRuntime::ClientRuntime(ClientConfig config) :
        config_(std::move(config)), serverLauncher_(std::make_unique<core::ServerLauncherProcess>()),
        fixedStep_(1.0 / static_cast<double>(std::max(1, config_.simulationTickHz))),
        multiplayerSession_(config_, fixedStep_) {}

    bool ClientRuntime::Initialize(flecs::world world) {
        world_ = world;
        window_.emplace(config_.windowWidth, config_.windowHeight, std::string{policy::kWindowTitle});
        window_->SetTargetFPS(config_.targetFps);
        InitializeWorldState(world);
        return true;
    }

    void ClientRuntime::InitializeWorldState(flecs::world world) {
        ClientRuntimeContext context = MakeContext(world);
        ClientRuntimeFlowController::InitializeWorldState(context);
    }

    void ClientRuntime::Shutdown() {
        if (!world_.has_value()) {
            return;
        }

        multiplayerSession_.Shutdown(world_->get_mut<ClientSessionState>());
        if (world_->get<LocalServerStartupState>().ownsProcess && serverLauncher_) {
            serverLauncher_->Stop();
            world_->get_mut<LocalServerStartupState>().ownsProcess = false;
        }
    }

    void ClientRuntime::CaptureInput(flecs::world world) {
        inputManager_.Update();
        ClientFlowState& flow = world.get_mut<ClientFlowState>();
        if (inputManager_.DebugOverlayToggled()) {
            flow.debugOverlayEnabled = !flow.debugOverlayEnabled;
        }
        if (inputManager_.QuitRequested()) {
            exitRequested_ = true;
        }
        world.set<ui::UiInputState>(inputManager_.BuildUiInputState());
    }

    void ClientRuntime::ProcessRuntimeIntent(flecs::world world) {
        ClientRuntimeContext context = MakeContext(world);
        ClientRuntimeFlowController::ProcessRuntimeIntent(context);
    }

    void ClientRuntime::BuildUiState(flecs::world world) {
        ClientRuntimeContext context = MakeContext(world);
        ClientUiDocumentFactory::BuildPublishedDocument(context);
    }

    void ClientRuntime::HandleUiInteraction(flecs::world world) {
        ClientRuntimeContext context = MakeContext(world);
        ClientUiController::HandleUiInteraction(context);
    }

    void ClientRuntime::PollTransport() {
        if (!world_.has_value()) {
            return;
        }
        ClientFlowState& flow = world_->get_mut<ClientFlowState>();
        LocalServerStartupState& localServer = world_->get_mut<LocalServerStartupState>();
        ClientSessionState& session = world_->get_mut<ClientSessionState>();
        multiplayerSession_.Poll(flow, localServer, session);
    }

    void ClientRuntime::RefreshSessionState(flecs::world world) {
        ClientRuntimeContext context = MakeContext(world);
        ClientRuntimeFlowController::RefreshRuntimeState(context);
        ClientRuntimeFlowController::PublishScreenState(context);
    }

    void ClientRuntime::AdvancePrediction(float frameSeconds) {
        const int simSteps = fixedStep_.Accumulate(frameSeconds);
        for (int i = 0; i < simSteps; ++i) {
            StepSimulation();
        }
    }

    void ClientRuntime::PublishPresentation(flecs::world world, float frameSeconds) {
        ClientRuntimeContext context = MakeContext(world);
        ClientPresentationBuilder::Publish(context, frameSeconds);
    }

    void ClientRuntime::RenderPublishedFrame(const flecs::world& world) {
        if (!window_.has_value() || ShouldExit()) {
            return;
        }

        const components::WorldRenderState& worldState = world.get<components::WorldRenderState>();
        const ui::UiDocument& document = world.get<ui::UiDocument>();
        const components::StatusRenderState& statusState = world.get<components::StatusRenderState>();
        const components::NetworkDebugState& debugState = world.get<components::NetworkDebugState>();

        window_->BeginDrawing();
        systems::RenderSystem::DrawFrame(worldState, document, statusState, debugState,
                                         world.get<ClientFlowState>().debugOverlayEnabled);
        window_->EndDrawing();
    }

    bool ClientRuntime::ShouldExit() const {
        return exitRequested_ || !window_.has_value() || raylib::Window::ShouldClose();
    }

    void ClientRuntime::ReturnToMenu(flecs::world world, std::string statusMessage) {
        ClientRuntimeContext context = MakeContext(world);
        ClientRuntimeFlowController::ReturnToMenu(context, std::move(statusMessage));
    }

    void ClientRuntime::StepSimulation() {
        if (!world_.has_value()) {
            return;
        }

        ClientFlowState& flow = world_->get_mut<ClientFlowState>();
        ClientSessionState& session = world_->get_mut<ClientSessionState>();
        if (flow.runtime.mode == core::RuntimeMode::Singleplayer) {
            if (!singleplayerSession_.IsActive()) {
                return;
            }

            game::PlayerInputFrame inputFrame =
                inputManager_.BuildPlayerInputFrame(session.clientTick, session.nextInputSequence++);
            ++session.clientTick;

            singleplayerSession_.Step(inputFrame, static_cast<float>(fixedStep_.StepSeconds()), session);
            return;
        }

        if (flow.runtime.mode != core::RuntimeMode::Multiplayer || !session.connected || !session.serverWelcomed ||
            !session.localPlayerId.IsValid()) {
            return;
        }

        game::PlayerInputFrame inputFrame =
            inputManager_.BuildPlayerInputFrame(session.clientTick, session.nextInputSequence++);
        ++session.clientTick;

        multiplayerSession_.SendInputFrame(session, inputFrame);
        session.pendingInputs.push_back(inputFrame);

        physics::MovementSystem::Predict(session.predictedLocalPlayer, inputFrame,
                                         static_cast<float>(fixedStep_.StepSeconds()), session.serverKinematics);
    }

    ClientRuntimeContext ClientRuntime::MakeContext(flecs::world world) {
        return {
            .world = world,
            .config = config_,
            .window = window_,
            .serverLauncher = serverLauncher_,
            .fixedStep = fixedStep_,
            .inputManager = inputManager_,
            .multiplayerSession = multiplayerSession_,
            .optionsService = optionsService_,
            .singleplayerSession = singleplayerSession_,
            .exitRequested = exitRequested_,
        };
    }

}  // namespace runtime

}  // namespace client
