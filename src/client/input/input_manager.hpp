#pragma once

#include <algorithm>

#include <raylib-cpp.hpp>

#include "shared/game/entity.hpp"
#include "shared/game/validation.hpp"

namespace client::input {

namespace game = shared::game;

class InputManager {
public:
    void Update() {
        moveAxisX_ = 0.0f;
        if (raylib::Keyboard::IsKeyDown(KEY_A) || raylib::Keyboard::IsKeyDown(KEY_LEFT)) {
            moveAxisX_ -= 1.0f;
        }
        if (raylib::Keyboard::IsKeyDown(KEY_D) || raylib::Keyboard::IsKeyDown(KEY_RIGHT)) {
            moveAxisX_ += 1.0f;
        }

        for (int gamepad = 0; gamepad < 4; ++gamepad) {
            if (!::IsGamepadAvailable(gamepad)) {
                continue;
            }
            moveAxisX_ += ::GetGamepadAxisMovement(gamepad, GAMEPAD_AXIS_LEFT_X);
            if (::IsGamepadButtonDown(gamepad, GAMEPAD_BUTTON_LEFT_FACE_LEFT)) {
                moveAxisX_ -= 1.0f;
            }
            if (::IsGamepadButtonDown(gamepad, GAMEPAD_BUTTON_LEFT_FACE_RIGHT)) {
                moveAxisX_ += 1.0f;
            }
        }

        moveAxisX_ = std::clamp(moveAxisX_, -1.0f, 1.0f);

        jumpPressed_ = raylib::Keyboard::IsKeyPressed(KEY_SPACE) || raylib::Keyboard::IsKeyPressed(KEY_W) ||
            raylib::Keyboard::IsKeyPressed(KEY_UP);

        debugOverlayToggled_ = raylib::Keyboard::IsKeyPressed(KEY_TAB);
        quitRequested_ = raylib::Window::ShouldClose();
    }

    [[nodiscard]] game::PlayerInputFrame BuildPlayerInputFrame(game::TickId tick, uint32_t sequence) const {
        game::PlayerInputFrame frame{
            .clientTick = tick,
            .sequence = sequence,
            .moveX = moveAxisX_,
            .jumpPressed = jumpPressed_,
        };

        const uint32_t previousSequence = sequence > 0 ? sequence - 1U : 0U;
        const game::PlayerInputValidationError validation = game::ValidatePlayerInputFrame(frame, previousSequence);
        if (validation != game::PlayerInputValidationError::None) {
            frame.moveX = std::clamp(frame.moveX, -1.0f, 1.0f);
            frame.jumpPressed = false;
        }

        return frame;
    }

    [[nodiscard]] float MoveAxisX() const { return moveAxisX_; }
    [[nodiscard]] bool JumpPressed() const { return jumpPressed_; }
    [[nodiscard]] bool DebugOverlayToggled() const { return debugOverlayToggled_; }
    [[nodiscard]] bool QuitRequested() const { return quitRequested_; }

private:
    float moveAxisX_ = 0.0f;
    bool jumpPressed_ = false;
    bool debugOverlayToggled_ = false;
    bool quitRequested_ = false;
};

}  // namespace client::input
