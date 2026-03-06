#include <cassert>

#include "shared/game/entity.hpp"
#include "shared/game/fixed_step.hpp"

int main() {
    shared::game::FixedStep fixed{1.0 / 30.0};

    const int stepCount = fixed.Accumulate(1.0 / 15.0);
    assert(stepCount == 2);

    shared::game::PlayerState player;
    player.onGround = true;
    player.position = {0.0f, 0.0f};

    shared::game::PlayerInputFrame moveRight;
    moveRight.moveX = 1.0f;
    moveRight.jumpPressed = false;

    shared::game::SimulatePlayerStep(player, moveRight, static_cast<float>(fixed.StepSeconds()),
                                     shared::game::PlayerKinematicsConfig{});
    assert(player.position.x > 0.0f);

    shared::game::PlayerInputFrame jump;
    jump.moveX = 0.0f;
    jump.jumpPressed = true;
    shared::game::SimulatePlayerStep(player, jump, static_cast<float>(fixed.StepSeconds()),
                                     shared::game::PlayerKinematicsConfig{});
    assert(player.position.y >= 0.0f);

    return 0;
}
