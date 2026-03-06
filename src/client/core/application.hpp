#pragma once

#include <string>

#include "client/core/scene_manager.hpp"

namespace client::core {

class Application {
public:
    static void UpdateScene(SceneManager& scenes, bool connecting, bool connected, bool welcomed,
                            const std::string& disconnectReason) {
        if (!disconnectReason.empty() || (!connected && !connecting)) {
            scenes.SwitchTo(SceneKind::Disconnected);
            return;
        }

        if (connecting || (connected && !welcomed)) {
            scenes.SwitchTo(SceneKind::Connecting);
            return;
        }

        scenes.SwitchTo(SceneKind::Gameplay);
    }
};

}  // namespace client::core
