#include <cassert>

#include "client/core/menu_model.hpp"
#include "client/ui/ui_state.hpp"

int main() {
    using namespace client;

    ui::MenuScreenState selection;
    assert(selection.SelectedIndex() == 0);
    assert(selection.SelectedAction() == core::MenuAction::StartServer);

    selection.MovePrevious();
    assert(selection.SelectedIndex() == 4);
    assert(selection.SelectedAction() == core::MenuAction::Quit);

    selection.MoveNext();
    assert(selection.SelectedIndex() == 0);

    selection.SetSelectedIndex(2);
    assert(selection.SelectedAction() == core::MenuAction::Singleplayer);

    selection.SetSelectedIndex(99);
    assert(selection.SelectedIndex() == 2);

    assert(core::MenuActionName(core::MenuAction::StartServer) == "Start Server");
    assert(core::MenuActionName(core::MenuAction::JoinServer) == "Join Server");
    assert(core::MenuActionName(core::MenuAction::Singleplayer) == "Singleplayer");
    assert(core::MenuActionName(core::MenuAction::Options) == "Options");
    assert(core::MenuActionName(core::MenuAction::Quit) == "Quit");
    assert(ui::UiWidgetIdForMenuAction(core::MenuAction::JoinServer) == ui::UiWidgetId::MenuJoinServer);
    assert(ui::MenuActionForWidgetId(ui::UiWidgetId::MenuQuit) == core::MenuAction::Quit);

    return 0;
}
