#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "client/core/client_config.hpp"
#include "client/core/client_config_policy.hpp"
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
    OptionsPlayerName,
    OptionsHost,
    OptionsPort,
    OptionsWindowWidth,
    OptionsWindowHeight,
    OptionsTargetFps,
    OptionsInterpolationDelay,
    OptionsDebugOverlay,
    OptionsSave,
    OptionsBack,
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

    std::string host = std::string{client::core::policy::kDefaultServerHost};
    std::string port = std::to_string(client::core::policy::kDefaultServerPort);
    std::string playerName = std::string{client::core::policy::kDefaultPlayerName};
    bool editing = false;

private:
    size_t selectedIndex_ = 0U;
};

enum class OptionsField : uint8_t {
    PlayerName = 0,
    Host,
    Port,
    WindowWidth,
    WindowHeight,
    TargetFps,
    InterpolationDelay,
    DebugOverlay,
    Save,
    Back,
};

class OptionsScreenState {
public:
    static constexpr std::array<OptionsField, 10> kFields{
        OptionsField::PlayerName,
        OptionsField::Host,
        OptionsField::Port,
        OptionsField::WindowWidth,
        OptionsField::WindowHeight,
        OptionsField::TargetFps,
        OptionsField::InterpolationDelay,
        OptionsField::DebugOverlay,
        OptionsField::Save,
        OptionsField::Back,
    };

    void ResetFromConfig(const client::ClientConfig& config) {
        playerName = config.playerName;
        host = config.serverHost;
        port = std::to_string(config.serverPort);
        windowWidth = std::to_string(config.windowWidth);
        windowHeight = std::to_string(config.windowHeight);
        targetFps = std::to_string(config.targetFps);
        interpolationDelay = std::to_string(config.interpolationDelayTicks);
        debugOverlayDefault = config.debugOverlayDefault;
        selectedIndex_ = 0U;
        editing = false;
    }

    [[nodiscard]] size_t SelectedIndex() const { return selectedIndex_; }
    [[nodiscard]] OptionsField SelectedField() const { return kFields[selectedIndex_]; }

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

    [[nodiscard]] static bool IsEditableField(OptionsField field) {
        return field == OptionsField::PlayerName || field == OptionsField::Host || field == OptionsField::Port ||
            field == OptionsField::WindowWidth || field == OptionsField::WindowHeight ||
            field == OptionsField::TargetFps || field == OptionsField::InterpolationDelay;
    }

    [[nodiscard]] static bool IsNumericField(OptionsField field) {
        return field == OptionsField::Port || field == OptionsField::WindowWidth || field == OptionsField::WindowHeight ||
            field == OptionsField::TargetFps || field == OptionsField::InterpolationDelay;
    }

    std::string playerName = std::string{client::core::policy::kDefaultPlayerName};
    std::string host = std::string{client::core::policy::kDefaultServerHost};
    std::string port = std::to_string(client::core::policy::kDefaultServerPort);
    std::string windowWidth = std::to_string(client::core::policy::kDefaultWindowWidth);
    std::string windowHeight = std::to_string(client::core::policy::kDefaultWindowHeight);
    std::string targetFps = std::to_string(client::core::policy::kDefaultTargetFps);
    std::string interpolationDelay = std::to_string(client::core::policy::kDefaultInterpolationDelayTicks);
    bool debugOverlayDefault = client::core::policy::kDefaultDebugOverlayEnabled;
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
    StartOptionsFieldEdit,
    StopOptionsFieldEdit,
    ToggleOptionsDebugOverlay,
    SaveOptions,
    BackToMenu,
};

struct UiCommand {
    UiCommandType type = UiCommandType::None;
    core::MenuAction menuAction = core::MenuAction::None;
    core::JoinFormField joinField = core::JoinFormField::Host;
    OptionsField optionsField = OptionsField::PlayerName;
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
    case UiWidgetId::OptionsPlayerName:
    case UiWidgetId::OptionsHost:
    case UiWidgetId::OptionsPort:
    case UiWidgetId::OptionsWindowWidth:
    case UiWidgetId::OptionsWindowHeight:
    case UiWidgetId::OptionsTargetFps:
    case UiWidgetId::OptionsInterpolationDelay:
    case UiWidgetId::OptionsDebugOverlay:
    case UiWidgetId::OptionsSave:
    case UiWidgetId::OptionsBack:
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
    case UiWidgetId::OptionsPlayerName:
    case UiWidgetId::OptionsHost:
    case UiWidgetId::OptionsPort:
    case UiWidgetId::OptionsWindowWidth:
    case UiWidgetId::OptionsWindowHeight:
    case UiWidgetId::OptionsTargetFps:
    case UiWidgetId::OptionsInterpolationDelay:
    case UiWidgetId::OptionsDebugOverlay:
    case UiWidgetId::OptionsSave:
    case UiWidgetId::OptionsBack:
        return core::JoinFormField::Host;
    }

    return core::JoinFormField::Host;
}

[[nodiscard]] inline UiWidgetId UiWidgetIdForOptionsField(OptionsField field) {
    switch (field) {
    case OptionsField::PlayerName:
        return UiWidgetId::OptionsPlayerName;
    case OptionsField::Host:
        return UiWidgetId::OptionsHost;
    case OptionsField::Port:
        return UiWidgetId::OptionsPort;
    case OptionsField::WindowWidth:
        return UiWidgetId::OptionsWindowWidth;
    case OptionsField::WindowHeight:
        return UiWidgetId::OptionsWindowHeight;
    case OptionsField::TargetFps:
        return UiWidgetId::OptionsTargetFps;
    case OptionsField::InterpolationDelay:
        return UiWidgetId::OptionsInterpolationDelay;
    case OptionsField::DebugOverlay:
        return UiWidgetId::OptionsDebugOverlay;
    case OptionsField::Save:
        return UiWidgetId::OptionsSave;
    case OptionsField::Back:
        return UiWidgetId::OptionsBack;
    }

    return UiWidgetId::None;
}

[[nodiscard]] inline OptionsField OptionsFieldForWidgetId(UiWidgetId widgetId) {
    switch (widgetId) {
    case UiWidgetId::OptionsPlayerName:
        return OptionsField::PlayerName;
    case UiWidgetId::OptionsHost:
        return OptionsField::Host;
    case UiWidgetId::OptionsPort:
        return OptionsField::Port;
    case UiWidgetId::OptionsWindowWidth:
        return OptionsField::WindowWidth;
    case UiWidgetId::OptionsWindowHeight:
        return OptionsField::WindowHeight;
    case UiWidgetId::OptionsTargetFps:
        return OptionsField::TargetFps;
    case UiWidgetId::OptionsInterpolationDelay:
        return OptionsField::InterpolationDelay;
    case UiWidgetId::OptionsDebugOverlay:
        return OptionsField::DebugOverlay;
    case UiWidgetId::OptionsSave:
        return OptionsField::Save;
    case UiWidgetId::OptionsBack:
        return OptionsField::Back;
    case UiWidgetId::None:
    case UiWidgetId::MenuStartServer:
    case UiWidgetId::MenuJoinServer:
    case UiWidgetId::MenuSingleplayer:
    case UiWidgetId::MenuOptions:
    case UiWidgetId::MenuQuit:
    case UiWidgetId::JoinHost:
    case UiWidgetId::JoinPort:
    case UiWidgetId::JoinName:
    case UiWidgetId::JoinConnect:
    case UiWidgetId::JoinBack:
        return OptionsField::PlayerName;
    }

    return OptionsField::PlayerName;
}

[[nodiscard]] inline std::string_view OptionsFieldName(OptionsField field) {
    switch (field) {
    case OptionsField::PlayerName:
        return "Player Name";
    case OptionsField::Host:
        return "Default Host";
    case OptionsField::Port:
        return "Default Port";
    case OptionsField::WindowWidth:
        return "Window Width";
    case OptionsField::WindowHeight:
        return "Window Height";
    case OptionsField::TargetFps:
        return "Target FPS";
    case OptionsField::InterpolationDelay:
        return "Interpolation Delay";
    case OptionsField::DebugOverlay:
        return "Debug Overlay";
    case OptionsField::Save:
        return "Save";
    case OptionsField::Back:
        return "Back";
    }

    return "Unknown";
}

}  // namespace client::ui
