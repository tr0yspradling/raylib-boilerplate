#pragma once

#include <algorithm>
#include <string>

#include <raylib-cpp.hpp>

#include "client/ui/ui_document.hpp"
#include "client/ui/ui_policy.hpp"

namespace client::ui {

class UiRenderer {
public:
    static void Draw(const UiDocument& document, int width, int height) {
        if (!document.title.empty()) {
            policy::color::kTitleText.DrawText(document.title.c_str(), width / 2 - policy::layout::kTitleCenterOffset,
                                               policy::layout::kTitleY, policy::typography::kTitleFontSize);
        }
        if (!document.subtitle.empty()) {
            policy::color::kSubtitleText.DrawText(document.subtitle.c_str(),
                                                  width / 2 - policy::layout::kSubtitleCenterOffset,
                                                  policy::layout::kSubtitleY, policy::typography::kSubtitleFontSize);
        }

        for (const UiWidget& widget : document.widgets) {
            DrawWidget(widget);
        }

        if (!document.statusMessage.empty()) {
            const int statusY = ComputeStatusY(document, height);
            policy::color::kStatusText.DrawText(document.statusMessage.c_str(),
                                                width / 2 - policy::layout::kStatusCenterOffset, statusY,
                                                policy::typography::kStatusFontSize);
        }

        if (!document.footerHint.empty()) {
            policy::color::kFooterText.DrawText(document.footerHint.c_str(),
                                                width / 2 - policy::layout::kFooterCenterOffset,
                                                height - policy::layout::kFooterBottomMargin,
                                                policy::typography::kFooterFontSize);
        }
    }

private:
    [[nodiscard]] static int ComputeStatusY(const UiDocument& document, int height) {
        float maxBottom = policy::layout::kMinimumStatusBaseline;
        for (const UiWidget& widget : document.widgets) {
            maxBottom = std::max(maxBottom, widget.bounds.y + widget.bounds.height);
        }
        return std::min(static_cast<int>(maxBottom + policy::layout::kStatusPadding),
                        height - policy::layout::kStatusBottomMargin);
    }

    static void DrawWidget(const UiWidget& widget) {
        const raylib::Color panelColor = WidgetPanelColor(widget.state);
        const raylib::Color textColor = WidgetTextColor(widget.state);
        DrawRectangleRounded({widget.bounds.x, widget.bounds.y, widget.bounds.width, widget.bounds.height},
                             policy::layout::kWidgetCornerRoundness, policy::layout::kWidgetCornerSegments, panelColor);

        std::string text;
        if (widget.kind == UiWidgetKind::TextField) {
            const std::string value =
                widget.value.empty() ? std::string{policy::copy::kRequiredFieldPlaceholder} : widget.value;
            text = widget.label + ": " + value;
            if (widget.state.editing) {
                text += "_";
            }
        } else {
            text = widget.label;
        }

        textColor.DrawText(text.c_str(), static_cast<int>(widget.bounds.x) + policy::layout::kWidgetTextXPadding,
                           static_cast<int>(widget.bounds.y) + policy::layout::kWidgetTextYPadding,
                           policy::typography::kBodyFontSize);
    }

    [[nodiscard]] static raylib::Color WidgetPanelColor(const UiWidgetState& state) {
        if (state.editing) {
            return policy::color::kWidgetEditingPanel;
        }
        if (state.pressed) {
            return policy::color::kWidgetPressedPanel;
        }
        if (state.focused) {
            return policy::color::kWidgetFocusedPanel;
        }
        if (state.hovered) {
            return policy::color::kWidgetHoveredPanel;
        }
        if (state.disabled) {
            return policy::color::kWidgetDisabledPanel;
        }
        return policy::color::kWidgetDefaultPanel;
    }

    [[nodiscard]] static raylib::Color WidgetTextColor(const UiWidgetState& state) {
        if (state.focused || state.editing || state.pressed) {
            return policy::color::kWidgetActiveText;
        }
        return policy::color::kWidgetDefaultText;
    }
};

}  // namespace client::ui
