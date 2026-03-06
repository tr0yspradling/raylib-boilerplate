#pragma once

#include <cstdint>
#include <string>

#include "shared/game/entity.hpp"
#include "shared/game/world.hpp"
#include "shared/net/auth.hpp"

namespace server {

struct ServerConfig {
    std::string bindAddress = "0.0.0.0";
    uint16_t listenPort = 27020;
    int simulationTickHz = 30;
    int snapshotRateHz = 15;
    int maxClients = 64;
    int maxInputFramesPerSecond = 120;
    int maxChunkHintsPerSecond = 30;
    int maxChunkResyncRequestsPerSecond = 60;
    int metricsLogIntervalSeconds = 10;
    bool enforceBuildHash = false;
    uint32_t requiredBuildHash = 0;
    shared::net::AuthMode authMode = shared::net::AuthMode::DevInsecure;
    std::string persistencePath = "server_data/world_state.txt";
    shared::game::WorldConfig worldConfig{};
    shared::game::PlayerKinematicsConfig playerKinematics{};

    float fakeLagMs = 0.0f;
    float fakeJitterMs = 0.0f;
    float fakeLossSendPct = 0.0f;
    float fakeLossRecvPct = 0.0f;
};

[[nodiscard]] ServerConfig LoadServerConfigFile(const std::string& path, std::string& warning);

}  // namespace server
