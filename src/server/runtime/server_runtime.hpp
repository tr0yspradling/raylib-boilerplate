#pragma once

#include <chrono>
#include <cstdint>
#include <memory>
#include <string>

#include <flecs.h>

#include "server/config/server_config.hpp"
#include "server/runtime/server_runtime_context.hpp"
#include "server/runtime/server_runtime_state.hpp"
#include "shared/game/game_state.hpp"
#include "shared/game/ids.hpp"
#include "shared/net/auth.hpp"
#include "shared/game/fixed_step.hpp"
#include "shared/net/protocol.hpp"
#include "shared/net/transport_gns.hpp"

namespace server {

namespace runtime {

class ServerRuntime {
public:
    explicit ServerRuntime(ServerConfig config);

    [[nodiscard]] bool Initialize();
    void Start();
    void Shutdown();
    void RequestStop();
    void PollTransportPump();
    void DecodeTransportMessages();
    void RunAuthAndSessionPhase();
    void RunInputApplyPhase();
    void AdvanceSimulation(float frameSeconds);
    void RunReplicationPhase();
    void RunPersistencePhase();
    void RunMetricsPhase();

    [[nodiscard]] bool IsRunning() const;

private:
    bool LoadPersistence();
    void SavePersistence();
    [[nodiscard]] ServerRuntimeContext MakeContext();

    ServerConfig config_;
    shared::net::TransportGns transport_;
    shared::game::GameState gameState_;
    shared::game::FixedStep fixedStep_;

    std::unique_ptr<shared::net::IAuthProvider> authProvider_;

    SessionsByConnectionMap sessionsByConnection_;
    ConnectionByPlayerMap connectionByPlayer_;
    ChunksByCoordMap chunksByCoord_;

    shared::game::PlayerId nextPlayerId_{1};

    bool initialized_ = false;
    bool running_ = false;
    std::string persistenceWarning_;
    std::chrono::steady_clock::time_point lastMetricsLogAt_{};
    size_t pendingReplicationSteps_ = 0U;
};

}  // namespace runtime

}  // namespace server
