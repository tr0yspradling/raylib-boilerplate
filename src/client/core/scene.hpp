#pragma once

#include <string_view>

#include "client/core/runtime_state.hpp"

namespace client::core {

enum class SceneKind {
    Splash,
    MainMenu,
    JoinServer,
    StartingServer,
    Connecting,
    GameplayMultiplayer,
    GameplaySingleplayer,
    Options,
    Disconnected,
};

[[nodiscard]] inline std::string_view SceneName(SceneKind kind) {
    switch (kind) {
    case SceneKind::Splash:
        return "Splash";
    case SceneKind::MainMenu:
        return "Main Menu";
    case SceneKind::JoinServer:
        return "Join Server";
    case SceneKind::StartingServer:
        return "Starting Server";
    case SceneKind::Connecting:
        return "Connecting";
    case SceneKind::GameplayMultiplayer:
        return "Gameplay (Multiplayer)";
    case SceneKind::GameplaySingleplayer:
        return "Gameplay (Singleplayer)";
    case SceneKind::Options:
        return "Options";
    case SceneKind::Disconnected:
        return "Disconnected";
    }

    return "Unknown";
}

[[nodiscard]] inline SceneKind SceneForRuntime(const RuntimeState& runtime) {
    if (!runtime.disconnectReason.empty() || runtime.mode == RuntimeMode::Disconnected) {
        return SceneKind::Disconnected;
    }

    switch (runtime.mode) {
        case RuntimeMode::Boot:
            return runtime.splashCompleted ? SceneKind::MainMenu : SceneKind::Splash;
        case RuntimeMode::Menu:
            return SceneKind::MainMenu;
        case RuntimeMode::JoiningServer:
            return runtime.joiningInProgress ? SceneKind::Connecting : SceneKind::JoinServer;
        case RuntimeMode::StartingLocalServer:
            return SceneKind::StartingServer;
        case RuntimeMode::Multiplayer:
            return SceneKind::GameplayMultiplayer;
        case RuntimeMode::Singleplayer:
            return SceneKind::GameplaySingleplayer;
        case RuntimeMode::Options:
            return SceneKind::Options;
        case RuntimeMode::Disconnected:
            return SceneKind::Disconnected;
    }

    return SceneKind::Disconnected;
}

}  // namespace client::core
