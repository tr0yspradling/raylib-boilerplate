#pragma once

#include <string_view>

#include "client/components/components.hpp"
#include "client/render/render_policy.hpp"

namespace client::render {

[[nodiscard]] inline components::StatusRenderState BuildStatusRenderState(core::SceneKind activeScene,
                                                                          std::string_view runtimeStatus,
                                                                          std::string_view disconnectReason) {
    components::StatusRenderState state;
    state.activeScene = activeScene;

    switch (activeScene) {
    case core::SceneKind::Splash:
        state.title = std::string{policy::status_copy::kSplashTitle};
        state.subtitle = std::string{policy::status_copy::kSplashSubtitle};
        state.footer = std::string{policy::status_copy::kSplashFooter};
        break;
    case core::SceneKind::Connecting:
        state.title = std::string{policy::status_copy::kConnectingTitle};
        state.subtitle = runtimeStatus.empty() ? std::string{policy::status_copy::kConnectingSubtitle}
                                               : std::string{runtimeStatus};
        state.footer = std::string{policy::status_copy::kConnectingFooter};
        break;
    case core::SceneKind::StartingServer:
        state.title = std::string{policy::status_copy::kStartingServerTitle};
        state.subtitle = runtimeStatus.empty() ? std::string{policy::status_copy::kStartingServerSubtitle}
                                               : std::string{runtimeStatus};
        break;
    case core::SceneKind::GameplaySingleplayer:
        state.title = std::string{policy::status_copy::kSingleplayerTitle};
        state.subtitle = runtimeStatus.empty() ? std::string{policy::status_copy::kSingleplayerSubtitle}
                                               : std::string{runtimeStatus};
        state.footer = std::string{policy::status_copy::kSingleplayerFooter};
        break;
    case core::SceneKind::Options:
        state.title = std::string{policy::status_copy::kOptionsTitle};
        state.subtitle = runtimeStatus.empty() ? std::string{policy::status_copy::kOptionsSubtitle}
                                               : std::string{runtimeStatus};
        state.footer = std::string{policy::status_copy::kOptionsFooter};
        break;
    case core::SceneKind::Disconnected:
        state.title = std::string{policy::status_copy::kDisconnectedTitle};
        state.subtitle = disconnectReason.empty() ? std::string{policy::status_copy::kDisconnectedSubtitle}
                                                  : std::string{disconnectReason};
        state.footer = std::string{policy::status_copy::kDisconnectedFooter};
        break;
    case core::SceneKind::MainMenu:
    case core::SceneKind::JoinServer:
    case core::SceneKind::GameplayMultiplayer:
        break;
    }

    return state;
}

}  // namespace client::render
