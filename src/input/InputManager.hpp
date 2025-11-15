#pragma once

#include <raylib-cpp.hpp>

// Extremely lightweight input helper that simply snapshots the current input
// state. More complex mappings can be added later without changing call sites.
class InputManager {
public:
    void Update() {
        mousePosition_ = raylib::Mouse::GetPosition();
        mouseWheel_ = raylib::Mouse::GetWheelMove();
    }

    [[nodiscard]] raylib::Vector2 MousePosition() const { return mousePosition_; }
    [[nodiscard]] float MouseWheel() const { return mouseWheel_; }

    [[nodiscard]] bool IsPressed(int key) const { return raylib::Keyboard::IsKeyPressed(key); }
    [[nodiscard]] bool IsDown(int key) const { return raylib::Keyboard::IsKeyDown(key); }

    [[nodiscard]] bool IsMousePressed(int button) const { return raylib::Mouse::IsButtonPressed(button); }
    [[nodiscard]] bool IsMouseDown(int button) const { return raylib::Mouse::IsButtonDown(button); }

private:
    raylib::Vector2 mousePosition_{0.0f, 0.0f};
    float mouseWheel_ = 0.0f;
};
