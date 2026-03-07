#include <cassert>
#include <string>

#include "client/core/scene.hpp"

int main() {
    using namespace client::core;

    RuntimeState runtime{};

    runtime.mode = RuntimeMode::Boot;
    runtime.splashCompleted = false;
    assert(SceneForRuntime(runtime) == SceneKind::Splash);

    runtime.splashCompleted = true;
    assert(SceneForRuntime(runtime) == SceneKind::MainMenu);

    runtime.mode = RuntimeMode::Menu;
    assert(SceneForRuntime(runtime) == SceneKind::MainMenu);

    runtime.mode = RuntimeMode::JoiningServer;
    runtime.joiningInProgress = false;
    assert(SceneForRuntime(runtime) == SceneKind::JoinServer);

    runtime.joiningInProgress = true;
    assert(SceneForRuntime(runtime) == SceneKind::Connecting);

    runtime.mode = RuntimeMode::StartingLocalServer;
    runtime.joiningInProgress = false;
    assert(SceneForRuntime(runtime) == SceneKind::StartingServer);

    runtime.mode = RuntimeMode::Multiplayer;
    assert(SceneForRuntime(runtime) == SceneKind::GameplayMultiplayer);

    runtime.mode = RuntimeMode::Singleplayer;
    assert(SceneForRuntime(runtime) == SceneKind::GameplaySingleplayer);

    runtime.mode = RuntimeMode::Options;
    assert(SceneForRuntime(runtime) == SceneKind::Options);

    runtime.mode = RuntimeMode::Disconnected;
    assert(SceneForRuntime(runtime) == SceneKind::Disconnected);

    runtime.mode = RuntimeMode::Menu;
    runtime.disconnectReason = "connection closed";
    assert(SceneForRuntime(runtime) == SceneKind::Disconnected);

    return 0;
}
