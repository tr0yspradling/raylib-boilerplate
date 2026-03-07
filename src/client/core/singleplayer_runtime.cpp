#include "client/core/singleplayer_runtime.hpp"

#include <string>

namespace client::core {

void SingleplayerRuntime::Start(std::string_view playerName) {
    gameState_ = game::GameState{};
    gameState_.SpawnPlayer(kSingleplayerPlayerId, playerName.empty() ? std::string{"player"} : std::string{playerName});
    active_ = true;
}

void SingleplayerRuntime::Stop() {
    gameState_ = game::GameState{};
    active_ = false;
}

bool SingleplayerRuntime::IsActive() const {
    return active_ && gameState_.HasPlayer(kSingleplayerPlayerId);
}

void SingleplayerRuntime::Step(const game::PlayerInputFrame& inputFrame, float fixedDeltaSeconds) {
    if (!IsActive()) {
        return;
    }

    gameState_.ApplyInputFrame(kSingleplayerPlayerId, inputFrame);
    gameState_.Step(fixedDeltaSeconds);
}

const game::PlayerState* SingleplayerRuntime::LocalPlayer() const {
    if (!IsActive()) {
        return nullptr;
    }

    const auto it = gameState_.Players().find(kSingleplayerPlayerId);
    if (it == gameState_.Players().end()) {
        return nullptr;
    }

    return &it->second;
}

game::TickId SingleplayerRuntime::CurrentTick() const {
    return gameState_.CurrentTick();
}

const game::PlayerKinematicsConfig& SingleplayerRuntime::Kinematics() const {
    return gameState_.Kinematics();
}

}  // namespace client::core
