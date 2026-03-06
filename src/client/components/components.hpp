#pragma once

#include <optional>
#include <string>
#include <vector>

#include "client/core/scene.hpp"
#include "shared/game/entity.hpp"
#include "shared/net/transport.hpp"

namespace client::components {

namespace game = shared::game;
namespace net = shared::net;

struct PlayerRenderState {
    game::PlayerId playerId{};
    game::EntityId entityId{};
    std::string displayName;
    game::Vec2f position{};
    bool isLocal = false;
};

struct WorldRenderState {
    PlayerRenderState localPlayer{};
    std::vector<PlayerRenderState> remotePlayers;
};

struct NetworkDebugState {
    std::string sceneName;
    core::SceneKind activeScene = core::SceneKind::Splash;
    bool connecting = false;
    bool connected = false;
    bool welcomed = false;
    std::string disconnectReason;
    std::string runtimeStatusMessage;
    game::TickId clientTick = 0;
    game::TickId serverTick = 0;
    size_t pendingInputCount = 0;
    size_t loadedChunkCount = 0;
    uint32_t chunkVersionConflicts = 0;
    std::vector<std::string> menuActions;
    size_t selectedMenuIndex = 0;
    std::string joinHost;
    std::string joinPort;
    std::string joinPlayerName;
    size_t selectedJoinFieldIndex = 0;
    bool joinEditing = false;
    std::optional<net::ConnectionMetrics> metrics;
};

}  // namespace client::components
