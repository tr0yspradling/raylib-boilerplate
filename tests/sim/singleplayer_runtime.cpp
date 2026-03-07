#include <cassert>

#include "client/core/singleplayer_runtime.hpp"

int main() {
    client::core::SingleplayerRuntime runtime;
    runtime.Start("alice");

    assert(runtime.IsActive());
    const shared::game::PlayerState* localPlayer = runtime.LocalPlayer();
    assert(localPlayer != nullptr);
    assert(localPlayer->displayName == "alice");
    assert(localPlayer->position.x == 0.0f);
    assert(localPlayer->position.y == 0.0f);

    runtime.Step({.clientTick = 0, .sequence = 1, .moveX = 1.0f, .jumpPressed = false}, 1.0f / 30.0f);
    localPlayer = runtime.LocalPlayer();
    assert(localPlayer != nullptr);
    assert(localPlayer->position.x > 0.0f);
    assert(localPlayer->position.y == 0.0f);

    const float yBeforeJump = localPlayer->position.y;
    runtime.Step({.clientTick = 1, .sequence = 2, .moveX = 0.0f, .jumpPressed = true}, 1.0f / 30.0f);
    localPlayer = runtime.LocalPlayer();
    assert(localPlayer != nullptr);
    assert(localPlayer->position.y > yBeforeJump);
    assert(localPlayer->onGround == false);
    assert(runtime.CurrentTick() == 2);

    runtime.Stop();
    assert(!runtime.IsActive());
    assert(runtime.LocalPlayer() == nullptr);
    assert(runtime.CurrentTick() == 0);

    return 0;
}
