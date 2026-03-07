#include "client/runtime/options_service.hpp"

#include <charconv>
#include <filesystem>
#include <string_view>
#include <system_error>

#include "client/core/config.hpp"

namespace client::runtime {

namespace {

[[nodiscard]] bool ParseNumber(std::string_view value, int minimum, int maximum, const char* label, int& out,
                               std::string& error) {
    if (value.empty()) {
        error = std::string{label} + " is required";
        return false;
    }

    const char* begin = value.data();
    const char* end = begin + value.size();
    const auto [ptr, parseError] = std::from_chars(begin, end, out);
    if (parseError != std::errc{} || ptr != end || out < minimum || out > maximum) {
        error = std::string{label} + " must be between " + std::to_string(minimum) + " and " + std::to_string(maximum);
        return false;
    }

    return true;
}

}  // namespace

OptionsApplyResult OptionsService::Apply(const ui::OptionsScreenState& optionsScreenState, ClientConfig& config,
                                         ui::JoinServerScreenState& joinScreenState, bool& debugOverlayEnabled,
                                         ApplyWindowSettingsFn applyWindowSettings) const {
    if (optionsScreenState.playerName.empty()) {
        return {.success = false, .statusMessage = "Player name is required"};
    }
    if (optionsScreenState.host.empty()) {
        return {.success = false, .statusMessage = "Default host is required"};
    }

    std::string validationError;
    int parsedPort = 0;
    int parsedWindowWidth = 0;
    int parsedWindowHeight = 0;
    int parsedTargetFps = 0;
    int parsedInterpolationDelay = 0;
    if (!ParseNumber(optionsScreenState.port, 1, 65535, "Default port", parsedPort, validationError) ||
        !ParseNumber(optionsScreenState.windowWidth, 640, 3840, "Window width", parsedWindowWidth, validationError) ||
        !ParseNumber(optionsScreenState.windowHeight, 360, 2160, "Window height", parsedWindowHeight, validationError) ||
        !ParseNumber(optionsScreenState.targetFps, 30, 360, "Target FPS", parsedTargetFps, validationError) ||
        !ParseNumber(optionsScreenState.interpolationDelay, 0, 10, "Interpolation delay", parsedInterpolationDelay,
                     validationError)) {
        return {.success = false, .statusMessage = std::move(validationError)};
    }

    config.playerName = optionsScreenState.playerName;
    config.serverHost = optionsScreenState.host;
    config.serverPort = static_cast<uint16_t>(parsedPort);
    config.windowWidth = parsedWindowWidth;
    config.windowHeight = parsedWindowHeight;
    config.targetFps = parsedTargetFps;
    config.interpolationDelayTicks = parsedInterpolationDelay;
    config.debugOverlayDefault = optionsScreenState.debugOverlayDefault;

    if (applyWindowSettings) {
        applyWindowSettings(config.windowWidth, config.windowHeight, config.targetFps);
    }
    debugOverlayEnabled = config.debugOverlayDefault;

    joinScreenState.ResetFromDefaults(config.serverHost, config.serverPort, config.playerName);

    std::string saveError;
    const std::filesystem::path configPath = config.configFilePath.empty() ? core::DefaultClientConfigPath()
                                                                           : std::filesystem::path{config.configFilePath};
    config.configFilePath = configPath.string();
    if (!core::SaveClientConfigFile(config, configPath, saveError)) {
        return {.success = false, .statusMessage = "Save failed: " + saveError};
    }

    return {.success = true, .statusMessage = "Saved client preferences to " + configPath.string()};
}

}  // namespace client::runtime
