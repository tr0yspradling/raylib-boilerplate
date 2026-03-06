#pragma once

#include <deque>
#include <string>

#include "shared/game/entity.hpp"

namespace shared::game {

struct AuthoritativePlayerState {
    EntityId entityId{};
    std::string displayName;
    Vec2f position{};
    Vec2f velocity{};
    bool onGround = true;
    uint32_t lastProcessedInputSequence = 0;
};

inline void PredictPlayer(PlayerState& localPlayer, const PlayerInputFrame& inputFrame, float dt,
                          const PlayerKinematicsConfig& kinematics = {}) {
    SimulatePlayerStep(localPlayer, inputFrame, dt, kinematics);
    localPlayer.lastProcessedInputSequence = inputFrame.sequence;
}

inline void ReconcilePlayer(PlayerState& localPlayer, const AuthoritativePlayerState& authoritative,
                            std::deque<PlayerInputFrame>& pendingInputs, float dt,
                            const PlayerKinematicsConfig& kinematics = {}) {
    localPlayer.position = authoritative.position;
    localPlayer.velocity = authoritative.velocity;
    localPlayer.onGround = authoritative.onGround;
    localPlayer.entityId = authoritative.entityId;
    localPlayer.displayName = authoritative.displayName;

    while (!pendingInputs.empty() && pendingInputs.front().sequence <= authoritative.lastProcessedInputSequence) {
        pendingInputs.pop_front();
    }

    for (const PlayerInputFrame& pending : pendingInputs) {
        SimulatePlayerStep(localPlayer, pending, dt, kinematics);
    }
}

}  // namespace shared::game
