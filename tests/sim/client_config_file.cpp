#include <cassert>
#include <filesystem>
#include <string>

#include "client/core/config.hpp"

int main() {
    namespace fs = std::filesystem;

    const fs::path tempRoot = fs::temp_directory_path() / "raylib_boilerplate_client_config_test";
    const fs::path configPath = tempRoot / "client.cfg";
    fs::remove_all(tempRoot);

    client::ClientConfig written{};
    written.serverHost = "192.168.0.44";
    written.serverPort = 28001;
    written.playerName = "tester";
    written.windowWidth = 1280;
    written.windowHeight = 720;
    written.targetFps = 144;
    written.interpolationDelayTicks = 3;
    written.debugOverlayDefault = false;

    std::string error;
    assert(client::core::SaveClientConfigFile(written, configPath, error));
    assert(error.empty());

    std::string warning;
    const client::ClientConfig loaded = client::core::LoadClientConfigFile(configPath, warning);
    assert(warning.empty());
    assert(loaded.serverHost == "192.168.0.44");
    assert(loaded.serverPort == 28001);
    assert(loaded.playerName == "tester");
    assert(loaded.windowWidth == 1280);
    assert(loaded.windowHeight == 720);
    assert(loaded.targetFps == 144);
    assert(loaded.interpolationDelayTicks == 3);
    assert(loaded.debugOverlayDefault == false);

    const client::ClientConfig missing =
        client::core::LoadClientConfigFile(tempRoot / "missing.cfg", warning);
    assert(warning.empty());
    assert(missing.serverHost == "127.0.0.1");

    fs::remove_all(tempRoot);
    return 0;
}
