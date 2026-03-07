#pragma once

#include <chrono>
#include <cstdint>
#include <deque>
#include <string>
#include <unordered_map>
#include <utility>

#include "client/core/runtime_state.hpp"
#include "shared/game/chunk.hpp"
#include "shared/game/entity.hpp"
#include "shared/game/interpolation.hpp"
#include "shared/game/world.hpp"
#include "shared/net/transport.hpp"

namespace client::runtime {

namespace game = shared::game;
namespace net = shared::net;

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

struct RemotePlayerView {
    game::PlayerId playerId{};
    std::string displayName;
    game::EntityId entityId{};
    game::Vec2f latestPosition{};
    game::PositionInterpolationBuffer interpolation;
};

struct ClientChunkState {
    game::ChunkData chunk;
};

struct ClientSessionState {
    net::ConnectionHandle serverConnection = net::kInvalidConnectionHandle;
    bool connecting = false;
    bool connected = false;
    bool serverWelcomed = false;
    game::TickId clientTick = 0;
    game::TickId latestServerTick = 0;
    float renderInterpolationTick = 0.0f;
    uint16_t serverTickRateHz = 30;
    uint16_t serverSnapshotRateHz = 15;
    game::PlayerKinematicsConfig serverKinematics{};
    game::WorldConfig serverWorldConfig{};
    bool hasWorldMetadata = false;
    game::PlayerId localPlayerId{};
    game::PlayerState predictedLocalPlayer{};
    std::deque<game::PlayerInputFrame> pendingInputs;
    uint32_t nextInputSequence = 1;
    std::unordered_map<game::PlayerId, RemotePlayerView, game::IdHash<game::PlayerIdTag>> remotePlayers;
    std::unordered_map<game::ChunkCoord, ClientChunkState, game::ChunkCoordHash> chunksByCoord;
    std::unordered_map<game::ChunkCoord, std::chrono::steady_clock::time_point, game::ChunkCoordHash>
        chunkResyncRequestedAt;
    uint32_t chunkVersionConflictCount = 0;
    std::chrono::steady_clock::time_point lastPingSentAt{};
    std::chrono::steady_clock::time_point lastChunkHintSentAt{};
    uint32_t nextPingSequence = 1;
};

inline void ResetClientSessionState(ClientSessionState& session) {
    session.serverConnection = net::kInvalidConnectionHandle;
    session.connecting = false;
    session.connected = false;
    session.serverWelcomed = false;
    session.clientTick = 0;
    session.latestServerTick = 0;
    session.renderInterpolationTick = 0.0f;
    session.serverKinematics = {};
    session.serverWorldConfig = {};
    session.hasWorldMetadata = false;
    session.localPlayerId = {};
    session.predictedLocalPlayer = {};
    session.pendingInputs.clear();
    session.nextInputSequence = 1;
    session.remotePlayers.clear();
    session.chunksByCoord.clear();
    session.chunkResyncRequestedAt.clear();
    session.chunkVersionConflictCount = 0;
}

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
