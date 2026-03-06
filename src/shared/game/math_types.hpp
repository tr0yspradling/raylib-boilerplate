#pragma once

#include <algorithm>
#include <cmath>

namespace shared::game {

struct Vec2f {
    float x = 0.0f;
    float y = 0.0f;

    [[nodiscard]] Vec2f operator+(const Vec2f& rhs) const { return {x + rhs.x, y + rhs.y}; }
    [[nodiscard]] Vec2f operator-(const Vec2f& rhs) const { return {x - rhs.x, y - rhs.y}; }
    [[nodiscard]] Vec2f operator*(float scalar) const { return {x * scalar, y * scalar}; }

    Vec2f& operator+=(const Vec2f& rhs) {
        x += rhs.x;
        y += rhs.y;
        return *this;
    }

    Vec2f& operator-=(const Vec2f& rhs) {
        x -= rhs.x;
        y -= rhs.y;
        return *this;
    }

    Vec2f& operator*=(float scalar) {
        x *= scalar;
        y *= scalar;
        return *this;
    }
};

[[nodiscard]] inline Vec2f Lerp(const Vec2f& a, const Vec2f& b, float alpha) {
    const float t = std::clamp(alpha, 0.0f, 1.0f);
    return {a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t};
}

[[nodiscard]] inline float LengthSq(const Vec2f& value) { return value.x * value.x + value.y * value.y; }

}  // namespace shared::game
