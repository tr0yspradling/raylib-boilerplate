#pragma once

#include <cmath>
#include <cstdint>

#include "shared/game/entity.hpp"
#include "shared/game/game_policy.hpp"

namespace shared::game {

enum class PlayerInputValidationError : uint8_t {
    None = 0,
    NonFiniteMoveAxis,
    MoveAxisOutOfRange,
    SequenceTooOld,
};

enum class PlayerKinematicsValidationError : uint8_t {
    None = 0,
    NonFiniteValue,
    InvalidMaxMoveSpeed,
    InvalidJumpSpeed,
    InvalidGravity,
    InvalidMaxFallSpeed,
    InvalidHorizontalBounds,
};

struct PlayerInputValidationConfig {
    float maxAbsMoveAxis = policy::validation::kDefaultMaxAbsMoveAxis;
    uint32_t maxSequenceBacktrack = policy::validation::kDefaultMaxSequenceBacktrack;
};

[[nodiscard]] inline PlayerInputValidationError ValidatePlayerInputFrame(const PlayerInputFrame& input,
                                                                         uint32_t lastReceivedSequence,
                                                                         const PlayerInputValidationConfig& config = {}) {
    if (!std::isfinite(input.moveX)) {
        return PlayerInputValidationError::NonFiniteMoveAxis;
    }

    if (std::abs(input.moveX) > config.maxAbsMoveAxis) {
        return PlayerInputValidationError::MoveAxisOutOfRange;
    }

    if (input.sequence + config.maxSequenceBacktrack < lastReceivedSequence) {
        return PlayerInputValidationError::SequenceTooOld;
    }

    return PlayerInputValidationError::None;
}

[[nodiscard]] inline PlayerKinematicsValidationError ValidatePlayerKinematicsConfig(
    const PlayerKinematicsConfig& config) {
    if (!std::isfinite(config.maxMoveSpeed) || !std::isfinite(config.jumpSpeed) || !std::isfinite(config.gravity) ||
        !std::isfinite(config.maxFallSpeed) || !std::isfinite(config.groundY) || !std::isfinite(config.minX) ||
        !std::isfinite(config.maxX)) {
        return PlayerKinematicsValidationError::NonFiniteValue;
    }

    if (config.maxMoveSpeed <= 0.0f) {
        return PlayerKinematicsValidationError::InvalidMaxMoveSpeed;
    }

    if (config.jumpSpeed <= 0.0f) {
        return PlayerKinematicsValidationError::InvalidJumpSpeed;
    }

    if (config.gravity >= 0.0f) {
        return PlayerKinematicsValidationError::InvalidGravity;
    }

    if (config.maxFallSpeed >= 0.0f) {
        return PlayerKinematicsValidationError::InvalidMaxFallSpeed;
    }

    if (config.maxX <= config.minX) {
        return PlayerKinematicsValidationError::InvalidHorizontalBounds;
    }

    return PlayerKinematicsValidationError::None;
}

[[nodiscard]] inline const char* ToString(PlayerInputValidationError error) {
    switch (error) {
    case PlayerInputValidationError::None:
        return "none";
    case PlayerInputValidationError::NonFiniteMoveAxis:
        return "non-finite movement axis";
    case PlayerInputValidationError::MoveAxisOutOfRange:
        return "movement axis out of range";
    case PlayerInputValidationError::SequenceTooOld:
        return "input sequence too old";
    }

    return "unknown";
}

[[nodiscard]] inline const char* ToString(PlayerKinematicsValidationError error) {
    switch (error) {
    case PlayerKinematicsValidationError::None:
        return "none";
    case PlayerKinematicsValidationError::NonFiniteValue:
        return "non-finite physics value";
    case PlayerKinematicsValidationError::InvalidMaxMoveSpeed:
        return "invalid max move speed";
    case PlayerKinematicsValidationError::InvalidJumpSpeed:
        return "invalid jump speed";
    case PlayerKinematicsValidationError::InvalidGravity:
        return "invalid gravity";
    case PlayerKinematicsValidationError::InvalidMaxFallSpeed:
        return "invalid max fall speed";
    case PlayerKinematicsValidationError::InvalidHorizontalBounds:
        return "invalid horizontal bounds";
    }

    return "unknown";
}

}  // namespace shared::game
