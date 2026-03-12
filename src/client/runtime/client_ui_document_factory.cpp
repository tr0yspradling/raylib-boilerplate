#include "client/runtime/client_ui_document_factory.hpp"

#include <utility>

#include "client/runtime/client_runtime_flow.hpp"
#include "client/ui/ui_document.hpp"
#include "client/ui/ui_policy.hpp"

namespace client::runtime {

namespace {

[[nodiscard]] float WindowWidth(const ClientRuntimeContext& context) {
    return context.window.has_value() ? static_cast<float>(raylib::Window::GetWidth())
                                      : static_cast<float>(context.config.windowWidth);
}

[[nodiscard]] ui::UiDocument BuildMenuDocument(const ui::ScreenState& screenState,
                                               const ui::MenuScreenState& menuScreenState,
                                               const ui::UiInteractionState& interactionState,
                                               float windowWidth) {
    ui::UiDocument document;
    document.scene = screenState.activeScene;
    document.title = std::string{ui::policy::copy::kMenuTitle};
    document.subtitle = std::string{ui::policy::copy::kMenuSubtitle};
    document.statusMessage = screenState.statusMessage;
    document.footerHint = std::string{ui::policy::copy::kMenuFooterHint};

    const float left = windowWidth * 0.5f - (ui::policy::layout::kMenuWidth * 0.5f);
    document.widgets.reserve(menuScreenState.Actions().size());

    for (size_t index = 0; index < menuScreenState.Actions().size(); ++index) {
        const core::MenuAction action = menuScreenState.Actions()[index];
        const ui::UiWidgetId widgetId = ui::UiWidgetIdForMenuAction(action);
        document.widgets.push_back({
            .id = widgetId,
            .kind = ui::UiWidgetKind::Button,
            .bounds = ui::UiRect{
                left,
                ui::policy::layout::kMenuStartY + static_cast<float>(index) * ui::policy::layout::kMenuRowSpacing,
                ui::policy::layout::kMenuWidth,
                ui::policy::layout::kRowHeight,
            },
            .label = std::string{core::MenuActionName(action)},
            .state =
                ui::UiWidgetState{
                    .hovered = interactionState.hoveredWidget == widgetId,
                    .focused = interactionState.focusedWidget == widgetId,
                    .pressed = interactionState.pressedWidget == widgetId,
                },
        });
    }

    return document;
}

[[nodiscard]] ui::UiDocument BuildJoinDocument(const ui::ScreenState& screenState,
                                               const ui::JoinServerScreenState& joinScreenState,
                                               const ui::UiInteractionState& interactionState,
                                               float windowWidth) {
    ui::UiDocument document;
    document.scene = screenState.activeScene;
    document.title = std::string{ui::policy::copy::kJoinTitle};
    document.subtitle = std::string{ui::policy::copy::kJoinSubtitle};
    document.statusMessage = screenState.statusMessage;
    document.footerHint = joinScreenState.editing ? std::string{ui::policy::copy::kEditFooterHint}
                                                  : std::string{ui::policy::copy::kFormFooterHint};

    const float left = windowWidth * 0.5f - (ui::policy::layout::kJoinWidth * 0.5f);
    document.widgets.reserve(ui::JoinServerScreenState::kFields.size());

    for (size_t index = 0; index < ui::JoinServerScreenState::kFields.size(); ++index) {
        const core::JoinFormField field = ui::JoinServerScreenState::kFields[index];
        const ui::UiWidgetId widgetId = ui::UiWidgetIdForJoinField(field);

        std::string label = std::string{core::JoinFormFieldName(field)};
        std::string value;
        if (field == core::JoinFormField::Host) {
            value = joinScreenState.host;
        } else if (field == core::JoinFormField::Port) {
            value = joinScreenState.port;
        } else if (field == core::JoinFormField::Name) {
            value = joinScreenState.playerName;
        }

        document.widgets.push_back({
            .id = widgetId,
            .kind = ui::JoinServerScreenState::IsEditableField(field) ? ui::UiWidgetKind::TextField
                                                                      : ui::UiWidgetKind::Button,
            .bounds = ui::UiRect{
                left,
                ui::policy::layout::kJoinStartY + static_cast<float>(index) * ui::policy::layout::kMenuRowSpacing,
                ui::policy::layout::kJoinWidth,
                ui::policy::layout::kRowHeight,
            },
            .label = std::move(label),
            .value = std::move(value),
            .state =
                ui::UiWidgetState{
                    .hovered = interactionState.hoveredWidget == widgetId,
                    .focused = interactionState.focusedWidget == widgetId,
                    .pressed = interactionState.pressedWidget == widgetId,
                    .editing = joinScreenState.editing && interactionState.focusedWidget == widgetId &&
                        ui::JoinServerScreenState::IsEditableField(field),
                },
        });
    }

    return document;
}

[[nodiscard]] ui::UiDocument BuildOptionsDocument(const ui::ScreenState& screenState,
                                                  const ui::OptionsScreenState& optionsScreenState,
                                                  const ui::UiInteractionState& interactionState,
                                                  float windowWidth) {
    ui::UiDocument document;
    document.scene = screenState.activeScene;
    document.title = std::string{ui::policy::copy::kOptionsTitle};
    document.subtitle = std::string{ui::policy::copy::kOptionsSubtitle};
    document.statusMessage = screenState.statusMessage;
    document.footerHint = optionsScreenState.editing ? std::string{ui::policy::copy::kEditFooterHint}
                                                     : std::string{ui::policy::copy::kFormFooterHint};

    const float left = windowWidth * 0.5f - (ui::policy::layout::kOptionsWidth * 0.5f);
    document.widgets.reserve(ui::OptionsScreenState::kFields.size());

    for (size_t index = 0; index < ui::OptionsScreenState::kFields.size(); ++index) {
        const ui::OptionsField field = ui::OptionsScreenState::kFields[index];
        const ui::UiWidgetId widgetId = ui::UiWidgetIdForOptionsField(field);

        std::string label = std::string{ui::OptionsFieldName(field)};
        std::string value;
        if (field == ui::OptionsField::PlayerName) {
            value = optionsScreenState.playerName;
        } else if (field == ui::OptionsField::Host) {
            value = optionsScreenState.host;
        } else if (field == ui::OptionsField::Port) {
            value = optionsScreenState.port;
        } else if (field == ui::OptionsField::WindowWidth) {
            value = optionsScreenState.windowWidth;
        } else if (field == ui::OptionsField::WindowHeight) {
            value = optionsScreenState.windowHeight;
        } else if (field == ui::OptionsField::TargetFps) {
            value = optionsScreenState.targetFps;
        } else if (field == ui::OptionsField::InterpolationDelay) {
            value = optionsScreenState.interpolationDelay;
        } else if (field == ui::OptionsField::DebugOverlay) {
            label += optionsScreenState.debugOverlayDefault ? ": On" : ": Off";
        }

        document.widgets.push_back({
            .id = widgetId,
            .kind = ui::OptionsScreenState::IsEditableField(field) ? ui::UiWidgetKind::TextField
                                                                   : ui::UiWidgetKind::Button,
            .bounds = ui::UiRect{
                left,
                ui::policy::layout::kOptionsStartY +
                    static_cast<float>(index) * ui::policy::layout::kOptionsRowSpacing,
                ui::policy::layout::kOptionsWidth,
                ui::policy::layout::kRowHeight,
            },
            .label = std::move(label),
            .value = std::move(value),
            .state =
                ui::UiWidgetState{
                    .hovered = interactionState.hoveredWidget == widgetId,
                    .focused = interactionState.focusedWidget == widgetId,
                    .pressed = interactionState.pressedWidget == widgetId,
                    .editing = optionsScreenState.editing && interactionState.focusedWidget == widgetId &&
                        ui::OptionsScreenState::IsEditableField(field),
                },
        });
    }

    return document;
}

}  // namespace

void ClientUiDocumentFactory::BuildPublishedDocument(ClientRuntimeContext& context) {
    ClientRuntimeFlowController::PublishScreenState(context);

    const ui::ScreenState& screenState = context.world.get<ui::ScreenState>();
    const ui::MenuScreenState& menuScreenState = context.world.get<ui::MenuScreenState>();
    const ui::JoinServerScreenState& joinScreenState = context.world.get<ui::JoinServerScreenState>();
    const ui::OptionsScreenState& optionsScreenState = context.world.get<ui::OptionsScreenState>();
    ui::UiInteractionState& interactionState = context.world.get_mut<ui::UiInteractionState>();

    ui::UiDocument document;
    const float windowWidth = WindowWidth(context);
    if (screenState.activeScene == core::SceneKind::MainMenu) {
        if (!interactionState.focusedWidget.has_value()) {
            interactionState.focusedWidget = ui::UiWidgetIdForMenuAction(menuScreenState.SelectedAction());
        }
        document = BuildMenuDocument(screenState, menuScreenState, interactionState, windowWidth);
    } else if (screenState.activeScene == core::SceneKind::JoinServer && !screenState.joiningInProgress) {
        if (!interactionState.focusedWidget.has_value()) {
            interactionState.focusedWidget = ui::UiWidgetIdForJoinField(joinScreenState.SelectedField());
        }
        document = BuildJoinDocument(screenState, joinScreenState, interactionState, windowWidth);
    } else if (screenState.activeScene == core::SceneKind::Options) {
        if (!interactionState.focusedWidget.has_value()) {
            interactionState.focusedWidget = ui::UiWidgetIdForOptionsField(optionsScreenState.SelectedField());
        }
        document = BuildOptionsDocument(screenState, optionsScreenState, interactionState, windowWidth);
    } else {
        interactionState.hoveredWidget.reset();
        interactionState.pressedWidget.reset();
        document.scene = screenState.activeScene;
    }

    context.world.set<ui::UiDocument>(std::move(document));
}

}  // namespace client::runtime
