#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "client/core/scene.hpp"
#include "client/ui/ui_state.hpp"

namespace client::ui {

enum class UiWidgetKind : uint8_t {
    Button = 0,
    TextField,
};

struct UiRect {
    float x = 0.0f;
    float y = 0.0f;
    float width = 0.0f;
    float height = 0.0f;

    [[nodiscard]] bool Contains(float pointX, float pointY) const {
        return pointX >= x && pointX <= (x + width) && pointY >= y && pointY <= (y + height);
    }
};

struct UiWidgetState {
    bool hovered = false;
    bool focused = false;
    bool pressed = false;
    bool editing = false;
    bool disabled = false;
};

struct UiWidget {
    UiWidgetId id = UiWidgetId::None;
    UiWidgetKind kind = UiWidgetKind::Button;
    UiRect bounds{};
    std::string label;
    std::string value;
    UiWidgetState state{};
};

struct UiDocument {
    core::SceneKind scene = core::SceneKind::Splash;
    std::string title;
    std::string subtitle;
    std::string statusMessage;
    std::string footerHint;
    std::vector<UiWidget> widgets;

    [[nodiscard]] const UiWidget* FindWidget(UiWidgetId widgetId) const {
        for (const UiWidget& widget : widgets) {
            if (widget.id == widgetId) {
                return &widget;
            }
        }
        return nullptr;
    }

    [[nodiscard]] UiWidget* FindWidget(UiWidgetId widgetId) {
        for (UiWidget& widget : widgets) {
            if (widget.id == widgetId) {
                return &widget;
            }
        }
        return nullptr;
    }

    [[nodiscard]] std::optional<UiWidgetId> FindWidgetAt(float pointX, float pointY) const {
        for (const UiWidget& widget : widgets) {
            if (widget.bounds.Contains(pointX, pointY)) {
                return widget.id;
            }
        }
        return std::nullopt;
    }
};

[[nodiscard]] inline std::optional<size_t> FindWidgetIndex(const UiDocument& document, UiWidgetId widgetId) {
    for (size_t index = 0; index < document.widgets.size(); ++index) {
        if (document.widgets[index].id == widgetId) {
            return index;
        }
    }
    return std::nullopt;
}

[[nodiscard]] inline std::optional<UiWidgetId> AdvanceFocus(const UiDocument& document,
                                                            std::optional<UiWidgetId> current,
                                                            bool forward) {
    if (document.widgets.empty()) {
        return std::nullopt;
    }

    if (!current.has_value()) {
        return forward ? std::optional<UiWidgetId>{document.widgets.front().id}
                       : std::optional<UiWidgetId>{document.widgets.back().id};
    }

    const std::optional<size_t> foundIndex = FindWidgetIndex(document, *current);
    if (!foundIndex.has_value()) {
        return forward ? std::optional<UiWidgetId>{document.widgets.front().id}
                       : std::optional<UiWidgetId>{document.widgets.back().id};
    }

    const size_t size = document.widgets.size();
    const size_t nextIndex = forward ? ((*foundIndex + 1U) % size) : (*foundIndex == 0U ? (size - 1U) : (*foundIndex - 1U));
    return document.widgets[nextIndex].id;
}

}  // namespace client::ui
