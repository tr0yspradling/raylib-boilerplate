#pragma once

#include <optional>
#include <string>
#include <vector>

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
    bool connecting = false;
    bool connected = false;
    bool welcomed = false;
    std::string disconnectReason;
    game::TickId clientTick = 0;
    game::TickId serverTick = 0;
    size_t pendingInputCount = 0;
    size_t loadedChunkCount = 0;
    uint32_t chunkVersionConflicts = 0;
    std::optional<net::ConnectionMetrics> metrics;
};

}  // namespace client::components
