#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "client/core/menu_model.hpp"
#include "client/core/runtime_state.hpp"
#include "client/core/scene.hpp"

namespace client::ui {

enum class UiWidgetId : uint8_t {
    None = 0,
    MenuStartServer,
    MenuJoinServer,
    MenuSingleplayer,
    MenuOptions,
    MenuQuit,
    JoinHost,
    JoinPort,
    JoinName,
    JoinConnect,
    JoinBack,
};

struct ScreenState {
    core::RuntimeMode mode = core::RuntimeMode::Boot;
    core::SceneKind activeScene = core::SceneKind::Splash;
    bool joiningInProgress = false;
    std::string statusMessage;
    std::string disconnectReason;
};

class MenuScreenState {
public:
    static constexpr std::array<core::MenuAction, 5> kActions{
        core::MenuAction::StartServer,
        core::MenuAction::JoinServer,
        core::MenuAction::Singleplayer,
        core::MenuAction::Options,
        core::MenuAction::Quit,
    };

    [[nodiscard]] size_t SelectedIndex() const { return selectedIndex_; }
    [[nodiscard]] core::MenuAction SelectedAction() const { return kActions[selectedIndex_]; }
    [[nodiscard]] const std::array<core::MenuAction, 5>& Actions() const { return kActions; }

    void MoveNext() { selectedIndex_ = (selectedIndex_ + 1U) % kActions.size(); }

    void MovePrevious() { selectedIndex_ = selectedIndex_ == 0U ? (kActions.size() - 1U) : (selectedIndex_ - 1U); }

    void SetSelectedIndex(size_t index) {
        if (index < kActions.size()) {
            selectedIndex_ = index;
        }
    }

private:
    size_t selectedIndex_ = 0U;
};

class JoinServerScreenState {
public:
    static constexpr std::array<core::JoinFormField, 5> kFields{
        core::JoinFormField::Host,
        core::JoinFormField::Port,
        core::JoinFormField::Name,
        core::JoinFormField::Connect,
        core::JoinFormField::Back,
    };

    void ResetFromDefaults(const std::string& defaultHost, uint16_t defaultPort, const std::string& defaultName) {
        host = defaultHost;
        port = std::to_string(defaultPort);
        playerName = defaultName;
        selectedIndex_ = 0U;
        editing = false;
    }

    [[nodiscard]] size_t SelectedIndex() const { return selectedIndex_; }
    [[nodiscard]] core::JoinFormField SelectedField() const { return kFields[selectedIndex_]; }

    void MoveNext() { selectedIndex_ = (selectedIndex_ + 1U) % kFields.size(); }

    void MovePrevious() { selectedIndex_ = selectedIndex_ == 0U ? (kFields.size() - 1U) : (selectedIndex_ - 1U); }

    void SetSelectedIndex(size_t index) {
        if (index < kFields.size()) {
            selectedIndex_ = index;
        }
    }

    [[nodiscard]] bool SelectedFieldIsEditable() const {
        return IsEditableField(SelectedField());
    }

    [[nodiscard]] static bool IsEditableField(core::JoinFormField field) {
        return field == core::JoinFormField::Host || field == core::JoinFormField::Port ||
            field == core::JoinFormField::Name;
    }

    std::string host = "127.0.0.1";
    std::string port = "27020";
    std::string playerName = "player";
    bool editing = false;

private:
    size_t selectedIndex_ = 0U;
};

struct UiInputState {
    float mouseX = 0.0f;
    float mouseY = 0.0f;
    bool mouseMoved = false;
    bool primaryPressed = false;
    bool navigateUpPressed = false;
    bool navigateDownPressed = false;
    bool acceptPressed = false;
    bool backPressed = false;
    bool backspacePressed = false;
    std::string textInput;
};

struct UiInteractionState {
    std::optional<UiWidgetId> hoveredWidget;
    std::optional<UiWidgetId> focusedWidget;
    std::optional<UiWidgetId> pressedWidget;
};

enum class UiCommandType : uint8_t {
    None = 0,
    ActivateMenuAction,
    StartJoinFieldEdit,
    StopJoinFieldEdit,
    SubmitJoin,
    BackToMenu,
};

struct UiCommand {
    UiCommandType type = UiCommandType::None;
    core::MenuAction menuAction = core::MenuAction::None;
    core::JoinFormField joinField = core::JoinFormField::Host;
};

struct UiCommandQueue {
    std::vector<UiCommand> commands;

    void Clear() { commands.clear(); }

    void Push(UiCommand command) { commands.push_back(command); }
};

[[nodiscard]] inline UiWidgetId UiWidgetIdForMenuAction(core::MenuAction action) {
    switch (action) {
    case core::MenuAction::StartServer:
        return UiWidgetId::MenuStartServer;
    case core::MenuAction::JoinServer:
        return UiWidgetId::MenuJoinServer;
    case core::MenuAction::Singleplayer:
        return UiWidgetId::MenuSingleplayer;
    case core::MenuAction::Options:
        return UiWidgetId::MenuOptions;
    case core::MenuAction::Quit:
        return UiWidgetId::MenuQuit;
    case core::MenuAction::None:
        return UiWidgetId::None;
    }

    return UiWidgetId::None;
}

[[nodiscard]] inline core::MenuAction MenuActionForWidgetId(UiWidgetId widgetId) {
    switch (widgetId) {
    case UiWidgetId::MenuStartServer:
        return core::MenuAction::StartServer;
    case UiWidgetId::MenuJoinServer:
        return core::MenuAction::JoinServer;
    case UiWidgetId::MenuSingleplayer:
        return core::MenuAction::Singleplayer;
    case UiWidgetId::MenuOptions:
        return core::MenuAction::Options;
    case UiWidgetId::MenuQuit:
        return core::MenuAction::Quit;
    case UiWidgetId::None:
    case UiWidgetId::JoinHost:
    case UiWidgetId::JoinPort:
    case UiWidgetId::JoinName:
    case UiWidgetId::JoinConnect:
    case UiWidgetId::JoinBack:
        return core::MenuAction::None;
    }

    return core::MenuAction::None;
}

[[nodiscard]] inline UiWidgetId UiWidgetIdForJoinField(core::JoinFormField field) {
    switch (field) {
    case core::JoinFormField::Host:
        return UiWidgetId::JoinHost;
    case core::JoinFormField::Port:
        return UiWidgetId::JoinPort;
    case core::JoinFormField::Name:
        return UiWidgetId::JoinName;
    case core::JoinFormField::Connect:
        return UiWidgetId::JoinConnect;
    case core::JoinFormField::Back:
        return UiWidgetId::JoinBack;
    }

    return UiWidgetId::None;
}

[[nodiscard]] inline core::JoinFormField JoinFieldForWidgetId(UiWidgetId widgetId) {
    switch (widgetId) {
    case UiWidgetId::JoinHost:
        return core::JoinFormField::Host;
    case UiWidgetId::JoinPort:
        return core::JoinFormField::Port;
    case UiWidgetId::JoinName:
        return core::JoinFormField::Name;
    case UiWidgetId::JoinConnect:
        return core::JoinFormField::Connect;
    case UiWidgetId::JoinBack:
        return core::JoinFormField::Back;
    case UiWidgetId::None:
    case UiWidgetId::MenuStartServer:
    case UiWidgetId::MenuJoinServer:
    case UiWidgetId::MenuSingleplayer:
    case UiWidgetId::MenuOptions:
    case UiWidgetId::MenuQuit:
        return core::JoinFormField::Host;
    }

    return core::JoinFormField::Host;
}

}  // namespace client::ui
