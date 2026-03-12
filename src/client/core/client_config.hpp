#pragma once

#include <cstdint>
#include <string>

#include "client/core/client_config_policy.hpp"

namespace client {

struct ClientConfig {
    std::string executablePath;
    std::string configFilePath;
    std::string serverHost = std::string{core::policy::kDefaultServerHost};
    uint16_t serverPort = core::policy::kDefaultServerPort;
    std::string playerName = std::string{core::policy::kDefaultPlayerName};
    uint32_t buildCompatibilityHash = 0;
    int windowWidth = core::policy::kDefaultWindowWidth;
    int windowHeight = core::policy::kDefaultWindowHeight;
    int targetFps = core::policy::kDefaultTargetFps;
    int simulationTickHz = core::policy::kDefaultSimulationTickHz;
    int interpolationDelayTicks = core::policy::kDefaultInterpolationDelayTicks;
    bool debugOverlayDefault = core::policy::kDefaultDebugOverlayEnabled;
    bool autoJoin = false;
    bool skipSplash = false;
};

}  // namespace client
