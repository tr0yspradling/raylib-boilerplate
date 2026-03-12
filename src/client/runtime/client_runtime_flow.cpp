#include "client/runtime/client_runtime_flow.hpp"

#include <algorithm>
#include <charconv>
#include <chrono>
#include <cstdio>
#include <limits>
#include <string>
#include <utility>

#include "client/runtime/client_runtime_policy.hpp"
#include "client/ui/ui_document.hpp"
#include "shared/net/net_policy.hpp"

namespace client::runtime {

namespace {

void StopOwnedLocalServer(ClientRuntimeContext& context) {
    LocalServerStartupState& localServer = context.LocalServerState();
    if (!localServer.ownsProcess || !context.serverLauncher) {
        return;
    }

    context.serverLauncher->Stop();
    localServer.ownsProcess = false;
}

void ResetSessionState(ClientRuntimeContext& context) {
    context.FlowState().runtime.joiningInProgress = false;
    ClientSessionState& session = context.SessionState();
    context.singleplayerSession.Stop(session);
    ResetClientSessionState(session);
}

[[nodiscard]] bool ApplyJoinFormToConfig(ClientRuntimeContext& context, const ui::JoinServerScreenState& joinScreenState) {
    ClientFlowState& flow = context.FlowState();
    if (joinScreenState.host.empty()) {
        flow.statusMessage = std::string{policy::kHostRequiredMessage};
        return false;
    }
    if (joinScreenState.playerName.empty()) {
        flow.statusMessage = std::string{policy::kPlayerNameRequiredMessage};
        return false;
    }
    if (joinScreenState.port.empty()) {
        flow.statusMessage = std::string{policy::kPortRequiredMessage};
        return false;
    }

    uint32_t parsedPort = 0;
    const char* begin = joinScreenState.port.data();
    const char* end = begin + joinScreenState.port.size();
    const auto [ptr, error] = std::from_chars(begin, end, parsedPort);
    if (error != std::errc{} || ptr != end || parsedPort == 0 || parsedPort > 65535U) {
        flow.statusMessage = std::string{policy::kPortRangeMessage};
        return false;
    }

    context.config.serverHost = joinScreenState.host;
    context.config.serverPort = static_cast<uint16_t>(parsedPort);
    context.config.playerName = joinScreenState.playerName;
    return true;
}

[[nodiscard]] bool BeginJoinServer(ClientRuntimeContext& context) {
    ClientSessionState& session = context.SessionState();
    if (session.connecting || session.connected) {
        return true;
    }

    ClientFlowState& flow = context.FlowState();
    flow.runtime.mode = core::RuntimeMode::JoiningServer;
    flow.runtime.joiningInProgress = false;
    context.world.get_mut<ui::JoinServerScreenState>().editing = false;
    context.world.get_mut<ui::OptionsScreenState>().editing = false;
    flow.statusMessage.clear();
    flow.disconnectReason.clear();
    flow.runtime.disconnectReason.clear();

    std::string error;
    if (!context.multiplayerSession.EnsureTransportInitialized(error)) {
        flow.statusMessage = std::string{policy::kTransportInitFailedPrefix} + error;
        std::fprintf(stderr, "[net.transport] client transport init failed: %s\n", error.c_str());
        return false;
    }

    if (!context.multiplayerSession.BeginConnectionAttempt(session, error)) {
        flow.statusMessage = std::string{policy::kConnectFailedPrefix} + error;
        std::fprintf(stderr, "[net.transport] connect failed: %s\n", error.c_str());
        return false;
    }

    session.connecting = true;
    flow.runtime.joiningInProgress = true;
    flow.statusMessage = std::string{policy::kJoinConnectingStatus};
    return true;
}

void BeginSingleplayer(ClientRuntimeContext& context) {
    ClientFlowState& flow = context.FlowState();
    ClientSessionState& session = context.SessionState();
    ResetSessionState(context);
    flow.disconnectReason.clear();
    flow.runtime.disconnectReason.clear();
    flow.runtime.requestedJoin = false;
    flow.runtime.requestedLocalServerStart = false;
    flow.runtime.joiningInProgress = false;

    context.singleplayerSession.Start(context.config.playerName, session);
    flow.runtime.mode = core::RuntimeMode::Singleplayer;
    flow.statusMessage.clear();
    context.world.get_mut<ui::JoinServerScreenState>().editing = false;
    context.world.get_mut<ui::OptionsScreenState>().editing = false;
    context.world.get_mut<ui::UiInteractionState>().focusedWidget.reset();
}

void ReturnToMenuInternal(ClientRuntimeContext& context, std::string statusMessage) {
    ClientFlowState& flow = context.FlowState();
    ResetSessionState(context);
    flow.statusMessage = MenuStatusMessageForReturn(flow, std::move(statusMessage));
    flow.disconnectReason.clear();
    flow.runtime.disconnectReason.clear();
    flow.runtime.requestedJoin = false;
    flow.runtime.requestedLocalServerStart = false;
    flow.runtime.joiningInProgress = false;
    context.world.get_mut<ui::JoinServerScreenState>().editing = false;
    context.world.get_mut<ui::OptionsScreenState>().editing = false;
    flow.runtime.mode = core::RuntimeMode::Menu;
    context.world.get_mut<ui::UiInteractionState>().focusedWidget = ui::UiWidgetId::MenuStartServer;
}

void FailLocalServerStartup(ClientRuntimeContext& context, const std::string& message) {
    LocalServerStartupState& localServer = context.LocalServerState();
    ClientFlowState& flow = context.FlowState();
    ResetSessionState(context);
    localServer.startupInProgress = false;
    flow.runtime.requestedLocalServerStart = false;
    StopOwnedLocalServer(context);
    ReturnToMenuInternal(context, message);
}

void ActivateMenuAction(ClientRuntimeContext& context, core::MenuAction action) {
    ClientFlowState& flow = context.FlowState();
    switch (action) {
    case core::MenuAction::JoinServer:
        flow.runtime.requestedJoin = false;
        flow.runtime.mode = core::RuntimeMode::JoiningServer;
        flow.runtime.joiningInProgress = false;
        context.world.get_mut<ui::JoinServerScreenState>().editing = false;
        context.world.get_mut<ui::OptionsScreenState>().editing = false;
        context.world.get_mut<ui::UiInteractionState>().focusedWidget = ui::UiWidgetId::JoinHost;
        flow.statusMessage = std::string{policy::kJoinFieldSelectionStatus};
        flow.disconnectReason.clear();
        flow.runtime.disconnectReason.clear();
        return;
    case core::MenuAction::Quit:
        context.exitRequested = true;
        return;
    case core::MenuAction::StartServer:
        flow.runtime.requestedJoin = false;
        flow.runtime.requestedLocalServerStart = true;
        flow.runtime.joiningInProgress = false;
        flow.runtime.mode = core::RuntimeMode::StartingLocalServer;
        flow.disconnectReason.clear();
        flow.runtime.disconnectReason.clear();
        flow.statusMessage = std::string{policy::kLocalServerLaunchStatus};
        context.world.get_mut<ui::JoinServerScreenState>().editing = false;
        context.world.get_mut<ui::OptionsScreenState>().editing = false;
        context.world.get_mut<ui::UiInteractionState>().focusedWidget.reset();
        return;
    case core::MenuAction::Singleplayer:
        BeginSingleplayer(context);
        return;
    case core::MenuAction::Options:
        context.world.get_mut<ui::OptionsScreenState>().ResetFromConfig(context.config);
        flow.runtime.mode = core::RuntimeMode::Options;
        flow.statusMessage = std::string{policy::kOptionsFieldSelectionStatus};
        context.world.get_mut<ui::JoinServerScreenState>().editing = false;
        context.world.get_mut<ui::UiInteractionState>().focusedWidget = ui::UiWidgetId::OptionsPlayerName;
        return;
    case core::MenuAction::None:
        return;
    }
}

void ConsumeUiCommands(ClientRuntimeContext& context) {
    ui::UiCommandQueue& commandQueue = context.world.get_mut<ui::UiCommandQueue>();
    ui::JoinServerScreenState& joinScreenState = context.world.get_mut<ui::JoinServerScreenState>();
    ui::OptionsScreenState& optionsScreenState = context.world.get_mut<ui::OptionsScreenState>();
    ui::UiInteractionState& interactionState = context.world.get_mut<ui::UiInteractionState>();
    ClientFlowState& flow = context.FlowState();

    for (const ui::UiCommand& command : commandQueue.commands) {
        switch (command.type) {
        case ui::UiCommandType::ActivateMenuAction:
            ActivateMenuAction(context, command.menuAction);
            break;
        case ui::UiCommandType::StartJoinFieldEdit:
            joinScreenState.editing = true;
            flow.statusMessage = std::string{policy::kFieldEditingStatus};
            interactionState.focusedWidget = ui::UiWidgetIdForJoinField(command.joinField);
            break;
        case ui::UiCommandType::StopJoinFieldEdit:
            joinScreenState.editing = false;
            flow.statusMessage.clear();
            break;
        case ui::UiCommandType::SubmitJoin:
            if (!ApplyJoinFormToConfig(context, joinScreenState)) {
                break;
            }
            flow.statusMessage.clear();
            (void)BeginJoinServer(context);
            break;
        case ui::UiCommandType::StartOptionsFieldEdit:
            optionsScreenState.editing = true;
            flow.statusMessage = std::string{policy::kFieldEditingStatus};
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
            {
                OptionsApplyResult result = context.optionsService.Apply(
                    optionsScreenState, context.config, joinScreenState, flow.debugOverlayEnabled,
                    context.window.has_value()
                        ? ApplyWindowSettingsFn{[](int width, int height, int targetFps) {
                              ::SetWindowSize(width, height);
                              ::SetTargetFPS(targetFps);
                          }}
                        : ApplyWindowSettingsFn{});
                flow.statusMessage = result.statusMessage;
                if (!result.success) {
                    break;
                }

                const size_t selectedIndex = optionsScreenState.SelectedIndex();
                optionsScreenState.ResetFromConfig(context.config);
                optionsScreenState.SetSelectedIndex(selectedIndex);
                interactionState.focusedWidget =
                    ui::UiWidgetIdForOptionsField(optionsScreenState.SelectedField());
            }
            break;
        case ui::UiCommandType::BackToMenu:
            ReturnToMenuInternal(context, {});
            break;
        case ui::UiCommandType::None:
            break;
        }
    }

    commandQueue.Clear();
}

void HandlePlaceholderScreenInput(ClientRuntimeContext& context) {
    ClientFlowState& flow = context.FlowState();
    LocalServerStartupState& localServer = context.LocalServerState();
    ClientSessionState& session = context.SessionState();
    switch (flow.runtime.mode) {
    case core::RuntimeMode::Boot:
    case core::RuntimeMode::Multiplayer:
    case core::RuntimeMode::Menu:
        return;
    case core::RuntimeMode::JoiningServer:
        if (flow.runtime.joiningInProgress && context.inputManager.MenuBackPressed()) {
            context.multiplayerSession.CloseConnection(session, net::policy::ToInt(net::policy::DisconnectCode::ClientShutdown),
                                                       std::string{policy::kJoinCanceledStatus}, false);
            ResetSessionState(context);
            if (localServer.startupInProgress) {
                localServer.startupInProgress = false;
                flow.runtime.requestedLocalServerStart = false;
                StopOwnedLocalServer(context);
                ReturnToMenuInternal(context, std::string{policy::kLocalServerCanceledStatus});
            } else {
                ReturnToMenuInternal(context, {});
            }
        }
        return;
    case core::RuntimeMode::StartingLocalServer:
        if (context.inputManager.MenuBackPressed()) {
            localServer.startupInProgress = false;
            flow.runtime.requestedLocalServerStart = false;
            StopOwnedLocalServer(context);
            ReturnToMenuInternal(context, std::string{policy::kLocalServerCanceledStatus});
        }
        return;
    case core::RuntimeMode::Singleplayer:
        if (context.inputManager.MenuBackPressed()) {
            ReturnToMenuInternal(context, {});
        }
        return;
    case core::RuntimeMode::Options:
        return;
    case core::RuntimeMode::Disconnected:
        if (context.inputManager.MenuSelectPressed() || context.inputManager.MenuBackPressed()) {
            ReturnToMenuInternal(context, {});
        }
        return;
    }
}

[[nodiscard]] std::string ComposeLocalServerExitMessage(const core::ServerProcessStatus& status) {
    std::string message = std::string{policy::kLocalServerExitedStatus};
    if (status.exitCode.has_value()) {
        message += " (exit " + std::to_string(*status.exitCode) + ")";
    }
    return message;
}

void UpdateLocalServerStartup(ClientRuntimeContext& context, std::chrono::steady_clock::time_point now) {
    ClientFlowState& flow = context.FlowState();
    LocalServerStartupState& localServer = context.LocalServerState();
    ClientSessionState& session = context.SessionState();
    if (flow.runtime.mode != core::RuntimeMode::StartingLocalServer && !localServer.startupInProgress) {
        return;
    }

    if (localServer.ownsProcess && context.serverLauncher) {
        const core::ServerProcessStatus status = context.serverLauncher->PollStatus();
        if (status.state == core::ServerProcessState::Exited) {
            FailLocalServerStartup(context, ComposeLocalServerExitMessage(status));
            return;
        }
    }

    if (flow.runtime.requestedLocalServerStart) {
        if (!localServer.ownsProcess) {
            std::string launchError;
            if (!context.serverLauncher ||
                !context.serverLauncher->Launch({.clientExecutablePath = context.config.executablePath,
                                                .serverPort = context.config.serverPort,
                                                .simulationTickHz = static_cast<int>(session.serverTickRateHz),
                                                .snapshotRateHz = static_cast<int>(session.serverSnapshotRateHz)},
                                               launchError)) {
                FailLocalServerStartup(context, std::string{policy::kFailedToLaunchServerPrefix} + launchError);
                return;
            }
            localServer.ownsProcess = true;
        }

        flow.runtime.requestedLocalServerStart = false;
        localServer.startupInProgress = true;
        localServer.launchStartedAt = now;
        localServer.lastConnectAttemptAt = now - policy::kLocalServerConnectRetryInterval;
        flow.statusMessage = std::string{policy::kLocalServerLaunchStatus};
    }

    if (!localServer.startupInProgress) {
        return;
    }

    if (std::chrono::duration_cast<std::chrono::milliseconds>(now - localServer.launchStartedAt) >=
        policy::kLocalServerStartupTimeout) {
        FailLocalServerStartup(context, std::string{policy::kLocalServerTimeoutStatus});
        return;
    }

    if (session.connecting || session.connected) {
        return;
    }

    if (std::chrono::duration_cast<std::chrono::milliseconds>(now - localServer.lastConnectAttemptAt) <
        policy::kLocalServerConnectRetryInterval) {
        return;
    }

    localServer.lastConnectAttemptAt = now;

    std::string error;
    if (!context.multiplayerSession.EnsureTransportInitialized(error)) {
        FailLocalServerStartup(context, std::string{policy::kTransportInitFailedPrefix} + error);
        return;
    }

    if (!context.multiplayerSession.BeginConnectionAttempt(session, error)) {
        flow.runtime.mode = core::RuntimeMode::StartingLocalServer;
        flow.runtime.joiningInProgress = false;
        flow.statusMessage = std::string{policy::kLocalServerWaitingStatus};
        return;
    }

    flow.runtime.mode = core::RuntimeMode::JoiningServer;
    flow.runtime.joiningInProgress = true;
    session.connecting = true;
    flow.statusMessage = std::string{policy::kLocalServerConnectingStatus};
}

}  // namespace

void ClientRuntimeFlowController::InitializeWorldState(ClientRuntimeContext& context) {
    ClientFlowState& flow = context.world.get_mut<ClientFlowState>();
    flow.runtime.mode = core::RuntimeMode::Boot;
    flow.runtime.splashCompleted = context.config.skipSplash;
    flow.runtime.requestedJoin = context.config.autoJoin;
    flow.runtime.requestedLocalServerStart = false;
    flow.runtime.joiningInProgress = false;
    flow.runtime.disconnectReason.clear();
    flow.statusMessage.clear();
    flow.disconnectReason.clear();
    flow.debugOverlayEnabled = context.config.debugOverlayDefault;
    flow.splashStartedAt = std::chrono::steady_clock::now();

    context.world.set<LocalServerStartupState>({});
    ClientSessionState sessionState;
    sessionState.serverTickRateHz = static_cast<uint16_t>(
        std::clamp(context.config.simulationTickHz, 1, static_cast<int>(std::numeric_limits<uint16_t>::max())));
    context.world.set<ClientSessionState>(std::move(sessionState));

    ui::JoinServerScreenState& joinScreenState = context.world.get_mut<ui::JoinServerScreenState>();
    joinScreenState.ResetFromDefaults(context.config.serverHost, context.config.serverPort, context.config.playerName);
    context.world.set<ui::MenuScreenState>({});
    context.world.set<ui::JoinServerScreenState>(joinScreenState);
    ui::OptionsScreenState optionsScreenState;
    optionsScreenState.ResetFromConfig(context.config);
    context.world.set<ui::OptionsScreenState>(optionsScreenState);
    context.world.set<ui::UiInteractionState>({});
    context.world.set<ui::UiCommandQueue>({});
    context.world.set<ui::UiDocument>({});
    context.world.set<ui::UiInputState>({});
    PublishScreenState(context);
}

void ClientRuntimeFlowController::ProcessRuntimeIntent(ClientRuntimeContext& context) {
    const auto now = std::chrono::steady_clock::now();
    ClientFlowState& flow = context.world.get_mut<ClientFlowState>();
    if (!flow.runtime.splashCompleted) {
        if (context.inputManager.MenuSelectPressed() || context.inputManager.MenuBackPressed() ||
            context.inputManager.MousePrimaryPressed() ||
            std::chrono::duration_cast<std::chrono::milliseconds>(now - flow.splashStartedAt) >= policy::kSplashDuration) {
            flow.runtime.splashCompleted = true;
        }
    }

    RefreshRuntimeState(context);
    ConsumeUiCommands(context);
    HandlePlaceholderScreenInput(context);
    UpdateLocalServerStartup(context, now);
    if (flow.runtime.mode == core::RuntimeMode::Menu && flow.runtime.requestedJoin) {
        flow.runtime.requestedJoin = false;
        (void)BeginJoinServer(context);
    }
    PublishScreenState(context);
}

void ClientRuntimeFlowController::RefreshRuntimeState(ClientRuntimeContext& context) {
    const ClientSessionState& session = context.SessionState();
    RefreshClientFlowState(context.FlowState(), context.LocalServerState(),
                           {
                               .connecting = session.connecting,
                               .connected = session.connected,
                               .serverWelcomed = session.serverWelcomed,
                               .singleplayerActive = context.singleplayerSession.IsActive(),
                           });
}

void ClientRuntimeFlowController::PublishScreenState(ClientRuntimeContext& context) {
    RefreshRuntimeState(context);
    const ClientFlowState& flow = context.world.get<ClientFlowState>();
    context.world.set<ui::ScreenState>({
        .mode = flow.runtime.mode,
        .activeScene = context.ActiveScene(),
        .joiningInProgress = flow.runtime.joiningInProgress,
        .statusMessage = ActiveScreenStatusMessage(flow),
        .disconnectReason = flow.disconnectReason,
    });
}

void ClientRuntimeFlowController::ReturnToMenu(ClientRuntimeContext& context, std::string statusMessage) {
    ReturnToMenuInternal(context, std::move(statusMessage));
}

}  // namespace client::runtime
