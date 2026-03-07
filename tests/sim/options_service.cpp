#include <cassert>
#include <filesystem>
#include <string>

#include "client/core/config.hpp"
#include "client/runtime/options_service.hpp"

int main() {
    namespace fs = std::filesystem;

    const fs::path tempRoot = fs::temp_directory_path() / "raylib_boilerplate_options_service_test";
    const fs::path configPath = tempRoot / "client.cfg";
    fs::remove_all(tempRoot);

    client::runtime::OptionsService service;
    client::ClientConfig config;
    config.configFilePath = configPath.string();
    config.playerName = "old-player";
    config.serverHost = "127.0.0.1";
    config.serverPort = 27020;

    client::ui::JoinServerScreenState joinScreenState;
    joinScreenState.ResetFromDefaults(config.serverHost, config.serverPort, config.playerName);

    client::ui::OptionsScreenState optionsScreenState;
    optionsScreenState.playerName = "alice";
    optionsScreenState.host = "192.168.0.44";
    optionsScreenState.port = "28001";
    optionsScreenState.windowWidth = "1280";
    optionsScreenState.windowHeight = "720";
    optionsScreenState.targetFps = "144";
    optionsScreenState.interpolationDelay = "3";
    optionsScreenState.debugOverlayDefault = false;

    bool debugOverlayEnabled = true;
    int appliedWidth = 0;
    int appliedHeight = 0;
    int appliedFps = 0;

    client::runtime::OptionsApplyResult result = service.Apply(
        optionsScreenState, config, joinScreenState, debugOverlayEnabled,
        [&](int width, int height, int targetFps) {
            appliedWidth = width;
            appliedHeight = height;
            appliedFps = targetFps;
        });

    assert(result.success);
    assert(config.playerName == "alice");
    assert(config.serverHost == "192.168.0.44");
    assert(config.serverPort == 28001);
    assert(config.windowWidth == 1280);
    assert(config.windowHeight == 720);
    assert(config.targetFps == 144);
    assert(config.interpolationDelayTicks == 3);
    assert(config.debugOverlayDefault == false);
    assert(debugOverlayEnabled == false);
    assert(joinScreenState.host == "192.168.0.44");
    assert(joinScreenState.port == "28001");
    assert(joinScreenState.playerName == "alice");
    assert(appliedWidth == 1280);
    assert(appliedHeight == 720);
    assert(appliedFps == 144);

    std::string warning;
    const client::ClientConfig loaded = client::core::LoadClientConfigFile(configPath, warning);
    assert(warning.empty());
    assert(loaded.playerName == "alice");
    assert(loaded.serverHost == "192.168.0.44");
    assert(loaded.serverPort == 28001);
    assert(loaded.windowWidth == 1280);
    assert(loaded.windowHeight == 720);
    assert(loaded.targetFps == 144);
    assert(loaded.interpolationDelayTicks == 3);
    assert(loaded.debugOverlayDefault == false);

    client::ui::OptionsScreenState invalidOptions = optionsScreenState;
    invalidOptions.port = "0";
    client::ClientConfig originalConfig = config;
    bool overlayAfterFailure = debugOverlayEnabled;
    client::runtime::OptionsApplyResult invalidResult =
        service.Apply(invalidOptions, config, joinScreenState, overlayAfterFailure);
    assert(!invalidResult.success);
    assert(invalidResult.statusMessage == "Default port must be between 1 and 65535");
    assert(config.serverPort == originalConfig.serverPort);
    assert(overlayAfterFailure == debugOverlayEnabled);

    fs::remove_all(tempRoot);
    return 0;
}
