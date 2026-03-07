#pragma once

#include <chrono>
#include <string>
#include <utility>

#include "client/core/runtime_state.hpp"

namespace client::runtime {

struct ClientFlowState {
    core::RuntimeState runtime{};
    std::string statusMessage;
    std::string disconnectReason;
    bool debugOverlayEnabled = true;
    std::chrono::steady_clock::time_point splashStartedAt{};
};

struct LocalServerStartupState {
    bool ownsProcess = false;
    bool startupInProgress = false;
    std::chrono::steady_clock::time_point launchStartedAt{};
    std::chrono::steady_clock::time_point lastConnectAttemptAt{};
};

struct ClientRuntimeSessionSnapshot {
    bool connecting = false;
    bool connected = false;
    bool serverWelcomed = false;
    bool singleplayerActive = false;
};

inline void RefreshClientFlowState(ClientFlowState& flow, LocalServerStartupState& localServerStartup,
                                   const ClientRuntimeSessionSnapshot& session) {
    if (!flow.disconnectReason.empty()) {
        flow.runtime.disconnectReason = flow.disconnectReason;
        if (flow.runtime.mode == core::RuntimeMode::JoiningServer) {
            flow.statusMessage = "Join failed: " + flow.disconnectReason;
            flow.disconnectReason.clear();
            flow.runtime.disconnectReason.clear();
            flow.runtime.joiningInProgress = false;
        } else {
            flow.runtime.mode = core::RuntimeMode::Disconnected;
            return;
        }
    } else {
        flow.runtime.disconnectReason.clear();
    }

    switch (flow.runtime.mode) {
        case core::RuntimeMode::Boot:
            if (flow.runtime.splashCompleted) {
                flow.runtime.mode = core::RuntimeMode::Menu;
            }
            return;
        case core::RuntimeMode::Menu:
            flow.runtime.joiningInProgress = false;
            return;
        case core::RuntimeMode::JoiningServer:
            flow.runtime.joiningInProgress = session.connecting || (session.connected && !session.serverWelcomed);
            if (session.connected && session.serverWelcomed) {
                localServerStartup.startupInProgress = false;
                flow.runtime.requestedLocalServerStart = false;
                flow.statusMessage.clear();
                flow.runtime.joiningInProgress = false;
                flow.runtime.mode = core::RuntimeMode::Multiplayer;
            }
            return;
        case core::RuntimeMode::Multiplayer:
            if (!session.connected) {
                flow.disconnectReason = "connection closed";
                flow.runtime.disconnectReason = flow.disconnectReason;
                flow.runtime.mode = core::RuntimeMode::Disconnected;
            }
            return;
        case core::RuntimeMode::StartingLocalServer:
            return;
        case core::RuntimeMode::Singleplayer:
            if (!session.singleplayerActive) {
                flow.runtime.mode = core::RuntimeMode::Menu;
            }
            return;
        case core::RuntimeMode::Options:
        case core::RuntimeMode::Disconnected:
            return;
    }
}

[[nodiscard]] inline std::string ActiveScreenStatusMessage(const ClientFlowState& flow) {
    return flow.runtime.mode == core::RuntimeMode::Disconnected ? flow.disconnectReason : flow.statusMessage;
}

[[nodiscard]] inline std::string MenuStatusMessageForReturn(const ClientFlowState& flow, std::string requestedStatus) {
    if (!requestedStatus.empty()) {
        return requestedStatus;
    }
    if (flow.runtime.mode == core::RuntimeMode::Disconnected && !flow.disconnectReason.empty()) {
        return flow.disconnectReason;
    }
    return {};
}

}  // namespace client::runtime
