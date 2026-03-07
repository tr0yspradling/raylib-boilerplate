#pragma once

#include <string_view>

#include "shared/game/game_state.hpp"

namespace client::core {

namespace game = shared::game;

class SingleplayerRuntime {
public:
    void Start(std::string_view playerName);
    void Stop();

    [[nodiscard]] bool IsActive() const;
    void Step(const game::PlayerInputFrame& inputFrame, float fixedDeltaSeconds);

    [[nodiscard]] const game::PlayerState* LocalPlayer() const;
    [[nodiscard]] game::TickId CurrentTick() const;
    [[nodiscard]] const game::PlayerKinematicsConfig& Kinematics() const;

private:
    static constexpr game::PlayerId kSingleplayerPlayerId{1};

    game::GameState gameState_{};
    bool active_ = false;
};

}  // namespace client::core
