#include <cassert>

#include "client/core/client_args.hpp"

int main() {
    using namespace client::core;

    const char* argvA[] = {
        "game_client",
        "--host",
        "192.168.1.10",
        "--port",
        "28000",
        "--name",
        "alice",
        "--tick-rate",
        "60",
        "--auto-join",
        "--skip-splash",
    };
    const ParsedClientArgs parsedA = ParseClientArgs(static_cast<int>(sizeof(argvA) / sizeof(argvA[0])), argvA);
    assert(parsedA.showHelp == false);
    assert(parsedA.config.serverHost == "192.168.1.10");
    assert(parsedA.config.serverPort == 28000);
    assert(parsedA.config.playerName == "alice");
    assert(parsedA.config.simulationTickHz == 60);
    assert(parsedA.config.autoJoin == true);
    assert(parsedA.config.skipSplash == true);

    const char* argvB[] = {"game_client", "--help"};
    const ParsedClientArgs parsedB = ParseClientArgs(static_cast<int>(sizeof(argvB) / sizeof(argvB[0])), argvB);
    assert(parsedB.showHelp == true);

    return 0;
}
