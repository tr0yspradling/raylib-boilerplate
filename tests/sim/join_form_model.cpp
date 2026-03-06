#include <cassert>

#include "client/core/menu_model.hpp"

int main() {
    using namespace client::core;

    JoinServerFormState form;
    form.ResetFromDefaults("example.com", 28000, "alice");

    assert(form.host == "example.com");
    assert(form.port == "28000");
    assert(form.playerName == "alice");
    assert(form.SelectedIndex() == 0U);
    assert(form.SelectedField() == JoinFormField::Host);
    assert(form.SelectedFieldIsEditable());

    form.MoveNext();
    assert(form.SelectedField() == JoinFormField::Port);
    assert(form.SelectedFieldIsEditable());

    form.MoveNext();
    assert(form.SelectedField() == JoinFormField::Name);
    assert(form.SelectedFieldIsEditable());

    form.MoveNext();
    assert(form.SelectedField() == JoinFormField::Connect);
    assert(!form.SelectedFieldIsEditable());

    form.MoveNext();
    assert(form.SelectedField() == JoinFormField::Back);
    assert(!form.SelectedFieldIsEditable());

    form.MoveNext();
    assert(form.SelectedField() == JoinFormField::Host);

    form.MovePrevious();
    assert(form.SelectedField() == JoinFormField::Back);

    assert(JoinFormFieldName(JoinFormField::Host) == "Host");
    assert(JoinFormFieldName(JoinFormField::Port) == "Port");
    assert(JoinFormFieldName(JoinFormField::Name) == "Name");
    assert(JoinFormFieldName(JoinFormField::Connect) == "Connect");
    assert(JoinFormFieldName(JoinFormField::Back) == "Back");

    return 0;
}
