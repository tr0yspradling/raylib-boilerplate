#include "client/client_entry.hpp"

#include <exception>
#include <iostream>
#include <string>
#include <utility>

#include "client/game_client.hpp"

namespace client {

namespace {

struct ParsedClientArgs {
    ClientConfig config{};
    bool showHelp = false;
};

[[nodiscard]] ParsedClientArgs ParseArgs(int argc, char** argv) {
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
        } else if (arg == "--help") {
            parsed.showHelp = true;
        }
    }

    return parsed;
}

}  // namespace

int RunClientEntry(int argc, char** argv) {
    try {
        ParsedClientArgs parsed = ParseArgs(argc, argv);
        if (parsed.showHelp) {
            std::cout << "Usage: [--host HOST] [--port PORT] [--name PLAYER] [--tick-rate HZ]\n";
            return 0;
        }

        ClientConfig config = std::move(parsed.config);
        GameClient client{std::move(config)};
        if (!client.Initialize()) {
            return 1;
        }

        return client.Run();
    } catch (const std::exception& exception) {
        std::cerr << "Fatal client error: " << exception.what() << '\n';
        return 1;
    }
}

}  // namespace client
