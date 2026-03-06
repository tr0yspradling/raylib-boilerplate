#include <cassert>
#include <string>

#include "client/core/application.hpp"

int main() {
    using namespace client::core;

    SceneManager scenes;
    RuntimeState runtime{};

    runtime.mode = RuntimeMode::Boot;
    runtime.splashCompleted = false;
    Application::UpdateScene(scenes, runtime);
    assert(scenes.ActiveScene() == SceneKind::Splash);

    runtime.splashCompleted = true;
    Application::UpdateScene(scenes, runtime);
    assert(scenes.ActiveScene() == SceneKind::MainMenu);

    runtime.mode = RuntimeMode::Menu;
    Application::UpdateScene(scenes, runtime);
    assert(scenes.ActiveScene() == SceneKind::MainMenu);

    runtime.mode = RuntimeMode::JoiningServer;
    runtime.joiningInProgress = false;
    Application::UpdateScene(scenes, runtime);
    assert(scenes.ActiveScene() == SceneKind::JoinServer);

    runtime.joiningInProgress = true;
    Application::UpdateScene(scenes, runtime);
    assert(scenes.ActiveScene() == SceneKind::Connecting);

    runtime.mode = RuntimeMode::StartingLocalServer;
    runtime.joiningInProgress = false;
    Application::UpdateScene(scenes, runtime);
    assert(scenes.ActiveScene() == SceneKind::StartingServer);

    runtime.mode = RuntimeMode::Multiplayer;
    Application::UpdateScene(scenes, runtime);
    assert(scenes.ActiveScene() == SceneKind::GameplayMultiplayer);

    runtime.mode = RuntimeMode::Singleplayer;
    Application::UpdateScene(scenes, runtime);
    assert(scenes.ActiveScene() == SceneKind::GameplaySingleplayer);

    runtime.mode = RuntimeMode::Options;
    Application::UpdateScene(scenes, runtime);
    assert(scenes.ActiveScene() == SceneKind::Options);

    runtime.mode = RuntimeMode::Disconnected;
    Application::UpdateScene(scenes, runtime);
    assert(scenes.ActiveScene() == SceneKind::Disconnected);

    runtime.mode = RuntimeMode::Menu;
    runtime.disconnectReason = "connection closed";
    Application::UpdateScene(scenes, runtime);
    assert(scenes.ActiveScene() == SceneKind::Disconnected);

    return 0;
}
