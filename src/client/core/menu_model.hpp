#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
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

enum class JoinFormField : uint8_t {
    Host = 0,
    Port,
    Name,
    Connect,
    Back,
};

[[nodiscard]] inline std::string_view JoinFormFieldName(JoinFormField field) {
    switch (field) {
    case JoinFormField::Host:
        return "Host";
    case JoinFormField::Port:
        return "Port";
    case JoinFormField::Name:
        return "Name";
    case JoinFormField::Connect:
        return "Connect";
    case JoinFormField::Back:
        return "Back";
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

class JoinServerFormState {
public:
    static constexpr std::array<JoinFormField, 5> kFields{
        JoinFormField::Host,
        JoinFormField::Port,
        JoinFormField::Name,
        JoinFormField::Connect,
        JoinFormField::Back,
    };

    void ResetFromDefaults(const std::string& defaultHost, uint16_t defaultPort, const std::string& defaultName) {
        host = defaultHost;
        port = std::to_string(defaultPort);
        playerName = defaultName;
        selectedIndex = 0U;
        editing = false;
    }

    [[nodiscard]] size_t SelectedIndex() const { return selectedIndex; }
    [[nodiscard]] JoinFormField SelectedField() const { return kFields[selectedIndex]; }

    void MoveNext() { selectedIndex = (selectedIndex + 1U) % kFields.size(); }

    void MovePrevious() { selectedIndex = selectedIndex == 0U ? (kFields.size() - 1U) : (selectedIndex - 1U); }

    [[nodiscard]] bool SelectedFieldIsEditable() const {
        const JoinFormField field = SelectedField();
        return field == JoinFormField::Host || field == JoinFormField::Port || field == JoinFormField::Name;
    }

    std::string host = "127.0.0.1";
    std::string port = "27020";
    std::string playerName = "player";
    bool editing = false;

private:
    size_t selectedIndex = 0U;
};

}  // namespace client::core
