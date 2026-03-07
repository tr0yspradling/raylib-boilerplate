#pragma once

#include <argparse/argparse.hpp>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include "client/core/client_config.hpp"

namespace client::core {

struct ParsedClientArgs {
    client::ClientConfig config{};
    bool showHelp = false;
    bool hostProvided = false;
    bool portProvided = false;
    bool nameProvided = false;
    bool tickRateProvided = false;

    void ApplyOverrides(client::ClientConfig& target) const {
        if (hostProvided) {
            target.serverHost = config.serverHost;
        }
        if (portProvided) {
            target.serverPort = config.serverPort;
        }
        if (nameProvided) {
            target.playerName = config.playerName;
        }
        if (tickRateProvided) {
            target.simulationTickHz = config.simulationTickHz;
        }
        target.autoJoin = config.autoJoin;
        target.skipSplash = config.skipSplash;
    }
};

inline void ConfigureClientArgParser(argparse::ArgumentParser& parser) {
    parser.add_description("Game client runtime options.");

    parser.add_argument("--host").help("Server host or IP address.");
    parser.add_argument("--port").help("Server port.").scan<'i', int>();
    parser.add_argument("--name").help("Player display name.");
    parser.add_argument("--tick-rate").help("Client simulation tick rate in Hz.").scan<'i', int>();
    parser.add_argument("--auto-join").help("Connect immediately on startup.").flag();
    parser.add_argument("--skip-splash").help("Start at the main menu without the splash screen.").flag();
    parser.add_argument("-h", "--help").help("Show this help message and exit.").flag();
}

[[nodiscard]] inline std::string ClientArgHelpText(std::string_view programName = "game_client") {
    argparse::ArgumentParser parser(std::string{programName}, "", argparse::default_arguments::none);
    ConfigureClientArgParser(parser);

    std::ostringstream output;
    output << parser;
    return output.str();
}

[[nodiscard]] inline ParsedClientArgs ParseClientArgs(int argc, const char* const* argv) {
    const std::string programName = argc > 0 && argv != nullptr && argv[0] != nullptr ? argv[0] : "game_client";
    argparse::ArgumentParser parser(programName, "", argparse::default_arguments::none);
    ConfigureClientArgParser(parser);

    std::vector<std::string> arguments;
    arguments.reserve(static_cast<size_t>(argc > 0 ? argc : 1));
    for (int i = 0; i < argc; ++i) {
        arguments.emplace_back(argv != nullptr && argv[i] != nullptr ? argv[i] : "");
    }
    if (arguments.empty()) {
        arguments.push_back(programName);
    }
    parser.parse_args(arguments);

    ParsedClientArgs parsed{};
    if (const auto host = parser.present<std::string>("--host")) {
        parsed.config.serverHost = *host;
        parsed.hostProvided = true;
    }
    if (const auto port = parser.present<int>("--port")) {
        parsed.config.serverPort = static_cast<uint16_t>(*port);
        parsed.portProvided = true;
    }
    if (const auto name = parser.present<std::string>("--name")) {
        parsed.config.playerName = *name;
        parsed.nameProvided = true;
    }
    if (const auto tickRate = parser.present<int>("--tick-rate")) {
        parsed.config.simulationTickHz = *tickRate;
        parsed.tickRateProvided = true;
    }
    parsed.config.autoJoin = parser.get<bool>("--auto-join");
    parsed.config.skipSplash = parser.get<bool>("--skip-splash");
    parsed.showHelp = parser.get<bool>("--help");

    return parsed;
}

}  // namespace client::core
