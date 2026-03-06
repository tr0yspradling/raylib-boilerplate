#include <cassert>

#include "client/core/menu_model.hpp"
#include "client/ui/ui_state.hpp"

int main() {
    using namespace client;

    ui::JoinServerScreenState form;
    form.ResetFromDefaults("example.com", 28000, "alice");

    assert(form.host == "example.com");
    assert(form.port == "28000");
    assert(form.playerName == "alice");
    assert(form.SelectedIndex() == 0U);
    assert(form.SelectedField() == core::JoinFormField::Host);
    assert(form.SelectedFieldIsEditable());

    form.MoveNext();
    assert(form.SelectedField() == core::JoinFormField::Port);
    assert(form.SelectedFieldIsEditable());

    form.MoveNext();
    assert(form.SelectedField() == core::JoinFormField::Name);
    assert(form.SelectedFieldIsEditable());

    form.MoveNext();
    assert(form.SelectedField() == core::JoinFormField::Connect);
    assert(!form.SelectedFieldIsEditable());

    form.MoveNext();
    assert(form.SelectedField() == core::JoinFormField::Back);
    assert(!form.SelectedFieldIsEditable());

    form.MoveNext();
    assert(form.SelectedField() == core::JoinFormField::Host);

    form.MovePrevious();
    assert(form.SelectedField() == core::JoinFormField::Back);

    assert(core::JoinFormFieldName(core::JoinFormField::Host) == "Host");
    assert(core::JoinFormFieldName(core::JoinFormField::Port) == "Port");
    assert(core::JoinFormFieldName(core::JoinFormField::Name) == "Name");
    assert(core::JoinFormFieldName(core::JoinFormField::Connect) == "Connect");
    assert(core::JoinFormFieldName(core::JoinFormField::Back) == "Back");
    assert(ui::UiWidgetIdForJoinField(core::JoinFormField::Port) == ui::UiWidgetId::JoinPort);
    assert(ui::JoinFieldForWidgetId(ui::UiWidgetId::JoinConnect) == core::JoinFormField::Connect);

    return 0;
}
