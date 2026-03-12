#pragma once

#include <algorithm>

#include <raylib-cpp.hpp>

#include "shared/game/game_policy.hpp"

namespace client::input::policy {

inline constexpr int kMaxSupportedGamepads = 4;
inline constexpr int kPrintableAsciiMin = 32;
inline constexpr int kPrintableAsciiMax = 126;

[[nodiscard]] inline bool IsMoveLeftDown() {
    return raylib::Keyboard::IsKeyDown(KEY_A) || raylib::Keyboard::IsKeyDown(KEY_LEFT);
}

[[nodiscard]] inline bool IsMoveRightDown() {
    return raylib::Keyboard::IsKeyDown(KEY_D) || raylib::Keyboard::IsKeyDown(KEY_RIGHT);
}

[[nodiscard]] inline bool IsJumpPressed() {
    return raylib::Keyboard::IsKeyPressed(KEY_SPACE) || raylib::Keyboard::IsKeyPressed(KEY_W) ||
        raylib::Keyboard::IsKeyPressed(KEY_UP);
}

[[nodiscard]] inline bool IsMenuUpPressed() {
    return raylib::Keyboard::IsKeyPressed(KEY_UP) || raylib::Keyboard::IsKeyPressed(KEY_W);
}

[[nodiscard]] inline bool IsMenuDownPressed() {
    return raylib::Keyboard::IsKeyPressed(KEY_DOWN) || raylib::Keyboard::IsKeyPressed(KEY_S);
}

[[nodiscard]] inline bool IsMenuSelectPressed() {
    return raylib::Keyboard::IsKeyPressed(KEY_ENTER) || raylib::Keyboard::IsKeyPressed(KEY_SPACE);
}

[[nodiscard]] inline bool IsMenuBackPressed() {
    return raylib::Keyboard::IsKeyPressed(KEY_ESCAPE);
}

[[nodiscard]] inline float ClampMoveAxis(float moveAxisX) {
    return std::clamp(moveAxisX, shared::game::policy::kMinNormalizedMoveAxis,
                      shared::game::policy::kMaxNormalizedMoveAxis);
}

}  // namespace client::input::policy
