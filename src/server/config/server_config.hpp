#pragma once

#include <cstdint>
#include <string>

#include "server/config/server_config_policy.hpp"
#include "shared/game/entity.hpp"
#include "shared/game/world.hpp"
#include "shared/net/auth.hpp"

namespace server {

struct ServerConfig {
    std::string bindAddress = std::string{config::policy::kDefaultBindAddress};
    uint16_t listenPort = config::policy::kDefaultListenPort;
    int simulationTickHz = config::policy::kDefaultSimulationTickHz;
    int snapshotRateHz = config::policy::kDefaultSnapshotRateHz;
    int maxClients = config::policy::kDefaultMaxClients;
    int maxInputFramesPerSecond = config::policy::kDefaultMaxInputFramesPerSecond;
    int maxChunkHintsPerSecond = config::policy::kDefaultMaxChunkHintsPerSecond;
    int maxChunkResyncRequestsPerSecond = config::policy::kDefaultMaxChunkResyncRequestsPerSecond;
    int metricsLogIntervalSeconds = config::policy::kDefaultMetricsLogIntervalSeconds;
    bool enforceBuildHash = false;
    uint32_t requiredBuildHash = 0;
    shared::net::AuthMode authMode = shared::net::AuthMode::DevInsecure;
    std::string persistencePath = std::string{config::policy::kDefaultPersistencePath};
    shared::game::WorldConfig worldConfig{};
    shared::game::PlayerKinematicsConfig playerKinematics{};

    float fakeLagMs = 0.0f;
    float fakeJitterMs = 0.0f;
    float fakeLossSendPct = 0.0f;
    float fakeLossRecvPct = 0.0f;
};

[[nodiscard]] ServerConfig LoadServerConfigFile(const std::string& path, std::string& warning);

}  // namespace server
