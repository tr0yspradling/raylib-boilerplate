#pragma once

#include <string>

#include <raylib-cpp.hpp>

#include "client/ui/ui_document.hpp"

namespace client::ui {

class UiRenderer {
public:
    static void Draw(const UiDocument& document, int width, int height) {
        if (!document.title.empty()) {
            raylib::Color{238, 246, 255, 255}.DrawText(document.title.c_str(), width / 2 - 140, 112, 56);
        }
        if (!document.subtitle.empty()) {
            raylib::Color{168, 194, 214, 255}.DrawText(document.subtitle.c_str(), width / 2 - 220, 176, 24);
        }

        for (const UiWidget& widget : document.widgets) {
            DrawWidget(widget);
        }

        if (!document.statusMessage.empty()) {
            raylib::Color{255, 209, 140, 255}.DrawText(document.statusMessage.c_str(), width / 2 - 300, 548, 20);
        }

        if (!document.footerHint.empty()) {
            raylib::Color{168, 196, 214, 255}.DrawText(document.footerHint.c_str(), width / 2 - 340, height - 62, 20);
        }
    }

private:
    static void DrawWidget(const UiWidget& widget) {
        const raylib::Color panelColor = WidgetPanelColor(widget.state);
        const raylib::Color textColor = WidgetTextColor(widget.state);
        DrawRectangleRounded({widget.bounds.x, widget.bounds.y, widget.bounds.width, widget.bounds.height}, 0.22f, 12,
                             panelColor);

        std::string text;
        if (widget.kind == UiWidgetKind::TextField) {
            const std::string value = widget.value.empty() ? std::string{"<required>"} : widget.value;
            text = widget.label + ": " + value;
            if (widget.state.editing) {
                text += "_";
            }
        } else {
            text = widget.label;
        }

        textColor.DrawText(text.c_str(), static_cast<int>(widget.bounds.x) + 30, static_cast<int>(widget.bounds.y) + 6, 28);
    }

    [[nodiscard]] static raylib::Color WidgetPanelColor(const UiWidgetState& state) {
        if (state.editing) {
            return raylib::Color{255, 206, 112, 255};
        }
        if (state.pressed) {
            return raylib::Color{86, 172, 230, 255};
        }
        if (state.focused) {
            return raylib::Color{118, 198, 255, 255};
        }
        if (state.hovered) {
            return raylib::Color{70, 98, 124, 255};
        }
        if (state.disabled) {
            return raylib::Color{34, 42, 50, 255};
        }
        return raylib::Color{46, 62, 78, 255};
    }

    [[nodiscard]] static raylib::Color WidgetTextColor(const UiWidgetState& state) {
        if (state.focused || state.editing || state.pressed) {
            return raylib::Color{32, 52, 72, 255};
        }
        return raylib::Color{214, 228, 240, 255};
    }
};

}  // namespace client::ui
