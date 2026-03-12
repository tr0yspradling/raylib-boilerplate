#pragma once

#include <algorithm>
#include <string>

#include "shared/game/game_policy.hpp"
#include "shared/game/ids.hpp"
#include "shared/game/math_types.hpp"

namespace shared::game {

enum class EntityKind : uint8_t {
    Player,
    Projectile,
    Npc,
};

struct PlayerInputFrame {
    TickId clientTick = 0;
    uint32_t sequence = 0;
    float moveX = 0.0f;
    bool jumpPressed = false;
};

struct PlayerKinematicsConfig {
    float maxMoveSpeed = policy::player::kDefaultMaxMoveSpeed;
    float jumpSpeed = policy::player::kDefaultJumpSpeed;
    float gravity = policy::player::kDefaultGravity;
    float maxFallSpeed = policy::player::kDefaultMaxFallSpeed;
    float groundY = policy::player::kDefaultGroundY;
    float minX = policy::player::kDefaultMinX;
    float maxX = policy::player::kDefaultMaxX;
};

struct PlayerState {
    PlayerId playerId{};
    EntityId entityId{};
    std::string displayName;
    Vec2f position{};
    Vec2f velocity{};
    bool onGround = true;
    uint32_t lastProcessedInputSequence = 0;
    uint32_t lastReceivedInputSequence = 0;
    PlayerInputFrame lastInput{};
};

inline void SimulatePlayerStep(PlayerState& player, const PlayerInputFrame& input, float dt,
                               const PlayerKinematicsConfig& config) {
    const float clampedMove =
        std::clamp(input.moveX, policy::kMinNormalizedMoveAxis, policy::kMaxNormalizedMoveAxis);
    player.velocity.x = clampedMove * config.maxMoveSpeed;

    if (input.jumpPressed && player.onGround) {
        player.velocity.y = config.jumpSpeed;
        player.onGround = false;
    }

    player.velocity.y += config.gravity * dt;
    player.velocity.y = std::max(player.velocity.y, config.maxFallSpeed);

    player.position += player.velocity * dt;
    player.position.x = std::clamp(player.position.x, config.minX, config.maxX);

    if (player.position.y <= config.groundY) {
        player.position.y = config.groundY;
        player.velocity.y = 0.0f;
        player.onGround = true;
    }
}

}  // namespace shared::game
