#include "server/config/server_config.hpp"

#include <algorithm>
#include <cctype>
#include <charconv>
#include <fstream>
#include <sstream>
#include <string_view>

#include "shared/game/validation.hpp"

namespace server {

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

bool ParseInt(const std::string& value, int& out) {
    const char* start = value.data();
    const char* end = value.data() + value.size();
    auto [ptr, ec] = std::from_chars(start, end, out);
    return ec == std::errc{} && ptr == end;
}

bool ParseUInt32(const std::string& value, uint32_t& out) {
    const char* start = value.data();
    const char* end = value.data() + value.size();
    auto [ptr, ec] = std::from_chars(start, end, out);
    return ec == std::errc{} && ptr == end;
}

bool ParseFloat(const std::string& value, float& out) {
    std::stringstream stream{value};
    stream >> out;
    return stream.good() || stream.eof();
}

bool ParseBool(const std::string& value, bool& out) {
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

}  // namespace

ServerConfig LoadServerConfigFile(const std::string& path, std::string& warning) {
    ServerConfig config;

    std::ifstream file{path};
    if (!file.is_open()) {
        warning = "config file not found, using defaults: " + path;
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
        uint32_t u32Value = 0;
        float floatValue = 0.0f;
        bool boolValue = false;

        if (key == "bind_address") {
            config.bindAddress = value;
        } else if (key == "listen_port" && ParseInt(value, intValue)) {
            config.listenPort = static_cast<uint16_t>(intValue);
        } else if (key == "simulation_tick_hz" && ParseInt(value, intValue)) {
            config.simulationTickHz = std::max(1, intValue);
        } else if (key == "snapshot_rate_hz" && ParseInt(value, intValue)) {
            config.snapshotRateHz = std::max(1, intValue);
        } else if (key == "max_input_frames_per_second" && ParseInt(value, intValue)) {
            config.maxInputFramesPerSecond = std::max(1, intValue);
        } else if (key == "max_chunk_hints_per_second" && ParseInt(value, intValue)) {
            config.maxChunkHintsPerSecond = std::max(1, intValue);
        } else if (key == "max_chunk_resync_requests_per_second" && ParseInt(value, intValue)) {
            config.maxChunkResyncRequestsPerSecond = std::max(1, intValue);
        } else if (key == "metrics_log_interval_seconds" && ParseInt(value, intValue)) {
            config.metricsLogIntervalSeconds = std::max(0, intValue);
        } else if (key == "world_chunk_width_tiles" && ParseInt(value, intValue)) {
            config.worldConfig.chunkWidthTiles = std::max(1, intValue);
        } else if (key == "world_chunk_height_tiles" && ParseInt(value, intValue)) {
            config.worldConfig.chunkHeightTiles = std::max(1, intValue);
        } else if (key == "world_tile_size" && ParseInt(value, intValue)) {
            config.worldConfig.tileSize = std::max(1, intValue);
        } else if (key == "world_interest_radius_chunks" && ParseInt(value, intValue)) {
            config.worldConfig.interestRadiusChunks = std::max(1, intValue);
        } else if (key == "max_clients" && ParseInt(value, intValue)) {
            config.maxClients = std::max(1, intValue);
        } else if (key == "enforce_build_hash" && ParseBool(value, boolValue)) {
            config.enforceBuildHash = boolValue;
        } else if (key == "required_build_hash" && ParseUInt32(value, u32Value)) {
            config.requiredBuildHash = u32Value;
        } else if (key == "auth_mode") {
            if (value == "dev") {
                config.authMode = shared::net::AuthMode::DevInsecure;
            } else if (value == "token") {
                config.authMode = shared::net::AuthMode::BackendToken;
            }
        } else if (key == "persistence_path") {
            config.persistencePath = value;
        } else if (key == "player_max_move_speed" && ParseFloat(value, floatValue)) {
            config.playerKinematics.maxMoveSpeed = floatValue;
        } else if (key == "player_jump_speed" && ParseFloat(value, floatValue)) {
            config.playerKinematics.jumpSpeed = floatValue;
        } else if (key == "player_gravity" && ParseFloat(value, floatValue)) {
            config.playerKinematics.gravity = floatValue;
        } else if (key == "player_max_fall_speed" && ParseFloat(value, floatValue)) {
            config.playerKinematics.maxFallSpeed = floatValue;
        } else if (key == "player_ground_y" && ParseFloat(value, floatValue)) {
            config.playerKinematics.groundY = floatValue;
        } else if (key == "world_min_x" && ParseFloat(value, floatValue)) {
            config.playerKinematics.minX = floatValue;
        } else if (key == "world_max_x" && ParseFloat(value, floatValue)) {
            config.playerKinematics.maxX = floatValue;
        } else if (key == "fake_lag_ms" && ParseFloat(value, floatValue)) {
            config.fakeLagMs = std::max(0.0f, floatValue);
        } else if (key == "fake_jitter_ms" && ParseFloat(value, floatValue)) {
            config.fakeJitterMs = std::max(0.0f, floatValue);
        } else if (key == "fake_loss_send_pct" && ParseFloat(value, floatValue)) {
            config.fakeLossSendPct = std::clamp(floatValue, 0.0f, 100.0f);
        } else if (key == "fake_loss_recv_pct" && ParseFloat(value, floatValue)) {
            config.fakeLossRecvPct = std::clamp(floatValue, 0.0f, 100.0f);
        } else {
            warning += "ignored unknown key '" + key + "' at line " + std::to_string(lineNumber) + "\n";
        }
    }

    const shared::game::PlayerKinematicsValidationError kinematicsValidation =
        shared::game::ValidatePlayerKinematicsConfig(config.playerKinematics);
    if (kinematicsValidation != shared::game::PlayerKinematicsValidationError::None) {
        warning += "invalid player kinematics config (" +
            std::string{shared::game::ToString(kinematicsValidation)} + "), using defaults\n";
        config.playerKinematics = shared::game::PlayerKinematicsConfig{};
    }

    config.worldConfig.chunkWidthTiles = std::clamp(config.worldConfig.chunkWidthTiles, 1, 255);
    config.worldConfig.chunkHeightTiles = std::clamp(config.worldConfig.chunkHeightTiles, 1, 255);
    config.worldConfig.tileSize = std::max(1, config.worldConfig.tileSize);
    config.worldConfig.interestRadiusChunks = std::max(1, config.worldConfig.interestRadiusChunks);
    config.maxInputFramesPerSecond = std::max(1, config.maxInputFramesPerSecond);
    config.maxChunkHintsPerSecond = std::max(1, config.maxChunkHintsPerSecond);
    config.maxChunkResyncRequestsPerSecond = std::max(1, config.maxChunkResyncRequestsPerSecond);
    config.metricsLogIntervalSeconds = std::clamp(config.metricsLogIntervalSeconds, 0, 600);

    return config;
}

}  // namespace server
