#include "client/core/client_args.hpp"

#include <cassert>
#include <string>

int main() {
    using namespace client::core;

    const char* argvA[] = {
        "game_client", "--host",      "192.168.1.10", "--port",      "28000",         "--name",
        "alice",       "--tick-rate", "60",           "--auto-join", "--skip-splash",
    };
    const ParsedClientArgs parsedA = ParseClientArgs(static_cast<int>(sizeof(argvA) / sizeof(argvA[0])), argvA);
    assert(parsedA.showHelp == false);
    assert(parsedA.config.serverHost == "192.168.1.10");
    assert(parsedA.config.serverPort == 28000);
    assert(parsedA.config.playerName == "alice");
    assert(parsedA.config.simulationTickHz == 60);
    assert(parsedA.hostProvided == true);
    assert(parsedA.portProvided == true);
    assert(parsedA.nameProvided == true);
    assert(parsedA.tickRateProvided == true);
    assert(parsedA.config.autoJoin == true);
    assert(parsedA.config.skipSplash == true);

    const char* argvB[] = {"game_client", "--help"};
    const ParsedClientArgs parsedB = ParseClientArgs(static_cast<int>(sizeof(argvB) / sizeof(argvB[0])), argvB);
    assert(parsedB.showHelp == true);

    client::ClientConfig mergedConfig{};
    mergedConfig.serverHost = "10.0.0.2";
    mergedConfig.serverPort = 27020;
    mergedConfig.playerName = "persisted";
    mergedConfig.simulationTickHz = 30;
    parsedA.ApplyOverrides(mergedConfig);
    assert(mergedConfig.serverHost == "192.168.1.10");
    assert(mergedConfig.serverPort == 28000);
    assert(mergedConfig.playerName == "alice");
    assert(mergedConfig.simulationTickHz == 60);
    assert(mergedConfig.autoJoin == true);
    assert(mergedConfig.skipSplash == true);

    const std::string helpText = ClientArgHelpText("game_client");
    assert(helpText.find("--auto-join") != std::string::npos);
    assert(helpText.find("--skip-splash") != std::string::npos);

    return 0;
}
