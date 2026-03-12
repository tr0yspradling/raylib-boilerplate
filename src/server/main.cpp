#include <exception>
#include <iostream>
#include <string>

#include "server/config/server_config_policy.hpp"
#include "server/game_server.hpp"

namespace {

struct ParsedServerArgs {
    server::ServerConfig config{};
    bool showHelp = false;
};

[[nodiscard]] ParsedServerArgs ParseArgs(int argc, char** argv) {
    std::string configPath = std::string{server::config::policy::kDefaultConfigPath};
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--config" && i + 1 < argc) {
            configPath = argv[++i];
        }
    }

    std::string warning;
    ParsedServerArgs parsed{};
    parsed.config = server::LoadServerConfigFile(configPath, warning);
    if (!warning.empty()) {
        std::cerr << "[server.config] " << warning << '\n';
    }

    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--port" && i + 1 < argc) {
            parsed.config.listenPort = static_cast<uint16_t>(std::stoi(argv[++i]));
        } else if (arg == "--tick-rate" && i + 1 < argc) {
            parsed.config.simulationTickHz = std::stoi(argv[++i]);
        } else if (arg == "--snapshot-rate" && i + 1 < argc) {
            parsed.config.snapshotRateHz = std::stoi(argv[++i]);
        } else if (arg == "--config" && i + 1 < argc) {
            ++i;
        } else if (arg == "--help") {
            parsed.showHelp = true;
        }
    }

    return parsed;
}

}  // namespace

int main(int argc, char** argv) {
    try {
        ParsedServerArgs parsed = ParseArgs(argc, argv);
        if (parsed.showHelp) {
            std::cout << server::config::policy::kUsageText << '\n';
            return 0;
        }

        server::ServerConfig config = std::move(parsed.config);
        server::GameServer server{std::move(config)};
        if (!server.Initialize()) {
            return 1;
        }
        return server.Run();
    } catch (const std::exception& exception) {
        std::cerr << "Fatal server error: " << exception.what() << '\n';
        return 1;
    }
}
