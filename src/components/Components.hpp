#pragma once

#include <raylib-cpp.hpp>

// Transform data is intentionally simple so it can be extended later without
// touching every system. This file provides a minimal set of components that
// make sense for most arcade-style prototypes.
struct Transform2D {
    raylib::Vector2 position{0.0f, 0.0f};
    raylib::Vector2 scale{1.0f, 1.0f};
    float rotation = 0.0f;
};

struct Velocity {
    raylib::Vector2 value{0.0f, 0.0f};
};

struct Spin {
    float degreesPerSecond = 0.0f;
};

struct Tint {
    raylib::Color value{255, 255, 255, 255};
};

struct Drawable {
    float size = 32.0f;
    float cornerRadius = 6.0f;
};

struct Pulse {
    float frequency = 1.0f;
    float amplitude = 0.25f;
    float baseScale = 1.0f;
    float elapsed = 0.0f;
};
