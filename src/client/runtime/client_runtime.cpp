#include "client/runtime/client_runtime.hpp"

#include <algorithm>
#include <charconv>
#include <chrono>
#include <cstdio>
#include <filesystem>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <unordered_set>
#include <utility>

#include "client/core/config.hpp"
#include "client/core/server_launcher_process.hpp"
#include "client/render/status_presenter.hpp"
#include "client/scenes/menu_scene.hpp"
#include "client/scenes/sandbox_scene.hpp"
#include "client/scenes/splash_scene.hpp"
#include "shared/game/chunk_streaming.hpp"
#include "shared/game/validation.hpp"
#include "shared/game/world.hpp"
#include "shared/net/send_policy.hpp"

namespace client {

namespace {

    constexpr std::chrono::milliseconds kSplashDuration{1400};
    constexpr float kMenuWidth = 420.0f;
    constexpr float kJoinWidth = 520.0f;
    constexpr float kRowHeight = 44.0f;
    constexpr float kRowSpacing = 56.0f;
    constexpr float kMenuStartY = 254.0f;
    constexpr float kJoinStartY = 244.0f;
    constexpr float kOptionsWidth = 620.0f;
    constexpr float kOptionsStartY = 162.0f;
    constexpr float kOptionsRowSpacing = 42.0f;
    constexpr std::chrono::milliseconds kLocalServerConnectRetryInterval{250};
    constexpr std::chrono::seconds kLocalServerStartupTimeout{10};

    [[nodiscard]] std::string ComposeSceneLabel(core::SceneKind sceneKind, std::string_view statusMessage) {
        switch (sceneKind) {
            case core::SceneKind::Splash:
                return std::string{core::SceneName(sceneKind)} + " - " + std::string{scenes::SplashCaption()};
            case core::SceneKind::MainMenu:
                if (!statusMessage.empty()) {
                    return std::string{core::SceneName(sceneKind)} + " - " + std::string{statusMessage};
                }
                return std::string{core::SceneName(sceneKind)} + " - Select mode";
            case core::SceneKind::JoinServer:
                if (!statusMessage.empty()) {
                    return std::string{core::SceneName(sceneKind)} + " - " + std::string{statusMessage};
                }
                return std::string{core::SceneName(sceneKind)} + " - Configure destination";
            case core::SceneKind::StartingServer:
                if (!statusMessage.empty()) {
                    return std::string{core::SceneName(sceneKind)} + " - " + std::string{statusMessage};
                }
                return std::string{core::SceneName(sceneKind)} + " - Booting local dedicated";
            case core::SceneKind::Connecting:
                if (!statusMessage.empty()) {
                    return std::string{core::SceneName(sceneKind)} + " - " + std::string{statusMessage};
                }
                return std::string{core::SceneName(sceneKind)} + " - " + std::string{scenes::ConnectingCaption()};
            case core::SceneKind::GameplayMultiplayer:
                return std::string{core::SceneName(sceneKind)} + " - " + std::string{scenes::GameplayCaption()};
            case core::SceneKind::GameplaySingleplayer:
                if (!statusMessage.empty()) {
                    return std::string{core::SceneName(sceneKind)} + " - " + std::string{statusMessage};
                }
                return std::string{core::SceneName(sceneKind)} + " - Local sandbox";
            case core::SceneKind::Options:
                if (!statusMessage.empty()) {
                    return std::string{core::SceneName(sceneKind)} + " - " + std::string{statusMessage};
                }
                return std::string{core::SceneName(sceneKind)} + " - Settings";
            case core::SceneKind::Disconnected:
                if (!statusMessage.empty()) {
                    return std::string{core::SceneName(sceneKind)} + " - " + std::string{statusMessage};
                }
                return std::string{core::SceneName(sceneKind)};
        }

        return "Unknown";
    }

}  // namespace

namespace runtime {

    ClientRuntime::ClientRuntime(ClientConfig config) :
        config_(std::move(config)), serverLauncher_(std::make_unique<core::ServerLauncherProcess>()),
        fixedStep_(1.0 / static_cast<double>(std::max(1, config_.simulationTickHz))) {
        serverTickRateHz_ = static_cast<uint16_t>(std::clamp(config_.simulationTickHz, 1, 65535));
    }

    flecs::world& ClientRuntime::RuntimeWorld() { return *world_; }

    const flecs::world& ClientRuntime::RuntimeWorld() const { return *world_; }

    ClientFlowState& ClientRuntime::FlowState() { return RuntimeWorld().get_mut<ClientFlowState>(); }

    const ClientFlowState& ClientRuntime::FlowState() const { return RuntimeWorld().get<ClientFlowState>(); }

    LocalServerStartupState& ClientRuntime::LocalServerState() {
        return RuntimeWorld().get_mut<LocalServerStartupState>();
    }

    const LocalServerStartupState& ClientRuntime::LocalServerState() const {
        return RuntimeWorld().get<LocalServerStartupState>();
    }

    bool ClientRuntime::Initialize(flecs::world world) {
        world_ = world;
        window_.emplace(config_.windowWidth, config_.windowHeight, "raylib boilerplate - multiplayer client");
        window_->SetTargetFPS(config_.targetFps);

        ClientFlowState& flow = world.get_mut<ClientFlowState>();
        flow.runtime.mode = core::RuntimeMode::Boot;
        flow.runtime.splashCompleted = config_.skipSplash;
        flow.runtime.requestedJoin = config_.autoJoin;
        flow.runtime.requestedLocalServerStart = false;
        flow.runtime.joiningInProgress = false;
        flow.runtime.disconnectReason.clear();
        flow.statusMessage.clear();
        flow.disconnectReason.clear();
        flow.debugOverlayEnabled = config_.debugOverlayDefault;
        flow.splashStartedAt = std::chrono::steady_clock::now();

        world.set<LocalServerStartupState>({});

        sceneManager_.SwitchTo(flow.runtime.splashCompleted ? core::SceneKind::MainMenu : core::SceneKind::Splash);
        ui::JoinServerScreenState& joinScreenState = world.get_mut<ui::JoinServerScreenState>();
        joinScreenState.ResetFromDefaults(config_.serverHost, config_.serverPort, config_.playerName);
        world.set<ui::MenuScreenState>({});
        world.set<ui::JoinServerScreenState>(joinScreenState);
        ui::OptionsScreenState optionsScreenState;
        optionsScreenState.ResetFromConfig(config_);
        world.set<ui::OptionsScreenState>(optionsScreenState);
        world.set<ui::UiInteractionState>({});
        world.set<ui::UiCommandQueue>({});
        world.set<ui::UiDocument>({});
        world.set<ui::UiInputState>({});
        PublishScreenState(world);
        return true;
    }

    void ClientRuntime::Shutdown() {
        if (transport_.IsInitialized() && connected_) {
            transport_.Close(serverConnection_, 0, "client shutdown", false);
        }
        if (transport_.IsInitialized()) {
            transport_.Shutdown();
        }
        StopOwnedLocalServer();
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
        const auto now = std::chrono::steady_clock::now();
        ClientFlowState& flow = world.get_mut<ClientFlowState>();
        if (!flow.runtime.splashCompleted) {
            if (inputManager_.MenuSelectPressed() || inputManager_.MenuBackPressed() ||
                inputManager_.MousePrimaryPressed() ||
                std::chrono::duration_cast<std::chrono::milliseconds>(now - flow.splashStartedAt) >= kSplashDuration) {
                flow.runtime.splashCompleted = true;
            }
        }

        RefreshRuntimeState();
        ConsumeUiCommands(world);
        HandlePlaceholderScreenInput(world);
        UpdateLocalServerStartup(world, now);
        if (flow.runtime.mode == core::RuntimeMode::Menu && flow.runtime.requestedJoin) {
            flow.runtime.requestedJoin = false;
            BeginJoinServer(world);
        }
        PublishScreenState(world);
    }

    void ClientRuntime::BuildUiState(flecs::world world) {
        PublishScreenState(world);

        const ui::ScreenState& screenState = world.get<ui::ScreenState>();
        const ui::MenuScreenState& menuScreenState = world.get<ui::MenuScreenState>();
        const ui::JoinServerScreenState& joinScreenState = world.get<ui::JoinServerScreenState>();
        const ui::OptionsScreenState& optionsScreenState = world.get<ui::OptionsScreenState>();
        ui::UiInteractionState& interactionState = world.get_mut<ui::UiInteractionState>();

        ui::UiDocument document;
        if (screenState.activeScene == core::SceneKind::MainMenu) {
            if (!interactionState.focusedWidget.has_value()) {
                interactionState.focusedWidget = ui::UiWidgetIdForMenuAction(menuScreenState.SelectedAction());
            }
            document = BuildMenuDocument(screenState, menuScreenState, interactionState);
        } else if (screenState.activeScene == core::SceneKind::JoinServer && !screenState.joiningInProgress) {
            if (!interactionState.focusedWidget.has_value()) {
                interactionState.focusedWidget = ui::UiWidgetIdForJoinField(joinScreenState.SelectedField());
            }
            document = BuildJoinDocument(screenState, joinScreenState, interactionState);
        } else if (screenState.activeScene == core::SceneKind::Options) {
            if (!interactionState.focusedWidget.has_value()) {
                interactionState.focusedWidget = ui::UiWidgetIdForOptionsField(optionsScreenState.SelectedField());
            }
            document = BuildOptionsDocument(screenState, optionsScreenState, interactionState);
        } else {
            interactionState.hoveredWidget.reset();
            interactionState.pressedWidget.reset();
            document.scene = screenState.activeScene;
        }

        world.set<ui::UiDocument>(std::move(document));
    }

    void ClientRuntime::HandleUiInteraction(flecs::world world) {
        const ui::ScreenState& screenState = world.get<ui::ScreenState>();
        const ui::UiInputState& inputState = world.get<ui::UiInputState>();
        const ui::UiDocument& document = world.get<ui::UiDocument>();

        if (document.widgets.empty()) {
            ui::UiInteractionState& interactionState = world.get_mut<ui::UiInteractionState>();
            interactionState.hoveredWidget.reset();
            interactionState.pressedWidget.reset();
            return;
        }

        HandleUiPointerFocus(world, document, inputState);

        if (screenState.activeScene == core::SceneKind::MainMenu) {
            HandleMenuInteraction(world, document, inputState);
            return;
        }

        if (screenState.activeScene == core::SceneKind::JoinServer && !screenState.joiningInProgress) {
            HandleJoinFormInteraction(world, document, inputState);
            return;
        }

        if (screenState.activeScene == core::SceneKind::Options) {
            HandleOptionsInteraction(world, document, inputState);
        }
    }

    void ClientRuntime::PollTransport() {
        if (!transport_.IsInitialized()) {
            return;
        }

        transport_.Poll();
        HandleConnectionEvents();
        HandleIncomingPackets();
    }

    void ClientRuntime::RefreshSessionState(flecs::world world) {
        RefreshRuntimeState();
        PublishScreenState(world);
    }

    void ClientRuntime::AdvancePrediction(float frameSeconds) {
        const int simSteps = fixedStep_.Accumulate(frameSeconds);
        for (int i = 0; i < simSteps; ++i) {
            StepSimulation();
        }
    }

    void ClientRuntime::PublishPresentation(flecs::world world, float frameSeconds) {
        const ClientFlowState& flow = world.get<ClientFlowState>();
        if (latestServerTick_ > static_cast<game::TickId>(config_.interpolationDelayTicks)) {
            const float maxTick =
                static_cast<float>(latestServerTick_ - static_cast<game::TickId>(config_.interpolationDelayTicks));
            renderInterpolationTick_ =
                std::min(renderInterpolationTick_ + frameSeconds * static_cast<float>(serverTickRateHz_), maxTick);
        }

        RefreshRuntimeState();
        core::Application::UpdateScene(sceneManager_, flow.runtime);

        const auto now = std::chrono::steady_clock::now();
        const auto sinceLastPing = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastPingSentAt_);
        if (flow.runtime.mode == core::RuntimeMode::Multiplayer && connected_ && sinceLastPing.count() >= 1000) {
            net::PingMessage ping{.sequence = nextPingSequence_++};
            const std::vector<uint8_t> payload = net::Serialize(ping);
            const std::vector<uint8_t> packet = net::BuildPacket(net::MessageId::Ping, payload);
            std::string error;
            transport_.Send(serverConnection_, packet,
                            net::SendOptionsForMessage(net::MessageId::Ping, net::MessageDirection::ClientToServer),
                            error);
            lastPingSentAt_ = now;
        }

        const auto sinceLastChunkHint =
            std::chrono::duration_cast<std::chrono::milliseconds>(now - lastChunkHintSentAt_);
        if (flow.runtime.mode == core::RuntimeMode::Multiplayer && connected_ && serverWelcomed_ &&
            IsLocalPlayerReady() && sinceLastChunkHint.count() >= 500) {
            SendChunkInterestHint();
            lastChunkHintSentAt_ = now;
        }

        PublishScreenState(world);
        world.set<components::WorldRenderState>(BuildWorldRenderState());
        world.set<components::StatusRenderState>(BuildStatusRenderState());
        world.set<components::NetworkDebugState>(BuildDebugState());
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

    void ClientRuntime::HandlePlaceholderScreenInput(flecs::world world) {
        ClientFlowState& flow = world.get_mut<ClientFlowState>();
        LocalServerStartupState& localServer = world.get_mut<LocalServerStartupState>();
        switch (flow.runtime.mode) {
            case core::RuntimeMode::Boot:
            case core::RuntimeMode::Multiplayer:
            case core::RuntimeMode::Menu:
                return;
            case core::RuntimeMode::JoiningServer:
                if (flow.runtime.joiningInProgress && inputManager_.MenuBackPressed()) {
                    if (transport_.IsInitialized() && serverConnection_ != net::kInvalidConnectionHandle) {
                        transport_.Close(serverConnection_, 0, "join canceled", false);
                    }
                    ResetSessionState();
                    if (localServer.startupInProgress) {
                        localServer.startupInProgress = false;
                        flow.runtime.requestedLocalServerStart = false;
                        StopOwnedLocalServer();
                        ReturnToMenu(world, "Local dedicated startup canceled");
                    } else {
                        ReturnToMenu(world);
                    }
                }
                return;
            case core::RuntimeMode::StartingLocalServer:
                if (inputManager_.MenuBackPressed()) {
                    localServer.startupInProgress = false;
                    flow.runtime.requestedLocalServerStart = false;
                    StopOwnedLocalServer();
                    ReturnToMenu(world, "Local dedicated startup canceled");
                }
                return;
            case core::RuntimeMode::Singleplayer:
                if (inputManager_.MenuBackPressed()) {
                    ReturnToMenu(world);
                }
                return;
            case core::RuntimeMode::Options:
                return;
            case core::RuntimeMode::Disconnected:
                if (inputManager_.MenuSelectPressed() || inputManager_.MenuBackPressed()) {
                    ReturnToMenu(world);
                }
                return;
        }
    }

    void ClientRuntime::UpdateLocalServerStartup(flecs::world world, std::chrono::steady_clock::time_point now) {
        ClientFlowState& flow = world.get_mut<ClientFlowState>();
        LocalServerStartupState& localServer = world.get_mut<LocalServerStartupState>();
        if (flow.runtime.mode != core::RuntimeMode::StartingLocalServer && !localServer.startupInProgress) {
            return;
        }

        if (localServer.ownsProcess && serverLauncher_) {
            const core::ServerProcessStatus status = serverLauncher_->PollStatus();
            if (status.state == core::ServerProcessState::Exited) {
                std::string message = "Local dedicated server exited before accepting connections";
                if (status.exitCode.has_value()) {
                    message += " (exit " + std::to_string(*status.exitCode) + ")";
                }
                FailLocalServerStartup(world, message);
                return;
            }
        }

        if (flow.runtime.requestedLocalServerStart) {
            if (!localServer.ownsProcess) {
                std::string launchError;
                if (!serverLauncher_ ||
                    !serverLauncher_->Launch({.clientExecutablePath = config_.executablePath,
                                              .serverPort = config_.serverPort,
                                              .simulationTickHz = static_cast<int>(serverTickRateHz_),
                                              .snapshotRateHz = static_cast<int>(serverSnapshotRateHz_)},
                                             launchError)) {
                    FailLocalServerStartup(world, "Failed to launch local dedicated server: " + launchError);
                    return;
                }
                localServer.ownsProcess = true;
            }

            flow.runtime.requestedLocalServerStart = false;
            localServer.startupInProgress = true;
            localServer.launchStartedAt = now;
            localServer.lastConnectAttemptAt = now - kLocalServerConnectRetryInterval;
            flow.statusMessage = "Launching local dedicated server...";
        }

        if (!localServer.startupInProgress) {
            return;
        }

        if (std::chrono::duration_cast<std::chrono::milliseconds>(now - localServer.launchStartedAt) >=
            kLocalServerStartupTimeout) {
            FailLocalServerStartup(world, "Timed out waiting for local dedicated server startup");
            return;
        }

        if (connecting_ || connected_) {
            return;
        }

        if (std::chrono::duration_cast<std::chrono::milliseconds>(now - localServer.lastConnectAttemptAt) <
            kLocalServerConnectRetryInterval) {
            return;
        }

        localServer.lastConnectAttemptAt = now;

        std::string error;
        if (!EnsureTransportInitialized(error)) {
            FailLocalServerStartup(world, "Transport init failed: " + error);
            return;
        }

        if (!BeginConnectionAttempt(error)) {
            flow.runtime.mode = core::RuntimeMode::StartingLocalServer;
            flow.runtime.joiningInProgress = false;
            flow.statusMessage = "Waiting for local dedicated server...";
            return;
        }

        flow.runtime.mode = core::RuntimeMode::JoiningServer;
        flow.runtime.joiningInProgress = true;
        connecting_ = true;
        flow.statusMessage = "Connecting to local dedicated server...";
    }

    void ClientRuntime::ConsumeUiCommands(flecs::world world) {
        ui::UiCommandQueue& commandQueue = world.get_mut<ui::UiCommandQueue>();
        ui::JoinServerScreenState& joinScreenState = world.get_mut<ui::JoinServerScreenState>();
        ui::OptionsScreenState& optionsScreenState = world.get_mut<ui::OptionsScreenState>();
        ui::UiInteractionState& interactionState = world.get_mut<ui::UiInteractionState>();
        ClientFlowState& flow = world.get_mut<ClientFlowState>();

        for (const ui::UiCommand& command : commandQueue.commands) {
            switch (command.type) {
                case ui::UiCommandType::ActivateMenuAction:
                    ActivateMenuAction(world, command.menuAction);
                    break;
                case ui::UiCommandType::StartJoinFieldEdit:
                    joinScreenState.editing = true;
                    flow.statusMessage = "Editing field. Type, Backspace to erase, Enter/Esc to finish";
                    interactionState.focusedWidget = ui::UiWidgetIdForJoinField(command.joinField);
                    break;
                case ui::UiCommandType::StopJoinFieldEdit:
                    joinScreenState.editing = false;
                    flow.statusMessage.clear();
                    break;
                case ui::UiCommandType::SubmitJoin:
                    if (!ApplyJoinFormToConfig(joinScreenState)) {
                        break;
                    }
                    flow.statusMessage.clear();
                    BeginJoinServer(world);
                    break;
                case ui::UiCommandType::StartOptionsFieldEdit:
                    optionsScreenState.editing = true;
                    flow.statusMessage = "Editing field. Type, Backspace to erase, Enter/Esc to finish";
                    interactionState.focusedWidget = ui::UiWidgetIdForOptionsField(command.optionsField);
                    break;
                case ui::UiCommandType::StopOptionsFieldEdit:
                    optionsScreenState.editing = false;
                    flow.statusMessage.clear();
                    break;
                case ui::UiCommandType::ToggleOptionsDebugOverlay:
                    optionsScreenState.debugOverlayDefault = !optionsScreenState.debugOverlayDefault;
                    break;
                case ui::UiCommandType::SaveOptions:
                    if (ApplyOptionsToConfig(world, optionsScreenState)) {
                        const size_t selectedIndex = optionsScreenState.SelectedIndex();
                        optionsScreenState.ResetFromConfig(config_);
                        optionsScreenState.SetSelectedIndex(selectedIndex);
                        interactionState.focusedWidget =
                            ui::UiWidgetIdForOptionsField(optionsScreenState.SelectedField());
                    }
                    break;
                case ui::UiCommandType::BackToMenu:
                    ReturnToMenu(world);
                    break;
                case ui::UiCommandType::None:
                    break;
            }
        }

        commandQueue.Clear();
    }

    void ClientRuntime::HandleUiPointerFocus(flecs::world world, const ui::UiDocument& document,
                                             const ui::UiInputState& inputState) {
        ui::UiInteractionState& interactionState = world.get_mut<ui::UiInteractionState>();
        const std::optional<ui::UiWidgetId> hoveredWidget = document.FindWidgetAt(inputState.mouseX, inputState.mouseY);
        interactionState.hoveredWidget = hoveredWidget;

        if (!hoveredWidget.has_value() || !inputState.mouseMoved) {
            return;
        }

        const ui::ScreenState& screenState = world.get<ui::ScreenState>();
        if (screenState.activeScene == core::SceneKind::JoinServer && world.get<ui::JoinServerScreenState>().editing) {
            return;
        }
        if (screenState.activeScene == core::SceneKind::Options && world.get<ui::OptionsScreenState>().editing) {
            return;
        }

        interactionState.focusedWidget = hoveredWidget;
        if (screenState.activeScene == core::SceneKind::MainMenu) {
            const core::MenuAction action = ui::MenuActionForWidgetId(*hoveredWidget);
            if (action == core::MenuAction::None) {
                return;
            }
            ui::MenuScreenState& menuScreenState = world.get_mut<ui::MenuScreenState>();
            const auto& actions = menuScreenState.Actions();
            for (size_t index = 0; index < actions.size(); ++index) {
                if (actions[index] == action) {
                    menuScreenState.SetSelectedIndex(index);
                    break;
                }
            }
            return;
        }

        if (screenState.activeScene == core::SceneKind::JoinServer) {
            const core::JoinFormField field = ui::JoinFieldForWidgetId(*hoveredWidget);
            ui::JoinServerScreenState& joinScreenState = world.get_mut<ui::JoinServerScreenState>();
            const auto& fields = ui::JoinServerScreenState::kFields;
            for (size_t index = 0; index < fields.size(); ++index) {
                if (fields[index] == field) {
                    joinScreenState.SetSelectedIndex(index);
                    break;
                }
            }
            return;
        }

        if (screenState.activeScene == core::SceneKind::Options) {
            const ui::OptionsField field = ui::OptionsFieldForWidgetId(*hoveredWidget);
            ui::OptionsScreenState& optionsScreenState = world.get_mut<ui::OptionsScreenState>();
            const auto& fields = ui::OptionsScreenState::kFields;
            for (size_t index = 0; index < fields.size(); ++index) {
                if (fields[index] == field) {
                    optionsScreenState.SetSelectedIndex(index);
                    break;
                }
            }
        }
    }

    void ClientRuntime::HandleMenuInteraction(flecs::world world, const ui::UiDocument& document,
                                              const ui::UiInputState& inputState) {
        ui::MenuScreenState& menuScreenState = world.get_mut<ui::MenuScreenState>();
        ui::UiInteractionState& interactionState = world.get_mut<ui::UiInteractionState>();
        ui::UiCommandQueue& commandQueue = world.get_mut<ui::UiCommandQueue>();
        interactionState.pressedWidget.reset();

        if (inputState.navigateDownPressed) {
            menuScreenState.MoveNext();
            interactionState.focusedWidget = ui::UiWidgetIdForMenuAction(menuScreenState.SelectedAction());
        }
        if (inputState.navigateUpPressed) {
            menuScreenState.MovePrevious();
            interactionState.focusedWidget = ui::UiWidgetIdForMenuAction(menuScreenState.SelectedAction());
        }

        std::optional<ui::UiWidgetId> activateWidget;
        if (inputState.primaryPressed && interactionState.hoveredWidget.has_value()) {
            activateWidget = interactionState.hoveredWidget;
        } else if (inputState.acceptPressed) {
            activateWidget = interactionState.focusedWidget;
        }

        if (!activateWidget.has_value()) {
            return;
        }

        const core::MenuAction action = ui::MenuActionForWidgetId(*activateWidget);
        if (action == core::MenuAction::None) {
            return;
        }

        interactionState.pressedWidget = activateWidget;
        commandQueue.Push({
            .type = ui::UiCommandType::ActivateMenuAction,
            .menuAction = action,
        });
        (void)document;
    }

    void ClientRuntime::ApplyJoinFormTextInput(ui::JoinServerScreenState& joinScreenState,
                                               const ui::UiInputState& inputState) const {
        if (!joinScreenState.editing || !joinScreenState.SelectedFieldIsEditable()) {
            return;
        }

        std::string* target = nullptr;
        const core::JoinFormField field = joinScreenState.SelectedField();
        switch (field) {
            case core::JoinFormField::Host:
                target = &joinScreenState.host;
                break;
            case core::JoinFormField::Port:
                target = &joinScreenState.port;
                break;
            case core::JoinFormField::Name:
                target = &joinScreenState.playerName;
                break;
            case core::JoinFormField::Connect:
            case core::JoinFormField::Back:
                return;
        }

        for (const char character : inputState.textInput) {
            if (field == core::JoinFormField::Port) {
                if (character >= '0' && character <= '9' && target->size() < 5U) {
                    target->push_back(character);
                }
                continue;
            }

            const size_t maxSize = field == core::JoinFormField::Host ? 64U : 24U;
            if (target->size() < maxSize) {
                target->push_back(character);
            }
        }

        if (inputState.backspacePressed && !target->empty()) {
            target->pop_back();
        }
    }

    void ClientRuntime::ApplyOptionsTextInput(ui::OptionsScreenState& optionsScreenState,
                                              const ui::UiInputState& inputState) const {
        if (!optionsScreenState.editing || !optionsScreenState.SelectedFieldIsEditable()) {
            return;
        }

        std::string* target = nullptr;
        const ui::OptionsField field = optionsScreenState.SelectedField();
        switch (field) {
            case ui::OptionsField::PlayerName:
                target = &optionsScreenState.playerName;
                break;
            case ui::OptionsField::Host:
                target = &optionsScreenState.host;
                break;
            case ui::OptionsField::Port:
                target = &optionsScreenState.port;
                break;
            case ui::OptionsField::WindowWidth:
                target = &optionsScreenState.windowWidth;
                break;
            case ui::OptionsField::WindowHeight:
                target = &optionsScreenState.windowHeight;
                break;
            case ui::OptionsField::TargetFps:
                target = &optionsScreenState.targetFps;
                break;
            case ui::OptionsField::InterpolationDelay:
                target = &optionsScreenState.interpolationDelay;
                break;
            case ui::OptionsField::DebugOverlay:
            case ui::OptionsField::Save:
            case ui::OptionsField::Back:
                return;
        }

        for (const char character : inputState.textInput) {
            if (ui::OptionsScreenState::IsNumericField(field)) {
                if (character >= '0' && character <= '9' && target->size() < 5U) {
                    target->push_back(character);
                }
                continue;
            }

            const size_t maxSize = field == ui::OptionsField::Host ? 64U : 24U;
            if (target->size() < maxSize) {
                target->push_back(character);
            }
        }

        if (inputState.backspacePressed && !target->empty()) {
            target->pop_back();
        }
    }

    void ClientRuntime::HandleJoinFormInteraction(flecs::world world, const ui::UiDocument& document,
                                                  const ui::UiInputState& inputState) {
        ui::JoinServerScreenState& joinScreenState = world.get_mut<ui::JoinServerScreenState>();
        ui::UiInteractionState& interactionState = world.get_mut<ui::UiInteractionState>();
        ui::UiCommandQueue& commandQueue = world.get_mut<ui::UiCommandQueue>();
        interactionState.pressedWidget.reset();

        ApplyJoinFormTextInput(joinScreenState, inputState);

        if (!joinScreenState.editing) {
            if (inputState.navigateDownPressed) {
                joinScreenState.MoveNext();
                interactionState.focusedWidget = ui::UiWidgetIdForJoinField(joinScreenState.SelectedField());
            }
            if (inputState.navigateUpPressed) {
                joinScreenState.MovePrevious();
                interactionState.focusedWidget = ui::UiWidgetIdForJoinField(joinScreenState.SelectedField());
            }
        }

        if (joinScreenState.editing && (inputState.acceptPressed || inputState.backPressed)) {
            commandQueue.Push({
                .type = ui::UiCommandType::StopJoinFieldEdit,
                .joinField = joinScreenState.SelectedField(),
            });
            return;
        }

        if (!joinScreenState.editing && inputState.backPressed) {
            commandQueue.Push({.type = ui::UiCommandType::BackToMenu});
            return;
        }

        std::optional<ui::UiWidgetId> activateWidget;
        if (inputState.primaryPressed && interactionState.hoveredWidget.has_value()) {
            activateWidget = interactionState.hoveredWidget;
        } else if (inputState.acceptPressed) {
            activateWidget = interactionState.focusedWidget;
        }

        if (!activateWidget.has_value()) {
            return;
        }

        const core::JoinFormField field = ui::JoinFieldForWidgetId(*activateWidget);
        interactionState.pressedWidget = activateWidget;

        if (ui::JoinServerScreenState::IsEditableField(field)) {
            commandQueue.Push({
                .type = ui::UiCommandType::StartJoinFieldEdit,
                .joinField = field,
            });
            return;
        }

        if (field == core::JoinFormField::Connect) {
            commandQueue.Push({.type = ui::UiCommandType::SubmitJoin});
            return;
        }

        if (field == core::JoinFormField::Back) {
            commandQueue.Push({.type = ui::UiCommandType::BackToMenu});
        }
        (void)document;
    }

    void ClientRuntime::HandleOptionsInteraction(flecs::world world, const ui::UiDocument& document,
                                                 const ui::UiInputState& inputState) {
        ui::OptionsScreenState& optionsScreenState = world.get_mut<ui::OptionsScreenState>();
        ui::UiInteractionState& interactionState = world.get_mut<ui::UiInteractionState>();
        ui::UiCommandQueue& commandQueue = world.get_mut<ui::UiCommandQueue>();
        interactionState.pressedWidget.reset();

        ApplyOptionsTextInput(optionsScreenState, inputState);

        if (!optionsScreenState.editing) {
            if (inputState.navigateDownPressed) {
                optionsScreenState.MoveNext();
                interactionState.focusedWidget = ui::UiWidgetIdForOptionsField(optionsScreenState.SelectedField());
            }
            if (inputState.navigateUpPressed) {
                optionsScreenState.MovePrevious();
                interactionState.focusedWidget = ui::UiWidgetIdForOptionsField(optionsScreenState.SelectedField());
            }
        }

        if (optionsScreenState.editing && (inputState.acceptPressed || inputState.backPressed)) {
            commandQueue.Push({
                .type = ui::UiCommandType::StopOptionsFieldEdit,
                .optionsField = optionsScreenState.SelectedField(),
            });
            return;
        }

        if (!optionsScreenState.editing && inputState.backPressed) {
            commandQueue.Push({.type = ui::UiCommandType::BackToMenu});
            return;
        }

        std::optional<ui::UiWidgetId> activateWidget;
        if (inputState.primaryPressed && interactionState.hoveredWidget.has_value()) {
            activateWidget = interactionState.hoveredWidget;
        } else if (inputState.acceptPressed) {
            activateWidget = interactionState.focusedWidget;
        }

        if (!activateWidget.has_value()) {
            return;
        }

        const ui::OptionsField field = ui::OptionsFieldForWidgetId(*activateWidget);
        interactionState.pressedWidget = activateWidget;

        if (ui::OptionsScreenState::IsEditableField(field)) {
            commandQueue.Push({
                .type = ui::UiCommandType::StartOptionsFieldEdit,
                .optionsField = field,
            });
            return;
        }

        if (field == ui::OptionsField::DebugOverlay) {
            commandQueue.Push({
                .type = ui::UiCommandType::ToggleOptionsDebugOverlay,
                .optionsField = field,
            });
            return;
        }

        if (field == ui::OptionsField::Save) {
            commandQueue.Push({
                .type = ui::UiCommandType::SaveOptions,
                .optionsField = field,
            });
            return;
        }

        if (field == ui::OptionsField::Back) {
            commandQueue.Push({.type = ui::UiCommandType::BackToMenu});
        }
        (void)document;
    }

    bool ClientRuntime::ApplyJoinFormToConfig(const ui::JoinServerScreenState& joinScreenState) {
        ClientFlowState& flow = FlowState();
        if (joinScreenState.host.empty()) {
            flow.statusMessage = "Host is required";
            return false;
        }
        if (joinScreenState.playerName.empty()) {
            flow.statusMessage = "Player name is required";
            return false;
        }
        if (joinScreenState.port.empty()) {
            flow.statusMessage = "Port is required";
            return false;
        }

        uint32_t parsedPort = 0;
        const char* begin = joinScreenState.port.data();
        const char* end = begin + joinScreenState.port.size();
        const auto [ptr, error] = std::from_chars(begin, end, parsedPort);
        if (error != std::errc{} || ptr != end || parsedPort == 0 || parsedPort > 65535) {
            flow.statusMessage = "Port must be between 1 and 65535";
            return false;
        }

        config_.serverHost = joinScreenState.host;
        config_.serverPort = static_cast<uint16_t>(parsedPort);
        config_.playerName = joinScreenState.playerName;
        return true;
    }

    bool ClientRuntime::ApplyOptionsToConfig(flecs::world world, ui::OptionsScreenState& optionsScreenState) {
        ClientFlowState& flow = world.get_mut<ClientFlowState>();
        auto parseNumber = [&flow](const std::string& value, int minimum, int maximum, const char* label,
                                   int& out) -> bool {
            if (value.empty()) {
                flow.statusMessage = std::string{label} + " is required";
                return false;
            }

            const char* begin = value.data();
            const char* end = begin + value.size();
            const auto [ptr, error] = std::from_chars(begin, end, out);
            if (error != std::errc{} || ptr != end || out < minimum || out > maximum) {
                flow.statusMessage = std::string{label} + " must be between " + std::to_string(minimum) + " and " +
                    std::to_string(maximum);
                return false;
            }

            return true;
        };

        if (optionsScreenState.playerName.empty()) {
            flow.statusMessage = "Player name is required";
            return false;
        }
        if (optionsScreenState.host.empty()) {
            flow.statusMessage = "Default host is required";
            return false;
        }

        int parsedPort = 0;
        int parsedWindowWidth = 0;
        int parsedWindowHeight = 0;
        int parsedTargetFps = 0;
        int parsedInterpolationDelay = 0;
        if (!parseNumber(optionsScreenState.port, 1, 65535, "Default port", parsedPort) ||
            !parseNumber(optionsScreenState.windowWidth, 640, 3840, "Window width", parsedWindowWidth) ||
            !parseNumber(optionsScreenState.windowHeight, 360, 2160, "Window height", parsedWindowHeight) ||
            !parseNumber(optionsScreenState.targetFps, 30, 360, "Target FPS", parsedTargetFps) ||
            !parseNumber(optionsScreenState.interpolationDelay, 0, 10, "Interpolation delay",
                         parsedInterpolationDelay)) {
            return false;
        }

        config_.playerName = optionsScreenState.playerName;
        config_.serverHost = optionsScreenState.host;
        config_.serverPort = static_cast<uint16_t>(parsedPort);
        config_.windowWidth = parsedWindowWidth;
        config_.windowHeight = parsedWindowHeight;
        config_.targetFps = parsedTargetFps;
        config_.interpolationDelayTicks = parsedInterpolationDelay;
        config_.debugOverlayDefault = optionsScreenState.debugOverlayDefault;

        if (window_.has_value()) {
            ::SetWindowSize(config_.windowWidth, config_.windowHeight);
            ::SetTargetFPS(config_.targetFps);
        }
        flow.debugOverlayEnabled = config_.debugOverlayDefault;

        world.get_mut<ui::JoinServerScreenState>().ResetFromDefaults(config_.serverHost, config_.serverPort,
                                                                     config_.playerName);

        std::string saveError;
        const std::filesystem::path configPath = config_.configFilePath.empty()
            ? core::DefaultClientConfigPath()
            : std::filesystem::path{config_.configFilePath};
        config_.configFilePath = configPath.string();
        if (!core::SaveClientConfigFile(config_, configPath, saveError)) {
            flow.statusMessage = "Save failed: " + saveError;
            return false;
        }

        flow.statusMessage = "Saved client preferences to " + configPath.string();
        return true;
    }

    void ClientRuntime::ActivateMenuAction(flecs::world world, core::MenuAction action) {
        ClientFlowState& flow = world.get_mut<ClientFlowState>();
        switch (action) {
            case core::MenuAction::JoinServer:
                flow.runtime.requestedJoin = false;
                flow.runtime.mode = core::RuntimeMode::JoiningServer;
                flow.runtime.joiningInProgress = false;
                world.get_mut<ui::JoinServerScreenState>().editing = false;
                world.get_mut<ui::OptionsScreenState>().editing = false;
                world.get_mut<ui::UiInteractionState>().focusedWidget = ui::UiWidgetId::JoinHost;
                flow.statusMessage = "Select a field and press Enter to edit";
                flow.disconnectReason.clear();
                flow.runtime.disconnectReason.clear();
                return;
            case core::MenuAction::Quit:
                exitRequested_ = true;
                return;
            case core::MenuAction::StartServer:
                flow.runtime.requestedJoin = false;
                flow.runtime.requestedLocalServerStart = true;
                flow.runtime.joiningInProgress = false;
                flow.runtime.mode = core::RuntimeMode::StartingLocalServer;
                flow.disconnectReason.clear();
                flow.runtime.disconnectReason.clear();
                flow.statusMessage = "Launching local dedicated server...";
                world.get_mut<ui::JoinServerScreenState>().editing = false;
                world.get_mut<ui::OptionsScreenState>().editing = false;
                world.get_mut<ui::UiInteractionState>().focusedWidget.reset();
                return;
            case core::MenuAction::Singleplayer:
                BeginSingleplayer(world);
                return;
            case core::MenuAction::Options:
                world.get_mut<ui::OptionsScreenState>().ResetFromConfig(config_);
                flow.runtime.mode = core::RuntimeMode::Options;
                flow.statusMessage = "Select a field and press Enter to edit";
                world.get_mut<ui::JoinServerScreenState>().editing = false;
                world.get_mut<ui::UiInteractionState>().focusedWidget = ui::UiWidgetId::OptionsPlayerName;
                return;
            case core::MenuAction::None:
                return;
        }
    }

    bool ClientRuntime::BeginJoinServer(flecs::world world) {
        if (connecting_ || connected_) {
            return true;
        }

        ClientFlowState& flow = world.get_mut<ClientFlowState>();
        flow.runtime.mode = core::RuntimeMode::JoiningServer;
        flow.runtime.joiningInProgress = false;
        world.get_mut<ui::JoinServerScreenState>().editing = false;
        world.get_mut<ui::OptionsScreenState>().editing = false;
        flow.statusMessage.clear();
        flow.disconnectReason.clear();
        flow.runtime.disconnectReason.clear();

        std::string error;
        if (!EnsureTransportInitialized(error)) {
            flow.statusMessage = "Transport init failed: " + error;
            std::fprintf(stderr, "[net.transport] client transport init failed: %s\n", error.c_str());
            return false;
        }

        if (!BeginConnectionAttempt(error)) {
            flow.statusMessage = "Connect failed: " + error;
            std::fprintf(stderr, "[net.transport] connect failed: %s\n", error.c_str());
            return false;
        }

        connecting_ = true;
        flow.runtime.joiningInProgress = true;
        flow.statusMessage = "Connecting...";
        return true;
    }

    void ClientRuntime::BeginSingleplayer(flecs::world world) {
        ClientFlowState& flow = world.get_mut<ClientFlowState>();
        ResetSessionState();
        flow.disconnectReason.clear();
        flow.runtime.disconnectReason.clear();
        flow.runtime.requestedJoin = false;
        flow.runtime.requestedLocalServerStart = false;
        flow.runtime.joiningInProgress = false;

        singleplayerRuntime_.Start(config_.playerName);
        if (const game::PlayerState* localPlayer = singleplayerRuntime_.LocalPlayer(); localPlayer != nullptr) {
            predictedLocalPlayer_ = *localPlayer;
            localPlayerId = localPlayer->playerId;
        }

        serverKinematics_ = singleplayerRuntime_.Kinematics();
        flow.runtime.mode = core::RuntimeMode::Singleplayer;
        flow.statusMessage.clear();
        world.get_mut<ui::JoinServerScreenState>().editing = false;
        world.get_mut<ui::OptionsScreenState>().editing = false;
        world.get_mut<ui::UiInteractionState>().focusedWidget.reset();
    }

    bool ClientRuntime::EnsureTransportInitialized(std::string& error) {
        if (transport_.IsInitialized()) {
            error.clear();
            return true;
        }

        if (!transport_.Initialize(
                net::TransportConfig{.isServer = false, .debugVerbosity = 4, .allowUnencryptedDev = true}, error)) {
            return false;
        }

        error.clear();
        return true;
    }

    bool ClientRuntime::BeginConnectionAttempt(std::string& error) {
        serverConnection_ = transport_.Connect(config_.serverHost, config_.serverPort, error);
        return serverConnection_ != net::kInvalidConnectionHandle;
    }

    void ClientRuntime::ReturnToMenu(flecs::world world, std::string statusMessage) {
        ClientFlowState& flow = world.get_mut<ClientFlowState>();
        ResetSessionState();
        flow.statusMessage = MenuStatusMessageForReturn(flow, std::move(statusMessage));
        flow.disconnectReason.clear();
        flow.runtime.disconnectReason.clear();
        flow.runtime.requestedJoin = false;
        flow.runtime.requestedLocalServerStart = false;
        flow.runtime.joiningInProgress = false;
        world.get_mut<ui::JoinServerScreenState>().editing = false;
        world.get_mut<ui::OptionsScreenState>().editing = false;
        flow.runtime.mode = core::RuntimeMode::Menu;
        world.get_mut<ui::UiInteractionState>().focusedWidget = ui::UiWidgetId::MenuStartServer;
    }

    void ClientRuntime::FailLocalServerStartup(flecs::world world, const std::string& message) {
        LocalServerStartupState& localServer = world.get_mut<LocalServerStartupState>();
        ClientFlowState& flow = world.get_mut<ClientFlowState>();
        ResetSessionState();
        localServer.startupInProgress = false;
        flow.runtime.requestedLocalServerStart = false;
        StopOwnedLocalServer();
        ReturnToMenu(world, message);
    }

    void ClientRuntime::StopOwnedLocalServer() {
        LocalServerStartupState& localServer = LocalServerState();
        if (!localServer.ownsProcess || !serverLauncher_) {
            return;
        }

        serverLauncher_->Stop();
        localServer.ownsProcess = false;
    }

    void ClientRuntime::HandleConnectionEvents() {
        ClientFlowState& flow = FlowState();
        LocalServerStartupState& localServer = LocalServerState();
        const std::vector<net::ConnectionEvent> events = transport_.DrainConnectionEvents();
        for (const net::ConnectionEvent& event : events) {
            if (event.connection != serverConnection_) {
                continue;
            }

            if (event.type == net::ConnectionEventType::Connected) {
                connected_ = true;
                connecting_ = false;
                flow.statusMessage = localServer.startupInProgress
                    ? "Connected to local dedicated server, waiting for server welcome..."
                    : "Connected, waiting for server welcome...";
                OnConnectedToServer();
                continue;
            }

            if (event.type == net::ConnectionEventType::ClosedByPeer ||
                event.type == net::ConnectionEventType::ProblemDetectedLocally) {
                connected_ = false;
                flow.disconnectReason = event.reason.empty() ? "connection closed" : event.reason;
                ResetSessionState();

                if (localServer.startupInProgress) {
                    flow.disconnectReason.clear();
                    flow.runtime.disconnectReason.clear();
                    flow.runtime.mode = core::RuntimeMode::StartingLocalServer;
                    flow.runtime.joiningInProgress = false;
                    flow.statusMessage = "Waiting for local dedicated server...";
                    continue;
                }

                if (flow.runtime.mode == core::RuntimeMode::JoiningServer) {
                    flow.statusMessage = "Join failed: " + flow.disconnectReason;
                    flow.disconnectReason.clear();
                    flow.runtime.disconnectReason.clear();
                }
            }
        }
    }

    void ClientRuntime::HandleIncomingPackets() {
        ClientFlowState& flow = FlowState();
        const std::vector<net::ReceivedPacket> packets = transport_.DrainReceivedPackets();
        for (const net::ReceivedPacket& packet : packets) {
            if (packet.connection != serverConnection_) {
                continue;
            }

            net::EnvelopeHeader header;
            std::span<const uint8_t> payload;
            std::string parseError;
            if (!net::ParsePacket(packet.bytes, header, payload, parseError)) {
                std::fprintf(stderr, "[net.protocol] drop malformed packet: %s\n", parseError.c_str());
                continue;
            }

            if (header.protocolVersion != net::kProtocolVersion) {
                flow.disconnectReason = "protocol version mismatch";
                connected_ = false;
                ResetSessionState();
                transport_.Close(serverConnection_, 4002, flow.disconnectReason, false);
                continue;
            }

            switch (header.messageId) {
                case net::MessageId::ServerWelcome:
                    {
                        net::ServerWelcomeMessage welcome;
                        if (net::Deserialize(payload, welcome, parseError)) {
                            HandleServerWelcome(welcome);
                        }
                        break;
                    }
                case net::MessageId::WorldMetadata:
                    {
                        net::WorldMetadataMessage metadata;
                        if (net::Deserialize(payload, metadata, parseError)) {
                            HandleWorldMetadata(metadata);
                        }
                        break;
                    }
                case net::MessageId::SpawnPlayer:
                    {
                        net::SpawnPlayerMessage spawn;
                        if (net::Deserialize(payload, spawn, parseError)) {
                            HandleSpawnPlayer(spawn);
                        }
                        break;
                    }
                case net::MessageId::DespawnEntity:
                    {
                        net::DespawnEntityMessage despawn;
                        if (net::Deserialize(payload, despawn, parseError)) {
                            HandleDespawnEntity(despawn);
                        }
                        break;
                    }
                case net::MessageId::SnapshotBaseline:
                case net::MessageId::SnapshotDelta:
                    {
                        net::ByteReader reader(payload);
                        net::SnapshotPayload snapshot;
                        if (net::DeserializeSnapshotPayload(reader, snapshot, parseError)) {
                            HandleSnapshot(snapshot);
                        }
                        break;
                    }
                case net::MessageId::ChunkBaseline:
                    {
                        net::ChunkBaselineMessage baseline;
                        if (net::Deserialize(payload, baseline, parseError)) {
                            HandleChunkBaseline(baseline);
                        }
                        break;
                    }
                case net::MessageId::ChunkDelta:
                    {
                        net::ChunkDeltaMessage delta;
                        if (net::Deserialize(payload, delta, parseError)) {
                            HandleChunkDelta(delta);
                        }
                        break;
                    }
                case net::MessageId::ChunkUnsubscribe:
                    {
                        net::ChunkUnsubscribeMessage unsubscribe;
                        if (net::Deserialize(payload, unsubscribe, parseError)) {
                            HandleChunkUnsubscribe(unsubscribe);
                        }
                        break;
                    }
                case net::MessageId::ResyncRequired:
                    {
                        net::ResyncRequiredMessage resync;
                        if (net::Deserialize(payload, resync, parseError)) {
                            HandleResyncRequired(resync);
                        }
                        break;
                    }
                case net::MessageId::DisconnectReason:
                    {
                        net::DisconnectReasonMessage reason;
                        if (net::Deserialize(payload, reason, parseError)) {
                            HandleDisconnectReason(reason);
                        }
                        break;
                    }
                case net::MessageId::Pong:
                default:
                    break;
            }
        }
    }

    void ClientRuntime::OnConnectedToServer() {
        ClientFlowState& flow = FlowState();
        net::ClientHelloMessage hello;
        hello.requestedProtocolVersion = net::kProtocolVersion;
        hello.buildCompatibilityHash = config_.buildCompatibilityHash;
        hello.playerName = config_.playerName;
        hello.authToken = "dev";

        const std::vector<uint8_t> payload = net::Serialize(hello);
        const std::vector<uint8_t> packet = net::BuildPacket(net::MessageId::ClientHello, payload);

        std::string error;
        if (!transport_.Send(
                serverConnection_, packet,
                net::SendOptionsForMessage(net::MessageId::ClientHello, net::MessageDirection::ClientToServer),
                error)) {
            flow.disconnectReason = "failed to send ClientHello: " + error;
            connected_ = false;
            ResetSessionState();
        }
    }

    void ClientRuntime::HandleServerWelcome(const net::ServerWelcomeMessage& message) {
        ClientFlowState& flow = FlowState();
        if (message.protocolVersion != net::kProtocolVersion) {
            flow.disconnectReason = "protocol mismatch";
            connected_ = false;
            ResetSessionState();
            transport_.Close(serverConnection_, 4003, flow.disconnectReason, false);
            return;
        }

        const game::PlayerKinematicsValidationError kinematicsValidation =
            game::ValidatePlayerKinematicsConfig(message.playerKinematics);
        if (kinematicsValidation != game::PlayerKinematicsValidationError::None) {
            flow.disconnectReason = std::string{"invalid server kinematics: "} + game::ToString(kinematicsValidation);
            connected_ = false;
            ResetSessionState();
            transport_.Close(serverConnection_, 4004, flow.disconnectReason, false);
            return;
        }

        serverWelcomed_ = true;
        localPlayerId = message.playerId;
        latestServerTick_ = message.serverTick;
        renderInterpolationTick_ = static_cast<float>(message.serverTick);
        serverTickRateHz_ = std::max<uint16_t>(1, message.serverTickRateHz);
        serverSnapshotRateHz_ = std::max<uint16_t>(1, message.snapshotRateHz);
        serverKinematics_ = message.playerKinematics;
        fixedStep_.SetStepSeconds(1.0 / static_cast<double>(serverTickRateHz_));

        predictedLocalPlayer_.playerId = message.playerId;
        predictedLocalPlayer_.entityId = game::EntityId{message.playerId.Value()};
        predictedLocalPlayer_.displayName = config_.playerName;
        predictedLocalPlayer_.position = {0.0f, 0.0f};
        predictedLocalPlayer_.velocity = {0.0f, 0.0f};
        predictedLocalPlayer_.onGround = true;
        pendingInputs_.clear();
        chunksByCoord_.clear();
        chunkVersionConflictCount_ = 0;
        serverWorldConfig_ = game::WorldConfig{};
        hasWorldMetadata_ = false;
    }

    void ClientRuntime::HandleWorldMetadata(const net::WorldMetadataMessage& message) {
        if (message.chunkWidthTiles == 0 || message.chunkHeightTiles == 0 || message.tileSize == 0 ||
            message.defaultInterestRadiusChunks == 0) {
            std::fprintf(stderr, "[world.chunk] invalid world metadata ignored\n");
            return;
        }

        serverWorldConfig_.chunkWidthTiles = static_cast<int>(message.chunkWidthTiles);
        serverWorldConfig_.chunkHeightTiles = static_cast<int>(message.chunkHeightTiles);
        serverWorldConfig_.tileSize = static_cast<int>(message.tileSize);
        serverWorldConfig_.interestRadiusChunks = static_cast<int>(message.defaultInterestRadiusChunks);
        hasWorldMetadata_ = true;
    }

    void ClientRuntime::HandleSpawnPlayer(const net::SpawnPlayerMessage& message) {
        if (message.playerId == localPlayerId) {
            predictedLocalPlayer_.position = message.spawnPosition;
            predictedLocalPlayer_.entityId = message.entityId;
            predictedLocalPlayer_.displayName = message.displayName;
            return;
        }

        RemotePlayerView& remote = remotePlayers_[message.playerId];
        remote.playerId = message.playerId;
        remote.entityId = message.entityId;
        remote.displayName = message.displayName;
        remote.latestPosition = message.spawnPosition;
        remote.interpolation.Push({.tick = latestServerTick_, .position = message.spawnPosition});
    }

    void ClientRuntime::HandleDespawnEntity(const net::DespawnEntityMessage& message) {
        for (auto it = remotePlayers_.begin(); it != remotePlayers_.end(); ++it) {
            if (it->second.entityId == message.entityId) {
                remotePlayers_.erase(it);
                return;
            }
        }
    }

    void ClientRuntime::HandleSnapshot(const net::SnapshotPayload& snapshot) {
        latestServerTick_ = snapshot.serverTick;

        std::unordered_set<game::PlayerId, game::IdHash<game::PlayerIdTag>> seenRemotePlayers;
        for (const net::SnapshotEntity& entity : snapshot.entities) {
            if (entity.playerId == localPlayerId) {
                ReconcileFromSnapshot(entity);
                continue;
            }

            RemotePlayerView& remote = remotePlayers_[entity.playerId];
            remote.playerId = entity.playerId;
            remote.entityId = entity.entityId;
            remote.displayName = entity.displayName;
            remote.latestPosition = entity.position;
            remote.interpolation.Push({.tick = snapshot.serverTick, .position = entity.position});
            seenRemotePlayers.insert(entity.playerId);
        }

        for (auto it = remotePlayers_.begin(); it != remotePlayers_.end();) {
            if (!seenRemotePlayers.contains(it->first)) {
                it = remotePlayers_.erase(it);
            } else {
                ++it;
            }
        }
    }

    void ClientRuntime::HandleChunkBaseline(const net::ChunkBaselineMessage& message) {
        if (!message.chunk.IsValid()) {
            std::fprintf(stderr, "[world.chunk] drop invalid baseline %d,%d\n", message.chunk.coord.x,
                         message.chunk.coord.y);
            return;
        }

        ClientChunkState& chunk = chunksByCoord_[message.chunk.coord];
        chunk.chunk = message.chunk;
        chunkResyncRequestedAt_.erase(message.chunk.coord);
    }

    void ClientRuntime::HandleChunkDelta(const net::ChunkDeltaMessage& message) {
        auto chunkIt = chunksByCoord_.find(message.delta.coord);
        if (chunkIt == chunksByCoord_.end()) {
            ++chunkVersionConflictCount_;
            std::fprintf(stderr, "[world.chunk] delta for unknown chunk %d,%d\n", message.delta.coord.x,
                         message.delta.coord.y);
            RequestChunkResync(message.delta.coord, 0U);
            return;
        }

        game::ChunkData& chunk = chunkIt->second.chunk;
        if (chunk.version.value != message.delta.baseVersion.value) {
            ++chunkVersionConflictCount_;
            std::fprintf(stderr, "[world.chunk] version mismatch chunk %d,%d local=%u base=%u\n", message.delta.coord.x,
                         message.delta.coord.y, chunk.version.value, message.delta.baseVersion.value);
            RequestChunkResync(message.delta.coord, chunk.version.value);
            return;
        }

        if (!game::ApplyChunkDelta(chunk, message.delta)) {
            ++chunkVersionConflictCount_;
            std::fprintf(stderr, "[world.chunk] invalid delta ops chunk %d,%d ops=%zu\n", message.delta.coord.x,
                         message.delta.coord.y, message.delta.operations.size());
            RequestChunkResync(message.delta.coord, chunk.version.value);
        }
    }

    void ClientRuntime::HandleChunkUnsubscribe(const net::ChunkUnsubscribeMessage& message) {
        const game::ChunkCoord coord{
            .x = message.chunkX,
            .y = message.chunkY,
        };
        chunksByCoord_.erase(coord);
        chunkResyncRequestedAt_.erase(coord);
    }

    void ClientRuntime::HandleResyncRequired(const net::ResyncRequiredMessage& message) {
        ClientFlowState& flow = FlowState();
        chunksByCoord_.clear();
        chunkResyncRequestedAt_.clear();
        chunkVersionConflictCount_ = 0;
        flow.disconnectReason = message.reason;
        lastChunkHintSentAt_ = std::chrono::steady_clock::now() - std::chrono::milliseconds(1000);
    }

    void ClientRuntime::HandleDisconnectReason(const net::DisconnectReasonMessage& message) {
        ClientFlowState& flow = FlowState();
        flow.disconnectReason = message.reason;
        connected_ = false;
        ResetSessionState();

        if (flow.runtime.mode == core::RuntimeMode::JoiningServer) {
            flow.statusMessage = "Join failed: " + flow.disconnectReason;
            flow.disconnectReason.clear();
            flow.runtime.disconnectReason.clear();
        }
    }

    void ClientRuntime::ResetSessionState() {
        FlowState().runtime.joiningInProgress = false;
        singleplayerRuntime_.Stop();
        connected_ = false;
        connecting_ = false;
        serverWelcomed_ = false;
        localPlayerId = {};
        predictedLocalPlayer_ = {};
        pendingInputs_.clear();
        remotePlayers_.clear();
        chunksByCoord_.clear();
        chunkResyncRequestedAt_.clear();
        chunkVersionConflictCount_ = 0;
        serverWorldConfig_ = game::WorldConfig{};
        hasWorldMetadata_ = false;
        clientTick_ = 0;
        latestServerTick_ = 0;
        renderInterpolationTick_ = 0.0f;
        nextInputSequence_ = 1;
        serverConnection_ = net::kInvalidConnectionHandle;
    }

    void ClientRuntime::RefreshRuntimeState() {
        RefreshClientFlowState(FlowState(), LocalServerState(),
                               {
                                   .connecting = connecting_,
                                   .connected = connected_,
                                   .serverWelcomed = serverWelcomed_,
                                   .singleplayerActive = singleplayerRuntime_.IsActive(),
                               });
    }

    void ClientRuntime::StepSimulation() {
        if (FlowState().runtime.mode == core::RuntimeMode::Singleplayer) {
            if (!singleplayerRuntime_.IsActive()) {
                return;
            }

            game::PlayerInputFrame inputFrame = inputManager_.BuildPlayerInputFrame(clientTick_, nextInputSequence_++);
            ++clientTick_;

            singleplayerRuntime_.Step(inputFrame, static_cast<float>(fixedStep_.StepSeconds()));
            if (const game::PlayerState* localPlayer = singleplayerRuntime_.LocalPlayer(); localPlayer != nullptr) {
                predictedLocalPlayer_ = *localPlayer;
                localPlayerId = localPlayer->playerId;
            }
            latestServerTick_ = singleplayerRuntime_.CurrentTick();
            renderInterpolationTick_ = static_cast<float>(latestServerTick_);
            return;
        }

        if (FlowState().runtime.mode != core::RuntimeMode::Multiplayer || !connected_ || !serverWelcomed_ ||
            !IsLocalPlayerReady()) {
            return;
        }

        game::PlayerInputFrame inputFrame = inputManager_.BuildPlayerInputFrame(clientTick_, nextInputSequence_++);
        ++clientTick_;

        SendInputFrame(inputFrame);
        pendingInputs_.push_back(inputFrame);

        physics::MovementSystem::Predict(predictedLocalPlayer_, inputFrame,
                                         static_cast<float>(fixedStep_.StepSeconds()), serverKinematics_);
    }

    void ClientRuntime::SendInputFrame(const game::PlayerInputFrame& frame) {
        net::InputFrameMessage inputMessage;
        inputMessage.clientTick = frame.clientTick;
        inputMessage.sequence = frame.sequence;
        inputMessage.moveX = frame.moveX;
        inputMessage.jumpPressed = frame.jumpPressed;

        const std::vector<uint8_t> payload = net::Serialize(inputMessage);
        const std::vector<uint8_t> packet = net::BuildPacket(net::MessageId::InputFrame, payload);

        std::string error;
        if (!transport_.Send(
                serverConnection_, packet,
                net::SendOptionsForMessage(net::MessageId::InputFrame, net::MessageDirection::ClientToServer), error)) {
            std::fprintf(stderr, "[net.protocol] input send failed: %s\n", error.c_str());
        }
    }

    void ClientRuntime::SendChunkInterestHint() {
        if (!connected_ || !serverWelcomed_ || !IsLocalPlayerReady()) {
            return;
        }

        const game::WorldConfig& worldConfig = hasWorldMetadata_ ? serverWorldConfig_ : game::WorldConfig{};
        const game::ChunkCoord center = game::WorldToChunkCoord(predictedLocalPlayer_.position, worldConfig);
        const net::ChunkInterestHintMessage hint{
            .centerChunkX = center.x,
            .centerChunkY = center.y,
            .radiusChunks = static_cast<uint16_t>(std::clamp(worldConfig.interestRadiusChunks, 1, 24)),
        };

        const std::vector<uint8_t> payload = net::Serialize(hint);
        const std::vector<uint8_t> packet = net::BuildPacket(net::MessageId::ChunkInterestHint, payload);

        std::string error;
        transport_.Send(
            serverConnection_, packet,
            net::SendOptionsForMessage(net::MessageId::ChunkInterestHint, net::MessageDirection::ClientToServer),
            error);
    }

    void ClientRuntime::RequestChunkResync(const game::ChunkCoord& coord, uint32_t clientVersion) {
        if (!connected_ || !serverWelcomed_) {
            return;
        }

        const auto now = std::chrono::steady_clock::now();
        const auto it = chunkResyncRequestedAt_.find(coord);
        if (it != chunkResyncRequestedAt_.end()) {
            const auto since = std::chrono::duration_cast<std::chrono::milliseconds>(now - it->second);
            if (since.count() < 250) {
                return;
            }
        }

        const net::ChunkResyncRequestMessage request{
            .chunkX = coord.x,
            .chunkY = coord.y,
            .clientVersion = clientVersion,
        };

        const std::vector<uint8_t> payload = net::Serialize(request);
        const std::vector<uint8_t> packet = net::BuildPacket(net::MessageId::ChunkResyncRequest, payload);
        std::string error;
        if (transport_.Send(
                serverConnection_, packet,
                net::SendOptionsForMessage(net::MessageId::ChunkResyncRequest, net::MessageDirection::ClientToServer),
                error)) {
            chunkResyncRequestedAt_[coord] = now;
        }
    }

    void ClientRuntime::ReconcileFromSnapshot(const net::SnapshotEntity& localEntity) {
        physics::MovementSystem::Reconcile(predictedLocalPlayer_, localEntity, pendingInputs_,
                                           static_cast<float>(fixedStep_.StepSeconds()), serverKinematics_);
    }

    void ClientRuntime::PublishScreenState(flecs::world world) {
        RefreshRuntimeState();
        const ClientFlowState& flow = world.get<ClientFlowState>();
        core::Application::UpdateScene(sceneManager_, flow.runtime);

        world.set<ui::ScreenState>({
            .mode = flow.runtime.mode,
            .activeScene = sceneManager_.ActiveScene(),
            .joiningInProgress = flow.runtime.joiningInProgress,
            .statusMessage = ActiveScreenStatusMessage(flow),
            .disconnectReason = flow.disconnectReason,
        });
    }

    ui::UiDocument ClientRuntime::BuildMenuDocument(const ui::ScreenState& screenState,
                                                    const ui::MenuScreenState& menuScreenState,
                                                    const ui::UiInteractionState& interactionState) const {
        ui::UiDocument document;
        document.scene = screenState.activeScene;
        document.title = "Main Menu";
        document.subtitle = "Select runtime mode";
        document.statusMessage = screenState.statusMessage;
        document.footerHint = "Navigate: W/S or Up/Down | Select: Enter/Space (A) | Mouse: hover + click";

        const float windowWidth = window_.has_value() ? static_cast<float>(raylib::Window::GetWidth())
                                                      : static_cast<float>(config_.windowWidth);
        const float left = windowWidth * 0.5f - (kMenuWidth * 0.5f);
        document.widgets.reserve(menuScreenState.Actions().size());

        for (size_t index = 0; index < menuScreenState.Actions().size(); ++index) {
            const core::MenuAction action = menuScreenState.Actions()[index];
            const ui::UiWidgetId widgetId = ui::UiWidgetIdForMenuAction(action);
            document.widgets.push_back({
                .id = widgetId,
                .kind = ui::UiWidgetKind::Button,
                .bounds =
                    ui::UiRect{left, kMenuStartY + static_cast<float>(index) * kRowSpacing, kMenuWidth, kRowHeight},
                .label = std::string{core::MenuActionName(action)},
                .state =
                    ui::UiWidgetState{
                        .hovered = interactionState.hoveredWidget == widgetId,
                        .focused = interactionState.focusedWidget == widgetId,
                        .pressed = interactionState.pressedWidget == widgetId,
                    },
            });
        }

        return document;
    }

    ui::UiDocument ClientRuntime::BuildJoinDocument(const ui::ScreenState& screenState,
                                                    const ui::JoinServerScreenState& joinScreenState,
                                                    const ui::UiInteractionState& interactionState) const {
        ui::UiDocument document;
        document.scene = screenState.activeScene;
        document.title = "Join Server";
        document.subtitle = "Configure host, port, and player name";
        document.statusMessage = screenState.statusMessage;
        document.footerHint = joinScreenState.editing
            ? "Editing: type text, Backspace to erase, Enter/Esc to finish"
            : "Navigate: W/S or Up/Down | Select: Enter/Space (A) | Back: Esc (B) | Mouse: hover + click";

        const float windowWidth = window_.has_value() ? static_cast<float>(raylib::Window::GetWidth())
                                                      : static_cast<float>(config_.windowWidth);
        const float left = windowWidth * 0.5f - (kJoinWidth * 0.5f);
        document.widgets.reserve(ui::JoinServerScreenState::kFields.size());

        for (size_t index = 0; index < ui::JoinServerScreenState::kFields.size(); ++index) {
            const core::JoinFormField field = ui::JoinServerScreenState::kFields[index];
            const ui::UiWidgetId widgetId = ui::UiWidgetIdForJoinField(field);

            std::string label = std::string{core::JoinFormFieldName(field)};
            std::string value;
            if (field == core::JoinFormField::Host) {
                value = joinScreenState.host;
            } else if (field == core::JoinFormField::Port) {
                value = joinScreenState.port;
            } else if (field == core::JoinFormField::Name) {
                value = joinScreenState.playerName;
            }

            document.widgets.push_back({
                .id = widgetId,
                .kind = ui::JoinServerScreenState::IsEditableField(field) ? ui::UiWidgetKind::TextField
                                                                          : ui::UiWidgetKind::Button,
                .bounds =
                    ui::UiRect{left, kJoinStartY + static_cast<float>(index) * kRowSpacing, kJoinWidth, kRowHeight},
                .label = std::move(label),
                .value = std::move(value),
                .state =
                    ui::UiWidgetState{
                        .hovered = interactionState.hoveredWidget == widgetId,
                        .focused = interactionState.focusedWidget == widgetId,
                        .pressed = interactionState.pressedWidget == widgetId,
                        .editing = joinScreenState.editing && interactionState.focusedWidget == widgetId &&
                            ui::JoinServerScreenState::IsEditableField(field),
                    },
            });
        }

        return document;
    }

    ui::UiDocument ClientRuntime::BuildOptionsDocument(const ui::ScreenState& screenState,
                                                       const ui::OptionsScreenState& optionsScreenState,
                                                       const ui::UiInteractionState& interactionState) const {
        ui::UiDocument document;
        document.scene = screenState.activeScene;
        document.title = "Options";
        document.subtitle = "Persist local client preferences";
        document.statusMessage = screenState.statusMessage;
        document.footerHint = optionsScreenState.editing
            ? "Editing: type text, Backspace to erase, Enter/Esc to finish"
            : "Navigate: W/S or Up/Down | Select: Enter/Space (A) | Back: Esc (B) | Mouse: hover + click";

        const float windowWidth = window_.has_value() ? static_cast<float>(raylib::Window::GetWidth())
                                                      : static_cast<float>(config_.windowWidth);
        const float left = windowWidth * 0.5f - (kOptionsWidth * 0.5f);
        document.widgets.reserve(ui::OptionsScreenState::kFields.size());

        for (size_t index = 0; index < ui::OptionsScreenState::kFields.size(); ++index) {
            const ui::OptionsField field = ui::OptionsScreenState::kFields[index];
            const ui::UiWidgetId widgetId = ui::UiWidgetIdForOptionsField(field);

            std::string label = std::string{ui::OptionsFieldName(field)};
            std::string value;
            if (field == ui::OptionsField::PlayerName) {
                value = optionsScreenState.playerName;
            } else if (field == ui::OptionsField::Host) {
                value = optionsScreenState.host;
            } else if (field == ui::OptionsField::Port) {
                value = optionsScreenState.port;
            } else if (field == ui::OptionsField::WindowWidth) {
                value = optionsScreenState.windowWidth;
            } else if (field == ui::OptionsField::WindowHeight) {
                value = optionsScreenState.windowHeight;
            } else if (field == ui::OptionsField::TargetFps) {
                value = optionsScreenState.targetFps;
            } else if (field == ui::OptionsField::InterpolationDelay) {
                value = optionsScreenState.interpolationDelay;
            } else if (field == ui::OptionsField::DebugOverlay) {
                label += optionsScreenState.debugOverlayDefault ? ": On" : ": Off";
            }

            document.widgets.push_back({
                .id = widgetId,
                .kind = ui::OptionsScreenState::IsEditableField(field) ? ui::UiWidgetKind::TextField
                                                                       : ui::UiWidgetKind::Button,
                .bounds =
                    ui::UiRect{
                        left,
                        kOptionsStartY + static_cast<float>(index) * kOptionsRowSpacing,
                        kOptionsWidth,
                        kRowHeight,
                    },
                .label = std::move(label),
                .value = std::move(value),
                .state =
                    ui::UiWidgetState{
                        .hovered = interactionState.hoveredWidget == widgetId,
                        .focused = interactionState.focusedWidget == widgetId,
                        .pressed = interactionState.pressedWidget == widgetId,
                        .editing = optionsScreenState.editing && interactionState.focusedWidget == widgetId &&
                            ui::OptionsScreenState::IsEditableField(field),
                    },
            });
        }

        return document;
    }

    components::WorldRenderState ClientRuntime::BuildWorldRenderState() const {
        components::WorldRenderState state;
        const ClientFlowState& flow = FlowState();

        if ((flow.runtime.mode == core::RuntimeMode::Singleplayer && singleplayerRuntime_.IsActive() &&
             localPlayerId.IsValid()) ||
            IsLocalPlayerReady()) {
            state.localPlayer = {
                .playerId = predictedLocalPlayer_.playerId,
                .entityId = predictedLocalPlayer_.entityId,
                .displayName = predictedLocalPlayer_.displayName,
                .position = predictedLocalPlayer_.position,
                .isLocal = true,
            };
        }

        state.remotePlayers.reserve(remotePlayers_.size());
        for (const auto& [playerId, remote] : remotePlayers_) {
            (void)playerId;
            state.remotePlayers.push_back({
                .playerId = remote.playerId,
                .entityId = remote.entityId,
                .displayName = remote.displayName,
                .position = remote.interpolation.SampleAt(renderInterpolationTick_),
                .isLocal = false,
            });
        }

        return state;
    }

    components::StatusRenderState ClientRuntime::BuildStatusRenderState() const {
        const ClientFlowState& flow = FlowState();
        return render::BuildStatusRenderState(sceneManager_.ActiveScene(), flow.statusMessage, flow.disconnectReason);
    }

    components::NetworkDebugState ClientRuntime::BuildDebugState() const {
        const ClientFlowState& flow = FlowState();
        components::NetworkDebugState state;
        state.activeScene = sceneManager_.ActiveScene();
        const std::string status = ActiveScreenStatusMessage(flow);
        state.sceneName = ComposeSceneLabel(state.activeScene, status);
        state.connecting = connecting_;
        state.connected = connected_;
        state.welcomed = serverWelcomed_;
        state.disconnectReason = flow.disconnectReason;
        state.runtimeStatusMessage = flow.statusMessage;
        state.clientTick = clientTick_;
        state.serverTick = latestServerTick_;
        state.pendingInputCount = pendingInputs_.size();
        state.loadedChunkCount = chunksByCoord_.size();
        state.chunkVersionConflicts = chunkVersionConflictCount_;
        if (transport_.IsInitialized() && serverConnection_ != net::kInvalidConnectionHandle) {
            state.metrics = transport_.GetConnectionMetrics(serverConnection_);
        }
        return state;
    }

    bool ClientRuntime::IsLocalPlayerReady() const { return serverWelcomed_ && localPlayerId.IsValid(); }

}  // namespace runtime

}  // namespace client
