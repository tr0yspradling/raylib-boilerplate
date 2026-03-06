#include <cassert>

#include "client/core/menu_model.hpp"

int main() {
    using namespace client::core;

    MenuSelectionState selection;
    assert(selection.SelectedIndex() == 0);
    assert(selection.SelectedAction() == MenuAction::StartServer);

    selection.MovePrevious();
    assert(selection.SelectedIndex() == 4);
    assert(selection.SelectedAction() == MenuAction::Quit);

    selection.MoveNext();
    assert(selection.SelectedIndex() == 0);

    selection.SetSelectedIndex(2);
    assert(selection.SelectedAction() == MenuAction::Singleplayer);

    selection.SetSelectedIndex(99);
    assert(selection.SelectedIndex() == 2);

    assert(MenuActionName(MenuAction::StartServer) == "Start Server");
    assert(MenuActionName(MenuAction::JoinServer) == "Join Server");
    assert(MenuActionName(MenuAction::Singleplayer) == "Singleplayer");
    assert(MenuActionName(MenuAction::Options) == "Options");
    assert(MenuActionName(MenuAction::Quit) == "Quit");

    return 0;
}
