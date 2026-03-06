#pragma once

#include <algorithm>
#include <string>

#include <raylib-cpp.hpp>

#include "client/ui/ui_state.hpp"
#include "shared/game/entity.hpp"
#include "shared/game/validation.hpp"

namespace client::input {

namespace game = shared::game;

class InputManager {
public:
    void Update() {
        moveAxisX_ = 0.0f;
        textInput_.clear();
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
        const Vector2 mousePosition = ::GetMousePosition();
        mouseMoved_ = mousePosition_.x != mousePosition.x || mousePosition_.y != mousePosition.y;
        mousePosition_ = mousePosition;
        mousePrimaryPressed_ = ::IsMouseButtonPressed(MOUSE_BUTTON_LEFT);

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
        backspacePressed_ = raylib::Keyboard::IsKeyPressed(KEY_BACKSPACE);

        int codepoint = ::GetCharPressed();
        while (codepoint > 0) {
            if (codepoint >= 32 && codepoint <= 126) {
                textInput_.push_back(static_cast<char>(codepoint));
            }
            codepoint = ::GetCharPressed();
        }
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
    [[nodiscard]] bool MousePrimaryPressed() const { return mousePrimaryPressed_; }
    [[nodiscard]] bool MouseMoved() const { return mouseMoved_; }
    [[nodiscard]] Vector2 MousePosition() const { return mousePosition_; }
    [[nodiscard]] bool BackspacePressed() const { return backspacePressed_; }
    [[nodiscard]] const std::string& TextInput() const { return textInput_; }

    [[nodiscard]] ui::UiInputState BuildUiInputState() const {
        return ui::UiInputState{
            .mouseX = mousePosition_.x,
            .mouseY = mousePosition_.y,
            .mouseMoved = mouseMoved_,
            .primaryPressed = mousePrimaryPressed_,
            .navigateUpPressed = menuUpPressed_,
            .navigateDownPressed = menuDownPressed_,
            .acceptPressed = menuSelectPressed_,
            .backPressed = menuBackPressed_,
            .backspacePressed = backspacePressed_,
            .textInput = textInput_,
        };
    }

private:
    float moveAxisX_ = 0.0f;
    bool jumpPressed_ = false;
    bool debugOverlayToggled_ = false;
    bool quitRequested_ = false;
    bool menuUpPressed_ = false;
    bool menuDownPressed_ = false;
    bool menuSelectPressed_ = false;
    bool menuBackPressed_ = false;
    bool mousePrimaryPressed_ = false;
    bool mouseMoved_ = false;
    bool backspacePressed_ = false;
    Vector2 mousePosition_{};
    std::string textInput_;
};

}  // namespace client::input
