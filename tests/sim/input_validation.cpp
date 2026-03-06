#include <cassert>

#include "shared/game/prediction.hpp"
#include "shared/game/validation.hpp"

int main() {
    using namespace shared::game;

    PlayerInputFrame input;
    input.sequence = 10;
    input.moveX = 0.5f;
    input.jumpPressed = false;

    assert(ValidatePlayerInputFrame(input, 9) == PlayerInputValidationError::None);

    input.moveX = 2.0f;
    assert(ValidatePlayerInputFrame(input, 9) == PlayerInputValidationError::MoveAxisOutOfRange);

    input.moveX = 0.5f;
    assert(ValidatePlayerInputFrame(input, 5000) == PlayerInputValidationError::SequenceTooOld);

    PlayerKinematicsConfig kinematics;
    assert(ValidatePlayerKinematicsConfig(kinematics) == PlayerKinematicsValidationError::None);
    kinematics.gravity = 1.0f;
    assert(ValidatePlayerKinematicsConfig(kinematics) == PlayerKinematicsValidationError::InvalidGravity);
    kinematics.gravity = -24.0f;
    kinematics.maxX = kinematics.minX;
    assert(ValidatePlayerKinematicsConfig(kinematics) == PlayerKinematicsValidationError::InvalidHorizontalBounds);

    PlayerState local;
    local.onGround = true;
    local.position = {0.0f, 0.0f};

    std::deque<PlayerInputFrame> pending;
    pending.push_back(PlayerInputFrame{.clientTick = 1, .sequence = 1, .moveX = 1.0f, .jumpPressed = false});
    pending.push_back(PlayerInputFrame{.clientTick = 2, .sequence = 2, .moveX = 1.0f, .jumpPressed = false});

    ReconcilePlayer(local,
                    AuthoritativePlayerState{.entityId = EntityId{1},
                                             .displayName = "p",
                                             .position = {0.0f, 0.0f},
                                             .velocity = {0.0f, 0.0f},
                                             .onGround = true,
                                             .lastProcessedInputSequence = 1},
                    pending, 1.0f / 30.0f, PlayerKinematicsConfig{});

    assert(pending.size() == 1);
    assert(pending.front().sequence == 2);
    assert(local.position.x > 0.0f);

    return 0;
}
