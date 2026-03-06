#pragma once

#include <cstdint>
#include <string>

namespace client::core {

struct Config {
    std::string serverHost = "127.0.0.1";
    uint16_t serverPort = 27020;
    std::string playerName = "player";

    int windowWidth = 1600;
    int windowHeight = 900;
    int targetFps = 120;

    int simulationTickHz = 30;
    int snapshotRateHz = 15;
};

}  // namespace client::core
