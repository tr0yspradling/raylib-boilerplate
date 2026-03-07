#pragma once

#include <string_view>

#include "client/components/components.hpp"

namespace client::render {

[[nodiscard]] inline components::StatusRenderState BuildStatusRenderState(core::SceneKind activeScene,
                                                                          std::string_view runtimeStatus,
                                                                          std::string_view disconnectReason) {
    components::StatusRenderState state;
    state.activeScene = activeScene;

    switch (activeScene) {
    case core::SceneKind::Splash:
        state.title = "Authoritative Multiplayer";
        state.subtitle = "raylib runtime bootstrap";
        state.footer = "Press Enter/Space to continue";
        break;
    case core::SceneKind::Connecting:
        state.title = "Joining Dedicated Server";
        state.subtitle = runtimeStatus.empty() ? "Connecting to authoritative host..." : std::string{runtimeStatus};
        state.footer = "Press Esc to cancel";
        break;
    case core::SceneKind::StartingServer:
        state.title = "Starting Local Dedicated Server";
        state.subtitle = runtimeStatus.empty() ? "Launching sibling game_server..." : std::string{runtimeStatus};
        break;
    case core::SceneKind::GameplaySingleplayer:
        state.title = "Singleplayer Sandbox";
        state.subtitle = runtimeStatus.empty() ? "Local authoritative sandbox active" : std::string{runtimeStatus};
        state.footer = "Press Esc to return to menu";
        break;
    case core::SceneKind::Options:
        state.title = "Options";
        state.subtitle = runtimeStatus.empty() ? "Persist local client preferences" : std::string{runtimeStatus};
        state.footer = "Navigate and save preferences";
        break;
    case core::SceneKind::Disconnected:
        state.title = "Disconnected";
        state.subtitle = disconnectReason.empty() ? "Connection closed" : std::string{disconnectReason};
        state.footer = "Press Enter or Esc to return to menu";
        break;
    case core::SceneKind::MainMenu:
    case core::SceneKind::JoinServer:
    case core::SceneKind::GameplayMultiplayer:
        break;
    }

    return state;
}

}  // namespace client::render
