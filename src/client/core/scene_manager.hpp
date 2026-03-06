#pragma once

#include "client/core/scene.hpp"

namespace client::core {

class SceneManager {
public:
    void SwitchTo(SceneKind scene) { activeScene_ = scene; }

    [[nodiscard]] SceneKind ActiveScene() const { return activeScene_; }
    [[nodiscard]] std::string_view ActiveSceneName() const { return SceneName(activeScene_); }

private:
    SceneKind activeScene_ = SceneKind::Splash;
};

}  // namespace client::core
