#pragma once

#include <deque>

#include "shared/game/entity.hpp"
#include "shared/game/prediction.hpp"
#include "shared/net/snapshot.hpp"

namespace client::physics {

namespace game = shared::game;
namespace net = shared::net;

class MovementSystem {
public:
    static void Predict(game::PlayerState& localPlayer, const game::PlayerInputFrame& inputFrame, float dt,
                        const game::PlayerKinematicsConfig& kinematics) {
        game::PredictPlayer(localPlayer, inputFrame, dt, kinematics);
    }

    static void Reconcile(game::PlayerState& localPlayer, const net::SnapshotEntity& authoritative,
                          std::deque<game::PlayerInputFrame>& pendingInputs, float dt,
                          const game::PlayerKinematicsConfig& kinematics) {
        game::ReconcilePlayer(
            localPlayer,
            game::AuthoritativePlayerState{.entityId = authoritative.entityId,
                                           .displayName = authoritative.displayName,
                                           .position = authoritative.position,
                                           .velocity = authoritative.velocity,
                                           .onGround = authoritative.onGround,
                                           .lastProcessedInputSequence = authoritative.lastProcessedInputSequence},
            pendingInputs, dt, kinematics);
    }
};

}  // namespace client::physics
