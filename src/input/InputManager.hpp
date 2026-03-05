#pragma once

#include <algorithm>
#include <raylib-cpp.hpp>

// Extremely lightweight input helper that simply snapshots the current input
// state. More complex mappings can be added later without changing call sites.
class InputManager {
public:
    void Update() {
        mousePosition_ = raylib::Mouse::GetPosition();
        mouseWheel_ = raylib::Mouse::GetWheelMove();

        selectPressed_ = IsMousePressed(MOUSE_BUTTON_LEFT) || IsPressed(KEY_ENTER) || IsPressed(KEY_SPACE);
        quitRequested_ = raylib::Window::ShouldClose();
        quitRequested_ |=
            (IsDown(KEY_LEFT_CONTROL) || IsDown(KEY_RIGHT_CONTROL)) && (IsPressed(KEY_Q) || IsPressed(KEY_W));
        pauseToggled_ = IsPressed(KEY_P);
        fullscreenToggled_ = (IsDown(KEY_LEFT_CONTROL) || IsDown(KEY_RIGHT_CONTROL)) && IsPressed(KEY_F);
        debugToggled_ = IsPressed(KEY_TAB);

        moveAxis_ = raylib::Vector2{0.0f, 0.0f};
        activeGamepad_ = -1;
        CollectKeyboardAxis();
        CollectGamepadAxisAndButtons();

        moveAxis_.x = std::clamp(moveAxis_.x, -1.0f, 1.0f);
        moveAxis_.y = std::clamp(moveAxis_.y, -1.0f, 1.0f);
        const float lengthSq = moveAxis_.x * moveAxis_.x + moveAxis_.y * moveAxis_.y;
        if (lengthSq < 0.09f) {
            moveAxis_ = raylib::Vector2{0.0f, 0.0f};
        }
    }

    [[nodiscard]] raylib::Vector2 MousePosition() const { return mousePosition_; }
    [[nodiscard]] float MouseWheel() const { return mouseWheel_; }
    [[nodiscard]] raylib::Vector2 MoveAxis() const { return moveAxis_; }
    [[nodiscard]] bool SelectPressed() const { return selectPressed_; }
    [[nodiscard]] bool QuitRequested() const { return quitRequested_; }
    [[nodiscard]] bool PauseToggled() const { return pauseToggled_; }
    [[nodiscard]] bool FullscreenToggled() const { return fullscreenToggled_; }
    [[nodiscard]] bool DebugToggled() const { return debugToggled_; }
    [[nodiscard]] int ActiveGamepad() const { return activeGamepad_; }

    [[nodiscard]] bool IsPressed(int key) const { return raylib::Keyboard::IsKeyPressed(key); }
    [[nodiscard]] bool IsDown(int key) const { return raylib::Keyboard::IsKeyDown(key); }

    [[nodiscard]] bool IsMousePressed(int button) const { return raylib::Mouse::IsButtonPressed(button); }
    [[nodiscard]] bool IsMouseDown(int button) const { return raylib::Mouse::IsButtonDown(button); }

private:
    void CollectKeyboardAxis() {
        if (IsDown(KEY_A) || IsDown(KEY_LEFT)) {
            moveAxis_.x -= 1.0f;
        }
        if (IsDown(KEY_D) || IsDown(KEY_RIGHT)) {
            moveAxis_.x += 1.0f;
        }
        if (IsDown(KEY_W) || IsDown(KEY_UP)) {
            moveAxis_.y -= 1.0f;
        }
        if (IsDown(KEY_S) || IsDown(KEY_DOWN)) {
            moveAxis_.y += 1.0f;
        }
    }

    void CollectGamepadAxisAndButtons() {
        for (int index = 0; index < 4; ++index) {
            if (!::IsGamepadAvailable(index)) {
                continue;
            }

            if (activeGamepad_ < 0) {
                activeGamepad_ = index;
            }

            moveAxis_.x += ::GetGamepadAxisMovement(index, GAMEPAD_AXIS_LEFT_X);
            moveAxis_.y += ::GetGamepadAxisMovement(index, GAMEPAD_AXIS_LEFT_Y);

            if (::IsGamepadButtonDown(index, GAMEPAD_BUTTON_LEFT_FACE_LEFT)) {
                moveAxis_.x -= 1.0f;
            }
            if (::IsGamepadButtonDown(index, GAMEPAD_BUTTON_LEFT_FACE_RIGHT)) {
                moveAxis_.x += 1.0f;
            }
            if (::IsGamepadButtonDown(index, GAMEPAD_BUTTON_LEFT_FACE_UP)) {
                moveAxis_.y -= 1.0f;
            }
            if (::IsGamepadButtonDown(index, GAMEPAD_BUTTON_LEFT_FACE_DOWN)) {
                moveAxis_.y += 1.0f;
            }

            if (::IsGamepadButtonPressed(index, GAMEPAD_BUTTON_RIGHT_FACE_DOWN) ||
                ::IsGamepadButtonPressed(index, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT)) {
                selectPressed_ = true;
            }

            if (::IsGamepadButtonPressed(index, GAMEPAD_BUTTON_MIDDLE_LEFT)) {
                quitRequested_ = true;
            }

            if (::IsGamepadButtonPressed(index, GAMEPAD_BUTTON_MIDDLE_RIGHT)) {
                pauseToggled_ = true;
            }
        }
    }

    raylib::Vector2 mousePosition_{0.0f, 0.0f};
    float mouseWheel_ = 0.0f;
    raylib::Vector2 moveAxis_{0.0f, 0.0f};
    bool selectPressed_ = false;
    bool quitRequested_ = false;
    bool pauseToggled_ = false;
    bool fullscreenToggled_ = false;
    bool debugToggled_ = false;
    int activeGamepad_ = -1;
};
