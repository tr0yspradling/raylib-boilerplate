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
        menuUpPressed_ = raylib::Keyboard::IsKeyPressed(KEY_UP) || raylib::Keyboard::IsKeyPressed(KEY_W);
        menuDownPressed_ = raylib::Keyboard::IsKeyPressed(KEY_DOWN) || raylib::Keyboard::IsKeyPressed(KEY_S);
        menuSelectPressed_ = raylib::Keyboard::IsKeyPressed(KEY_ENTER) || raylib::Keyboard::IsKeyPressed(KEY_SPACE);
        menuBackPressed_ = raylib::Keyboard::IsKeyPressed(KEY_ESCAPE);

        for (int gamepad = 0; gamepad < 4; ++gamepad) {
            if (!::IsGamepadAvailable(gamepad)) {
                continue;
            }

            if (::IsGamepadButtonPressed(gamepad, GAMEPAD_BUTTON_LEFT_FACE_UP)) {
                menuUpPressed_ = true;
            }
            if (::IsGamepadButtonPressed(gamepad, GAMEPAD_BUTTON_LEFT_FACE_DOWN)) {
                menuDownPressed_ = true;
            }
            if (::IsGamepadButtonPressed(gamepad, GAMEPAD_BUTTON_RIGHT_FACE_DOWN)) {
                menuSelectPressed_ = true;
            }
            if (::IsGamepadButtonPressed(gamepad, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT)) {
                menuBackPressed_ = true;
            }
        }

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
    [[nodiscard]] bool MenuUpPressed() const { return menuUpPressed_; }
    [[nodiscard]] bool MenuDownPressed() const { return menuDownPressed_; }
    [[nodiscard]] bool MenuSelectPressed() const { return menuSelectPressed_; }
    [[nodiscard]] bool MenuBackPressed() const { return menuBackPressed_; }

private:
    float moveAxisX_ = 0.0f;
    bool jumpPressed_ = false;
    bool debugOverlayToggled_ = false;
    bool quitRequested_ = false;
    bool menuUpPressed_ = false;
    bool menuDownPressed_ = false;
    bool menuSelectPressed_ = false;
    bool menuBackPressed_ = false;
};

}  // namespace client::input
