#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace client::core {

enum class MenuAction : uint8_t {
    None = 0,
    StartServer,
    JoinServer,
    Singleplayer,
    Options,
    Quit,
};

[[nodiscard]] inline std::string_view MenuActionName(MenuAction action) {
    switch (action) {
    case MenuAction::None:
        return "None";
    case MenuAction::StartServer:
        return "Start Server";
    case MenuAction::JoinServer:
        return "Join Server";
    case MenuAction::Singleplayer:
        return "Singleplayer";
    case MenuAction::Options:
        return "Options";
    case MenuAction::Quit:
        return "Quit";
    }

    return "Unknown";
}

class MenuSelectionState {
public:
    static constexpr std::array<MenuAction, 5> kDefaultActions{
        MenuAction::StartServer,
        MenuAction::JoinServer,
        MenuAction::Singleplayer,
        MenuAction::Options,
        MenuAction::Quit,
    };

    [[nodiscard]] size_t SelectedIndex() const { return selectedIndex_; }
    [[nodiscard]] MenuAction SelectedAction() const { return kDefaultActions[selectedIndex_]; }
    [[nodiscard]] const std::array<MenuAction, 5>& Actions() const { return kDefaultActions; }

    void MoveNext() { selectedIndex_ = (selectedIndex_ + 1U) % kDefaultActions.size(); }

    void MovePrevious() {
        selectedIndex_ = selectedIndex_ == 0U ? (kDefaultActions.size() - 1U) : (selectedIndex_ - 1U);
    }

    void SetSelectedIndex(size_t index) {
        if (index >= kDefaultActions.size()) {
            return;
        }
        selectedIndex_ = index;
    }

private:
    size_t selectedIndex_ = 0U;
};

}  // namespace client::core
