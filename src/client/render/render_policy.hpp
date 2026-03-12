#pragma once

#include <string_view>

namespace client::render::policy {

namespace background {
inline constexpr int kMinimumTopBandHeight = 80;
inline constexpr int kTopBandDivisor = 6;
}  // namespace background

namespace world {
inline constexpr float kGroundHeightRatio = 0.78f;
inline constexpr float kPlayerRadius = 18.0f;
inline constexpr float kPixelsPerWorldUnit = 52.0f;
inline constexpr int kPlayerNameCenterOffset = 30;
inline constexpr int kPlayerNameVerticalOffset = 36;
inline constexpr int kControlsLeftMargin = 20;
inline constexpr int kControlsBottomMargin = 36;
inline constexpr std::string_view kControlsHint =
    "Move: A/D or arrows | Jump: Space | Esc menu | Tab debug overlay";
inline constexpr int kLabelFontSize = 18;
inline constexpr int kControlsFontSize = 20;
}  // namespace world

namespace status_copy {
inline constexpr std::string_view kSplashTitle = "Authoritative Multiplayer";
inline constexpr std::string_view kSplashSubtitle = "raylib runtime bootstrap";
inline constexpr std::string_view kSplashFooter = "Press Enter/Space to continue";
inline constexpr std::string_view kConnectingTitle = "Joining Dedicated Server";
inline constexpr std::string_view kConnectingSubtitle = "Connecting to authoritative host...";
inline constexpr std::string_view kConnectingFooter = "Press Esc to cancel";
inline constexpr std::string_view kStartingServerTitle = "Starting Local Dedicated Server";
inline constexpr std::string_view kStartingServerSubtitle = "Launching sibling game_server...";
inline constexpr std::string_view kSingleplayerTitle = "Singleplayer Sandbox";
inline constexpr std::string_view kSingleplayerSubtitle = "Local authoritative sandbox active";
inline constexpr std::string_view kSingleplayerFooter = "Press Esc to return to menu";
inline constexpr std::string_view kOptionsTitle = "Options";
inline constexpr std::string_view kOptionsSubtitle = "Persist local client preferences";
inline constexpr std::string_view kOptionsFooter = "Navigate and save preferences";
inline constexpr std::string_view kDisconnectedTitle = "Disconnected";
inline constexpr std::string_view kDisconnectedSubtitle = "Connection closed";
inline constexpr std::string_view kDisconnectedFooter = "Press Enter or Esc to return to menu";
}  // namespace status_copy

}  // namespace client::render::policy
