#include "client/core/config.hpp"

#include <algorithm>
#include <cctype>
#include <charconv>
#include <fstream>
#include <string_view>
#include <system_error>

#include "client/core/client_config_policy.hpp"

namespace client::core {

namespace {

[[nodiscard]] std::string Trim(std::string_view value) {
    size_t begin = 0;
    while (begin < value.size() && std::isspace(static_cast<unsigned char>(value[begin])) != 0) {
        ++begin;
    }

    size_t end = value.size();
    while (end > begin && std::isspace(static_cast<unsigned char>(value[end - 1])) != 0) {
        --end;
    }

    return std::string{value.substr(begin, end - begin)};
}

[[nodiscard]] bool ParseInt(std::string_view value, int& out) {
    const char* begin = value.data();
    const char* end = value.data() + value.size();
    const auto [ptr, error] = std::from_chars(begin, end, out);
    return error == std::errc{} && ptr == end;
}

[[nodiscard]] bool ParseBool(std::string_view value, bool& out) {
    if (value == "1" || value == "true" || value == "TRUE" || value == "on") {
        out = true;
        return true;
    }
    if (value == "0" || value == "false" || value == "FALSE" || value == "off") {
        out = false;
        return true;
    }
    return false;
}

void ClampClientConfig(client::ClientConfig& config) {
    config.serverPort =
        static_cast<uint16_t>(std::clamp<int>(config.serverPort, policy::kMinServerPort, policy::kMaxServerPort));
    config.windowWidth = std::clamp(config.windowWidth, policy::kMinWindowWidth, policy::kMaxWindowWidth);
    config.windowHeight = std::clamp(config.windowHeight, policy::kMinWindowHeight, policy::kMaxWindowHeight);
    config.targetFps = std::clamp(config.targetFps, policy::kMinTargetFps, policy::kMaxTargetFps);
    config.simulationTickHz =
        std::clamp(config.simulationTickHz, policy::kMinSimulationTickHz, policy::kMaxSimulationTickHz);
    config.interpolationDelayTicks = std::clamp(config.interpolationDelayTicks, policy::kMinInterpolationDelayTicks,
                                                policy::kMaxInterpolationDelayTicks);
    if (config.playerName.empty()) {
        config.playerName = std::string{policy::kDefaultPlayerName};
    }
    if (config.serverHost.empty()) {
        config.serverHost = std::string{policy::kDefaultServerHost};
    }
}

}  // namespace

std::filesystem::path DefaultClientConfigPath() {
    return policy::DefaultClientConfigPath();
}

client::ClientConfig LoadClientConfigFile(const std::filesystem::path& path, std::string& warning) {
    client::ClientConfig config;
    config.configFilePath = path.string();

    std::ifstream file{path};
    if (!file.is_open()) {
        warning.clear();
        return config;
    }

    warning.clear();
    std::string line;
    int lineNumber = 0;
    while (std::getline(file, line)) {
        ++lineNumber;

        const std::string trimmed = Trim(line);
        if (trimmed.empty() || trimmed.starts_with('#') || trimmed.starts_with(';')) {
            continue;
        }

        const size_t equalPos = trimmed.find('=');
        if (equalPos == std::string::npos) {
            warning += "ignored malformed line " + std::to_string(lineNumber) + "\n";
            continue;
        }

        const std::string key = Trim(std::string_view{trimmed}.substr(0, equalPos));
        const std::string value = Trim(std::string_view{trimmed}.substr(equalPos + 1));
        int intValue = 0;
        bool boolValue = false;

        if (key == "host") {
            config.serverHost = value;
        } else if (key == "port" && ParseInt(value, intValue)) {
            config.serverPort = static_cast<uint16_t>(intValue);
        } else if (key == "player_name") {
            config.playerName = value;
        } else if (key == "window_width" && ParseInt(value, intValue)) {
            config.windowWidth = intValue;
        } else if (key == "window_height" && ParseInt(value, intValue)) {
            config.windowHeight = intValue;
        } else if (key == "target_fps" && ParseInt(value, intValue)) {
            config.targetFps = intValue;
        } else if (key == "interpolation_delay_ticks" && ParseInt(value, intValue)) {
            config.interpolationDelayTicks = intValue;
        } else if (key == "debug_overlay_default" && ParseBool(value, boolValue)) {
            config.debugOverlayDefault = boolValue;
        } else {
            warning += "ignored unknown key '" + key + "' at line " + std::to_string(lineNumber) + "\n";
        }
    }

    ClampClientConfig(config);
    return config;
}

bool SaveClientConfigFile(const client::ClientConfig& config, const std::filesystem::path& path, std::string& error) {
    client::ClientConfig saved = config;
    ClampClientConfig(saved);

    std::error_code createError;
    if (!path.parent_path().empty()) {
        std::filesystem::create_directories(path.parent_path(), createError);
        if (createError) {
            error = "failed to create config directory: " + createError.message();
            return false;
        }
    }

    std::ofstream file{path};
    if (!file.is_open()) {
        error = "failed to open config file for writing: " + path.string();
        return false;
    }

    file << policy::kConfigHeader << '\n';
    file << "host=" << saved.serverHost << '\n';
    file << "port=" << saved.serverPort << '\n';
    file << "player_name=" << saved.playerName << '\n';
    file << "window_width=" << saved.windowWidth << '\n';
    file << "window_height=" << saved.windowHeight << '\n';
    file << "target_fps=" << saved.targetFps << '\n';
    file << "interpolation_delay_ticks=" << saved.interpolationDelayTicks << '\n';
    file << "debug_overlay_default=" << (saved.debugOverlayDefault ? "true" : "false") << '\n';

    if (!file.good()) {
        error = "failed while writing config file: " + path.string();
        return false;
    }

    error.clear();
    return true;
}

}  // namespace client::core
