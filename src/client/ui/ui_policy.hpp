#pragma once

#include <string_view>

#include <raylib-cpp.hpp>

namespace client::ui::policy {

namespace layout {
inline constexpr float kMenuWidth = 420.0f;
inline constexpr float kJoinWidth = 520.0f;
inline constexpr float kOptionsWidth = 620.0f;
inline constexpr float kRowHeight = 44.0f;
inline constexpr float kMenuRowSpacing = 56.0f;
inline constexpr float kOptionsRowSpacing = 42.0f;
inline constexpr float kMenuStartY = 254.0f;
inline constexpr float kJoinStartY = 244.0f;
inline constexpr float kOptionsStartY = 162.0f;
inline constexpr int kTitleCenterOffset = 140;
inline constexpr int kSubtitleCenterOffset = 220;
inline constexpr int kStatusCenterOffset = 300;
inline constexpr int kFooterCenterOffset = 340;
inline constexpr int kTitleY = 112;
inline constexpr int kSubtitleY = 176;
inline constexpr int kFooterBottomMargin = 62;
inline constexpr float kMinimumStatusBaseline = 520.0f;
inline constexpr float kStatusPadding = 20.0f;
inline constexpr int kStatusBottomMargin = 96;
inline constexpr int kWidgetTextXPadding = 30;
inline constexpr int kWidgetTextYPadding = 6;
inline constexpr float kWidgetCornerRoundness = 0.22f;
inline constexpr int kWidgetCornerSegments = 12;
}  // namespace layout

namespace typography {
inline constexpr int kTitleFontSize = 56;
inline constexpr int kSubtitleFontSize = 24;
inline constexpr int kBodyFontSize = 28;
inline constexpr int kStatusFontSize = 20;
inline constexpr int kFooterFontSize = 20;
}  // namespace typography

namespace copy {
inline constexpr std::string_view kMenuTitle = "Main Menu";
inline constexpr std::string_view kMenuSubtitle = "Select runtime mode";
inline constexpr std::string_view kJoinTitle = "Join Server";
inline constexpr std::string_view kJoinSubtitle = "Configure host, port, and player name";
inline constexpr std::string_view kOptionsTitle = "Options";
inline constexpr std::string_view kOptionsSubtitle = "Persist local client preferences";
inline constexpr std::string_view kRequiredFieldPlaceholder = "<required>";
inline constexpr std::string_view kMenuFooterHint =
    "Navigate: W/S or Up/Down | Select: Enter/Space (A) | Mouse: hover + click";
inline constexpr std::string_view kEditFooterHint =
    "Editing: type text, Backspace to erase, Enter/Esc to finish";
inline constexpr std::string_view kFormFooterHint =
    "Navigate: W/S or Up/Down | Select: Enter/Space (A) | Back: Esc (B) | Mouse: hover + click";
}  // namespace copy

namespace color {
inline const raylib::Color kTitleText{238, 246, 255, 255};
inline const raylib::Color kSubtitleText{168, 194, 214, 255};
inline const raylib::Color kStatusText{255, 209, 140, 255};
inline const raylib::Color kFooterText{168, 196, 214, 255};
inline const raylib::Color kWidgetEditingPanel{255, 206, 112, 255};
inline const raylib::Color kWidgetPressedPanel{86, 172, 230, 255};
inline const raylib::Color kWidgetFocusedPanel{118, 198, 255, 255};
inline const raylib::Color kWidgetHoveredPanel{70, 98, 124, 255};
inline const raylib::Color kWidgetDisabledPanel{34, 42, 50, 255};
inline const raylib::Color kWidgetDefaultPanel{46, 62, 78, 255};
inline const raylib::Color kWidgetActiveText{32, 52, 72, 255};
inline const raylib::Color kWidgetDefaultText{214, 228, 240, 255};
inline const raylib::Color kBackgroundClear{16, 22, 28, 255};
inline const raylib::Color kBackgroundGradientTop{28, 44, 62, 190};
inline const raylib::Color kBackgroundGradientBottom{16, 22, 28, 0};
}  // namespace color

}  // namespace client::ui::policy
