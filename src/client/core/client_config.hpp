#pragma once

#include <cstdint>
#include <string>

namespace client {

struct ClientConfig {
    std::string executablePath;
    std::string configFilePath;
    std::string serverHost = "127.0.0.1";
    uint16_t serverPort = 27020;
    std::string playerName = "player";
    uint32_t buildCompatibilityHash = 0;
    int windowWidth = 1600;
    int windowHeight = 900;
    int targetFps = 120;
    int simulationTickHz = 30;
    int interpolationDelayTicks = 2;
    bool debugOverlayDefault = true;
    bool autoJoin = false;
    bool skipSplash = false;
};

}  // namespace client
