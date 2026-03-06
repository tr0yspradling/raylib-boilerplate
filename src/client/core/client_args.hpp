#pragma once

#include <string>

#include "client/core/client_config.hpp"

namespace client::core {

struct ParsedClientArgs {
    client::ClientConfig config{};
    bool showHelp = false;
};

[[nodiscard]] inline ParsedClientArgs ParseClientArgs(int argc, const char* const* argv) {
    ParsedClientArgs parsed{};

    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--host" && i + 1 < argc) {
            parsed.config.serverHost = argv[++i];
        } else if (arg == "--port" && i + 1 < argc) {
            parsed.config.serverPort = static_cast<uint16_t>(std::stoi(argv[++i]));
        } else if (arg == "--name" && i + 1 < argc) {
            parsed.config.playerName = argv[++i];
        } else if (arg == "--tick-rate" && i + 1 < argc) {
            parsed.config.simulationTickHz = std::stoi(argv[++i]);
        } else if (arg == "--auto-join") {
            parsed.config.autoJoin = true;
        } else if (arg == "--skip-splash") {
            parsed.config.skipSplash = true;
        } else if (arg == "--help") {
            parsed.showHelp = true;
        }
    }

    return parsed;
}

}  // namespace client::core
