#include <cassert>

#include "client/ui/ui_document.hpp"

int main() {
    client::ui::UiDocument document;
    document.widgets.push_back({
        .id = client::ui::UiWidgetId::MenuStartServer,
        .kind = client::ui::UiWidgetKind::Button,
        .bounds = client::ui::UiRect{10.0f, 20.0f, 100.0f, 30.0f},
        .label = "Start Server",
    });
    document.widgets.push_back({
        .id = client::ui::UiWidgetId::MenuJoinServer,
        .kind = client::ui::UiWidgetKind::Button,
        .bounds = client::ui::UiRect{10.0f, 60.0f, 100.0f, 30.0f},
        .label = "Join Server",
    });

    assert(document.FindWidgetAt(12.0f, 25.0f) == client::ui::UiWidgetId::MenuStartServer);
    assert(document.FindWidgetAt(24.0f, 75.0f) == client::ui::UiWidgetId::MenuJoinServer);
    assert(!document.FindWidgetAt(400.0f, 400.0f).has_value());

    assert(client::ui::AdvanceFocus(document, std::nullopt, true) == client::ui::UiWidgetId::MenuStartServer);
    assert(client::ui::AdvanceFocus(document, client::ui::UiWidgetId::MenuStartServer, true) ==
        client::ui::UiWidgetId::MenuJoinServer);
    assert(client::ui::AdvanceFocus(document, client::ui::UiWidgetId::MenuStartServer, false) ==
        client::ui::UiWidgetId::MenuJoinServer);

    return 0;
}
