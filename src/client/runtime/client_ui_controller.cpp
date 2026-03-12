#include "client/runtime/client_ui_controller.hpp"

#include <optional>
#include <string>

#include "client/runtime/client_runtime_policy.hpp"
#include "client/ui/ui_document.hpp"

namespace client::runtime {

namespace {

void ApplyJoinFormTextInput(ui::JoinServerScreenState& joinScreenState, const ui::UiInputState& inputState) {
    if (!joinScreenState.editing || !joinScreenState.SelectedFieldIsEditable()) {
        return;
    }

    std::string* target = nullptr;
    const core::JoinFormField field = joinScreenState.SelectedField();
    switch (field) {
    case core::JoinFormField::Host:
        target = &joinScreenState.host;
        break;
    case core::JoinFormField::Port:
        target = &joinScreenState.port;
        break;
    case core::JoinFormField::Name:
        target = &joinScreenState.playerName;
        break;
    case core::JoinFormField::Connect:
    case core::JoinFormField::Back:
        return;
    }

    for (const char character : inputState.textInput) {
        if (field == core::JoinFormField::Port) {
            if (character >= '0' && character <= '9' && target->size() < policy::kNumericFieldMaxDigits) {
                target->push_back(character);
            }
            continue;
        }

        const size_t maxSize =
            field == core::JoinFormField::Host ? policy::kHostFieldMaxLength : policy::kPlayerNameFieldMaxLength;
        if (target->size() < maxSize) {
            target->push_back(character);
        }
    }

    if (inputState.backspacePressed && !target->empty()) {
        target->pop_back();
    }
}

void ApplyOptionsTextInput(ui::OptionsScreenState& optionsScreenState, const ui::UiInputState& inputState) {
    if (!optionsScreenState.editing || !optionsScreenState.SelectedFieldIsEditable()) {
        return;
    }

    std::string* target = nullptr;
    const ui::OptionsField field = optionsScreenState.SelectedField();
    switch (field) {
    case ui::OptionsField::PlayerName:
        target = &optionsScreenState.playerName;
        break;
    case ui::OptionsField::Host:
        target = &optionsScreenState.host;
        break;
    case ui::OptionsField::Port:
        target = &optionsScreenState.port;
        break;
    case ui::OptionsField::WindowWidth:
        target = &optionsScreenState.windowWidth;
        break;
    case ui::OptionsField::WindowHeight:
        target = &optionsScreenState.windowHeight;
        break;
    case ui::OptionsField::TargetFps:
        target = &optionsScreenState.targetFps;
        break;
    case ui::OptionsField::InterpolationDelay:
        target = &optionsScreenState.interpolationDelay;
        break;
    case ui::OptionsField::DebugOverlay:
    case ui::OptionsField::Save:
    case ui::OptionsField::Back:
        return;
    }

    for (const char character : inputState.textInput) {
        if (ui::OptionsScreenState::IsNumericField(field)) {
            if (character >= '0' && character <= '9' && target->size() < policy::kNumericFieldMaxDigits) {
                target->push_back(character);
            }
            continue;
        }

        const size_t maxSize =
            field == ui::OptionsField::Host ? policy::kHostFieldMaxLength : policy::kPlayerNameFieldMaxLength;
        if (target->size() < maxSize) {
            target->push_back(character);
        }
    }

    if (inputState.backspacePressed && !target->empty()) {
        target->pop_back();
    }
}

void HandleUiPointerFocus(ClientRuntimeContext& context, const ui::UiDocument& document, const ui::UiInputState& inputState) {
    ui::UiInteractionState& interactionState = context.world.get_mut<ui::UiInteractionState>();
    const std::optional<ui::UiWidgetId> hoveredWidget = document.FindWidgetAt(inputState.mouseX, inputState.mouseY);
    interactionState.hoveredWidget = hoveredWidget;

    if (!hoveredWidget.has_value() || !inputState.mouseMoved) {
        return;
    }

    const ui::ScreenState& screenState = context.world.get<ui::ScreenState>();
    if (screenState.activeScene == core::SceneKind::JoinServer &&
        context.world.get<ui::JoinServerScreenState>().editing) {
        return;
    }
    if (screenState.activeScene == core::SceneKind::Options &&
        context.world.get<ui::OptionsScreenState>().editing) {
        return;
    }

    interactionState.focusedWidget = hoveredWidget;
    if (screenState.activeScene == core::SceneKind::MainMenu) {
        const core::MenuAction action = ui::MenuActionForWidgetId(*hoveredWidget);
        if (action == core::MenuAction::None) {
            return;
        }
        ui::MenuScreenState& menuScreenState = context.world.get_mut<ui::MenuScreenState>();
        const auto& actions = menuScreenState.Actions();
        for (size_t index = 0; index < actions.size(); ++index) {
            if (actions[index] == action) {
                menuScreenState.SetSelectedIndex(index);
                break;
            }
        }
        return;
    }

    if (screenState.activeScene == core::SceneKind::JoinServer) {
        const core::JoinFormField field = ui::JoinFieldForWidgetId(*hoveredWidget);
        ui::JoinServerScreenState& joinScreenState = context.world.get_mut<ui::JoinServerScreenState>();
        const auto& fields = ui::JoinServerScreenState::kFields;
        for (size_t index = 0; index < fields.size(); ++index) {
            if (fields[index] == field) {
                joinScreenState.SetSelectedIndex(index);
                break;
            }
        }
        return;
    }

    if (screenState.activeScene == core::SceneKind::Options) {
        const ui::OptionsField field = ui::OptionsFieldForWidgetId(*hoveredWidget);
        ui::OptionsScreenState& optionsScreenState = context.world.get_mut<ui::OptionsScreenState>();
        const auto& fields = ui::OptionsScreenState::kFields;
        for (size_t index = 0; index < fields.size(); ++index) {
            if (fields[index] == field) {
                optionsScreenState.SetSelectedIndex(index);
                break;
            }
        }
    }
}

void HandleMenuInteraction(ClientRuntimeContext& context, const ui::UiInputState& inputState) {
    ui::MenuScreenState& menuScreenState = context.world.get_mut<ui::MenuScreenState>();
    ui::UiInteractionState& interactionState = context.world.get_mut<ui::UiInteractionState>();
    ui::UiCommandQueue& commandQueue = context.world.get_mut<ui::UiCommandQueue>();
    interactionState.pressedWidget.reset();

    if (inputState.navigateDownPressed) {
        menuScreenState.MoveNext();
        interactionState.focusedWidget = ui::UiWidgetIdForMenuAction(menuScreenState.SelectedAction());
    }
    if (inputState.navigateUpPressed) {
        menuScreenState.MovePrevious();
        interactionState.focusedWidget = ui::UiWidgetIdForMenuAction(menuScreenState.SelectedAction());
    }

    std::optional<ui::UiWidgetId> activateWidget;
    if (inputState.primaryPressed && interactionState.hoveredWidget.has_value()) {
        activateWidget = interactionState.hoveredWidget;
    } else if (inputState.acceptPressed) {
        activateWidget = interactionState.focusedWidget;
    }

    if (!activateWidget.has_value()) {
        return;
    }

    const core::MenuAction action = ui::MenuActionForWidgetId(*activateWidget);
    if (action == core::MenuAction::None) {
        return;
    }

    interactionState.pressedWidget = activateWidget;
    commandQueue.Push({
        .type = ui::UiCommandType::ActivateMenuAction,
        .menuAction = action,
    });
}

void HandleJoinFormInteraction(ClientRuntimeContext& context, const ui::UiInputState& inputState) {
    ui::JoinServerScreenState& joinScreenState = context.world.get_mut<ui::JoinServerScreenState>();
    ui::UiInteractionState& interactionState = context.world.get_mut<ui::UiInteractionState>();
    ui::UiCommandQueue& commandQueue = context.world.get_mut<ui::UiCommandQueue>();
    interactionState.pressedWidget.reset();

    ApplyJoinFormTextInput(joinScreenState, inputState);

    if (!joinScreenState.editing) {
        if (inputState.navigateDownPressed) {
            joinScreenState.MoveNext();
            interactionState.focusedWidget = ui::UiWidgetIdForJoinField(joinScreenState.SelectedField());
        }
        if (inputState.navigateUpPressed) {
            joinScreenState.MovePrevious();
            interactionState.focusedWidget = ui::UiWidgetIdForJoinField(joinScreenState.SelectedField());
        }
    }

    if (joinScreenState.editing && (inputState.acceptPressed || inputState.backPressed)) {
        commandQueue.Push({
            .type = ui::UiCommandType::StopJoinFieldEdit,
            .joinField = joinScreenState.SelectedField(),
        });
        return;
    }

    if (!joinScreenState.editing && inputState.backPressed) {
        commandQueue.Push({.type = ui::UiCommandType::BackToMenu});
        return;
    }

    std::optional<ui::UiWidgetId> activateWidget;
    if (inputState.primaryPressed && interactionState.hoveredWidget.has_value()) {
        activateWidget = interactionState.hoveredWidget;
    } else if (inputState.acceptPressed) {
        activateWidget = interactionState.focusedWidget;
    }

    if (!activateWidget.has_value()) {
        return;
    }

    const core::JoinFormField field = ui::JoinFieldForWidgetId(*activateWidget);
    interactionState.pressedWidget = activateWidget;

    if (ui::JoinServerScreenState::IsEditableField(field)) {
        commandQueue.Push({
            .type = ui::UiCommandType::StartJoinFieldEdit,
            .joinField = field,
        });
        return;
    }

    if (field == core::JoinFormField::Connect) {
        commandQueue.Push({.type = ui::UiCommandType::SubmitJoin});
        return;
    }

    if (field == core::JoinFormField::Back) {
        commandQueue.Push({.type = ui::UiCommandType::BackToMenu});
    }
}

void HandleOptionsInteraction(ClientRuntimeContext& context, const ui::UiInputState& inputState) {
    ui::OptionsScreenState& optionsScreenState = context.world.get_mut<ui::OptionsScreenState>();
    ui::UiInteractionState& interactionState = context.world.get_mut<ui::UiInteractionState>();
    ui::UiCommandQueue& commandQueue = context.world.get_mut<ui::UiCommandQueue>();
    interactionState.pressedWidget.reset();

    ApplyOptionsTextInput(optionsScreenState, inputState);

    if (!optionsScreenState.editing) {
        if (inputState.navigateDownPressed) {
            optionsScreenState.MoveNext();
            interactionState.focusedWidget = ui::UiWidgetIdForOptionsField(optionsScreenState.SelectedField());
        }
        if (inputState.navigateUpPressed) {
            optionsScreenState.MovePrevious();
            interactionState.focusedWidget = ui::UiWidgetIdForOptionsField(optionsScreenState.SelectedField());
        }
    }

    if (optionsScreenState.editing && (inputState.acceptPressed || inputState.backPressed)) {
        commandQueue.Push({
            .type = ui::UiCommandType::StopOptionsFieldEdit,
            .optionsField = optionsScreenState.SelectedField(),
        });
        return;
    }

    if (!optionsScreenState.editing && inputState.backPressed) {
        commandQueue.Push({.type = ui::UiCommandType::BackToMenu});
        return;
    }

    std::optional<ui::UiWidgetId> activateWidget;
    if (inputState.primaryPressed && interactionState.hoveredWidget.has_value()) {
        activateWidget = interactionState.hoveredWidget;
    } else if (inputState.acceptPressed) {
        activateWidget = interactionState.focusedWidget;
    }

    if (!activateWidget.has_value()) {
        return;
    }

    const ui::OptionsField field = ui::OptionsFieldForWidgetId(*activateWidget);
    interactionState.pressedWidget = activateWidget;

    if (ui::OptionsScreenState::IsEditableField(field)) {
        commandQueue.Push({
            .type = ui::UiCommandType::StartOptionsFieldEdit,
            .optionsField = field,
        });
        return;
    }

    if (field == ui::OptionsField::DebugOverlay) {
        commandQueue.Push({
            .type = ui::UiCommandType::ToggleOptionsDebugOverlay,
            .optionsField = field,
        });
        return;
    }

    if (field == ui::OptionsField::Save) {
        commandQueue.Push({
            .type = ui::UiCommandType::SaveOptions,
            .optionsField = field,
        });
        return;
    }

    if (field == ui::OptionsField::Back) {
        commandQueue.Push({.type = ui::UiCommandType::BackToMenu});
    }
}

}  // namespace

void ClientUiController::HandleUiInteraction(ClientRuntimeContext& context) {
    const ui::ScreenState& screenState = context.world.get<ui::ScreenState>();
    const ui::UiInputState& inputState = context.world.get<ui::UiInputState>();
    const ui::UiDocument& document = context.world.get<ui::UiDocument>();

    if (document.widgets.empty()) {
        ui::UiInteractionState& interactionState = context.world.get_mut<ui::UiInteractionState>();
        interactionState.hoveredWidget.reset();
        interactionState.pressedWidget.reset();
        return;
    }

    HandleUiPointerFocus(context, document, inputState);

    if (screenState.activeScene == core::SceneKind::MainMenu) {
        HandleMenuInteraction(context, inputState);
        return;
    }

    if (screenState.activeScene == core::SceneKind::JoinServer && !screenState.joiningInProgress) {
        HandleJoinFormInteraction(context, inputState);
        return;
    }

    if (screenState.activeScene == core::SceneKind::Options) {
        HandleOptionsInteraction(context, inputState);
    }
}

}  // namespace client::runtime
