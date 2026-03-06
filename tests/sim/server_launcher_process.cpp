#include <cassert>
#include <filesystem>
#include <fstream>
#include <string>

#include "client/core/server_launcher.hpp"

namespace {

std::filesystem::path ServerBinaryPathFor(const std::filesystem::path& clientPath) {
#if defined(_WIN32)
    return clientPath.parent_path() / "game_server.exe";
#else
    return clientPath.parent_path() / "game_server";
#endif
}

}  // namespace

int main() {
    using namespace client::core;

    const std::filesystem::path tempRoot =
        std::filesystem::temp_directory_path() / "raylib_boilerplate_server_launcher_test";
    std::filesystem::remove_all(tempRoot);
    std::filesystem::create_directories(tempRoot / "build" / "debug");

#if defined(_WIN32)
    const std::filesystem::path clientPath = tempRoot / "build" / "debug" / "game_client.exe";
#else
    const std::filesystem::path clientPath = tempRoot / "build" / "debug" / "game_client";
#endif
    const std::filesystem::path serverPath = ServerBinaryPathFor(clientPath);

    {
        std::ofstream serverBinary{serverPath};
        serverBinary << "placeholder";
    }

    const std::filesystem::path resolvedPath = ResolveSiblingServerExecutable(clientPath);
    assert(resolvedPath == serverPath);

    ServerLaunchCommand command;
    std::string error;
    const bool built = BuildLocalServerLaunchCommand(
        {.clientExecutablePath = clientPath.string(), .serverPort = 27021, .simulationTickHz = 45, .snapshotRateHz = 20},
        command, error);
    assert(built);
    assert(error.empty());
    assert(command.executablePath == serverPath);
    assert(command.arguments.size() == 6);
    assert(command.arguments[0] == "--port");
    assert(command.arguments[1] == "27021");
    assert(command.arguments[2] == "--tick-rate");
    assert(command.arguments[3] == "45");
    assert(command.arguments[4] == "--snapshot-rate");
    assert(command.arguments[5] == "20");

    std::filesystem::remove(serverPath);
    const bool builtMissingBinary =
        BuildLocalServerLaunchCommand({.clientExecutablePath = clientPath.string()}, command, error);
    assert(!builtMissingBinary);
    assert(error.find("game_server executable not found") != std::string::npos);

    std::filesystem::remove_all(tempRoot);
    return 0;
}
