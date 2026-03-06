#include "client/client_entry.hpp"

#include <exception>
#include <iostream>
#include <utility>

#include "client/core/client_args.hpp"
#include "client/game_client.hpp"

namespace client {

int RunClientEntry(int argc, char** argv) {
    try {
        const core::ParsedClientArgs parsed = core::ParseClientArgs(argc, argv);
        if (parsed.showHelp) {
            std::cout
                << "Usage: [--host HOST] [--port PORT] [--name PLAYER] [--tick-rate HZ] [--auto-join] [--skip-splash]\n";
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
