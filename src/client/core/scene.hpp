#pragma once

#include <string_view>

namespace client::core {

enum class SceneKind {
    Splash,
    Connecting,
    Gameplay,
    Disconnected,
};

[[nodiscard]] inline std::string_view SceneName(SceneKind kind) {
    switch (kind) {
    case SceneKind::Splash:
        return "Splash";
    case SceneKind::Connecting:
        return "Connecting";
    case SceneKind::Gameplay:
        return "Gameplay";
    case SceneKind::Disconnected:
        return "Disconnected";
    }

    return "Unknown";
}

}  // namespace client::core
