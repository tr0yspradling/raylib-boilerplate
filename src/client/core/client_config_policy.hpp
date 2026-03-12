#pragma once

#include <cstdint>
#include <filesystem>
#include <string_view>

namespace client::core::policy {

inline constexpr std::string_view kDefaultServerHost = "127.0.0.1";
inline constexpr uint16_t kDefaultServerPort = 27020;
inline constexpr std::string_view kDefaultPlayerName = "player";
inline constexpr int kDefaultWindowWidth = 1600;
inline constexpr int kDefaultWindowHeight = 900;
inline constexpr int kDefaultTargetFps = 120;
inline constexpr int kDefaultSimulationTickHz = 30;
inline constexpr int kDefaultInterpolationDelayTicks = 2;
inline constexpr bool kDefaultDebugOverlayEnabled = true;

inline constexpr int kMinServerPort = 1;
inline constexpr int kMaxServerPort = 65535;
inline constexpr int kMinWindowWidth = 640;
inline constexpr int kMaxWindowWidth = 3840;
inline constexpr int kMinWindowHeight = 360;
inline constexpr int kMaxWindowHeight = 2160;
inline constexpr int kMinTargetFps = 30;
inline constexpr int kMaxTargetFps = 360;
inline constexpr int kMinSimulationTickHz = 1;
inline constexpr int kMaxSimulationTickHz = 240;
inline constexpr int kMinInterpolationDelayTicks = 0;
inline constexpr int kMaxInterpolationDelayTicks = 10;

inline constexpr std::string_view kConfigDirectoryName = "client_data";
inline constexpr std::string_view kConfigFileName = "client.cfg";
inline constexpr std::string_view kConfigHeader = "# Client preferences";

[[nodiscard]] inline std::filesystem::path DefaultClientConfigPath() {
    return std::filesystem::path{std::string{kConfigDirectoryName}} / std::string{kConfigFileName};
}

}  // namespace client::core::policy
