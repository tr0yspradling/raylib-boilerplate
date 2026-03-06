#include "client/client_entry.hpp"

#include <exception>
#include <filesystem>
#include <iostream>
#include <string>
#include <utility>

#include "client/core/client_args.hpp"
#include "client/game_client.hpp"

namespace client {

int RunClientEntry(int argc, char** argv) {
    try {
        const std::string programName = argc > 0 && argv != nullptr && argv[0] != nullptr ? argv[0] : "game_client";

        core::ParsedClientArgs parsed{};
        try {
            parsed = core::ParseClientArgs(argc, argv);
        } catch (const std::exception& exception) {
            std::cerr << exception.what() << '\n';
            std::cerr << core::ClientArgHelpText(programName);
            return 1;
        }

        if (parsed.showHelp) {
            std::cout << core::ClientArgHelpText(programName);
            return 0;
        }

        ClientConfig config = std::move(parsed.config);
        const std::filesystem::path executablePath =
            std::filesystem::absolute(std::filesystem::path{programName}).lexically_normal();
        config.executablePath = executablePath.string();
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
