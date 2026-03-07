#pragma once

#include "client/core/runtime_state.hpp"
#include "client/core/scene_manager.hpp"

namespace client::core {

class Application {
public:
    static void UpdateScene(SceneManager& scenes, const RuntimeState& runtime) {
        scenes.SwitchTo(SceneForRuntime(runtime));
    }
};

}  // namespace client::core
