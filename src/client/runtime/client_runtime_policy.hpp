#pragma once

#include <chrono>
#include <cstddef>
#include <string_view>

namespace client::runtime::policy {

inline constexpr std::string_view kWindowTitle = "raylib boilerplate - multiplayer client";

inline constexpr auto kSplashDuration = std::chrono::milliseconds{1400};
inline constexpr auto kLocalServerConnectRetryInterval = std::chrono::milliseconds{250};
inline constexpr auto kLocalServerStartupTimeout = std::chrono::seconds{10};

inline constexpr std::size_t kNumericFieldMaxDigits = 5U;
inline constexpr std::size_t kHostFieldMaxLength = 64U;
inline constexpr std::size_t kPlayerNameFieldMaxLength = 24U;

inline constexpr std::string_view kFieldEditingStatus = "Editing field. Type, Backspace to erase, Enter/Esc to finish";
inline constexpr std::string_view kJoinFieldSelectionStatus = "Select a field and press Enter to edit";
inline constexpr std::string_view kOptionsFieldSelectionStatus = "Select a field and press Enter to edit";
inline constexpr std::string_view kLocalServerLaunchStatus = "Launching local dedicated server...";
inline constexpr std::string_view kLocalServerWaitingStatus = "Waiting for local dedicated server...";
inline constexpr std::string_view kLocalServerConnectingStatus = "Connecting to local dedicated server...";
inline constexpr std::string_view kJoinConnectingStatus = "Connecting...";
inline constexpr std::string_view kJoinCanceledStatus = "join canceled";
inline constexpr std::string_view kLocalServerCanceledStatus = "Local dedicated startup canceled";
inline constexpr std::string_view kLocalServerTimeoutStatus = "Timed out waiting for local dedicated server startup";
inline constexpr std::string_view kLocalServerExitedStatus = "Local dedicated server exited before accepting connections";
inline constexpr std::string_view kConnectionClosedStatus = "connection closed";
inline constexpr std::string_view kLocalDedicatedBootLabel = "Booting local dedicated";
inline constexpr std::string_view kLocalSandboxLabel = "Local sandbox";
inline constexpr std::string_view kSettingsLabel = "Settings";
inline constexpr std::string_view kMenuDefaultStatus = "Select mode";
inline constexpr std::string_view kJoinDefaultStatus = "Configure destination";

inline constexpr std::string_view kHostRequiredMessage = "Host is required";
inline constexpr std::string_view kPlayerNameRequiredMessage = "Player name is required";
inline constexpr std::string_view kPortRequiredMessage = "Port is required";
inline constexpr std::string_view kPortRangeMessage = "Port must be between 1 and 65535";

inline constexpr std::string_view kJoinFailedPrefix = "Join failed: ";
inline constexpr std::string_view kTransportInitFailedPrefix = "Transport init failed: ";
inline constexpr std::string_view kConnectFailedPrefix = "Connect failed: ";
inline constexpr std::string_view kFailedToLaunchServerPrefix = "Failed to launch local dedicated server: ";

}  // namespace client::runtime::policy
