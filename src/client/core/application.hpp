#pragma once

#include "client/core/runtime_state.hpp"
#include "client/core/scene_manager.hpp"

namespace client::core {

class Application {
public:
    static void UpdateScene(SceneManager& scenes, const RuntimeState& runtime) {
        if (!runtime.disconnectReason.empty() || runtime.mode == RuntimeMode::Disconnected) {
            scenes.SwitchTo(SceneKind::Disconnected);
            return;
        }

        switch (runtime.mode) {
        case RuntimeMode::Boot:
            scenes.SwitchTo(runtime.splashCompleted ? SceneKind::MainMenu : SceneKind::Splash);
            return;
        case RuntimeMode::Menu:
            scenes.SwitchTo(SceneKind::MainMenu);
            return;
        case RuntimeMode::JoiningServer:
            scenes.SwitchTo(SceneKind::JoinServer);
            return;
        case RuntimeMode::StartingLocalServer:
            scenes.SwitchTo(SceneKind::StartingServer);
            return;
        case RuntimeMode::Multiplayer:
            scenes.SwitchTo(SceneKind::GameplayMultiplayer);
            return;
        case RuntimeMode::Singleplayer:
            scenes.SwitchTo(SceneKind::GameplaySingleplayer);
            return;
        case RuntimeMode::Options:
            scenes.SwitchTo(SceneKind::Options);
            return;
        case RuntimeMode::Disconnected:
            scenes.SwitchTo(SceneKind::Disconnected);
            return;
        }
    }
};

}  // namespace client::core
