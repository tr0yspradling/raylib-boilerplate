#pragma once

#include <string_view>

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

}  // namespace client::core
