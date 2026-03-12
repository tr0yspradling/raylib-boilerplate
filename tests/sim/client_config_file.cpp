#include <cassert>
#include <filesystem>
#include <string>

#include "client/core/config.hpp"
#include "client/core/client_config_policy.hpp"

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
    assert(missing.serverHost == std::string{client::core::policy::kDefaultServerHost});
    assert(missing.serverPort == client::core::policy::kDefaultServerPort);
    assert(missing.playerName == std::string{client::core::policy::kDefaultPlayerName});
    assert(missing.windowWidth == client::core::policy::kDefaultWindowWidth);
    assert(missing.windowHeight == client::core::policy::kDefaultWindowHeight);
    assert(missing.targetFps == client::core::policy::kDefaultTargetFps);
    assert(missing.interpolationDelayTicks == client::core::policy::kDefaultInterpolationDelayTicks);
    assert(missing.debugOverlayDefault == client::core::policy::kDefaultDebugOverlayEnabled);
    assert(client::core::policy::DefaultClientConfigPath() ==
        (fs::path{std::string{client::core::policy::kConfigDirectoryName}} /
         std::string{client::core::policy::kConfigFileName}));

    fs::remove_all(tempRoot);
    return 0;
}
