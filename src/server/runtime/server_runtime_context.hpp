#pragma once

#include <chrono>
#include <cstddef>
#include <memory>
#include <string>

#include "server/config/server_config.hpp"
#include "server/runtime/server_runtime_state.hpp"
#include "shared/game/fixed_step.hpp"
#include "shared/net/auth.hpp"
#include "shared/net/transport_gns.hpp"

namespace server::runtime {

struct ServerRuntimeContext {
    ServerConfig& config;
    shared::net::TransportGns& transport;
    shared::game::GameState& gameState;
    shared::game::FixedStep& fixedStep;
    std::unique_ptr<shared::net::IAuthProvider>& authProvider;
    SessionsByConnectionMap& sessionsByConnection;
    ConnectionByPlayerMap& connectionByPlayer;
    ChunksByCoordMap& chunksByCoord;
    shared::game::PlayerId& nextPlayerId;
    std::chrono::steady_clock::time_point& lastMetricsLogAt;
    size_t& pendingReplicationSteps;
};

}  // namespace server::runtime
