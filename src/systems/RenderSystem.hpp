#pragma once

#include <flecs.h>
#include <raylib-cpp.hpp>

#include "components/Components.hpp"

namespace systems {

class RenderSystem {
public:
    static void DrawWorld(flecs::world& world, raylib::Camera2D& camera,
                          const raylib::Rectangle& worldBounds = {-700.0f, -380.0f, 1400.0f, 760.0f}) {
        camera.BeginMode();
        DrawGrid(worldBounds);
        DrawRectangleLinesEx(worldBounds, 3.0f, raylib::Color{48, 76, 92, 220});

        world.each([&](const Transform2D& transform, const Drawable& drawable, const Tint& tint) {
            const float width = drawable.size * transform.scale.x;
            const float height = drawable.size * transform.scale.y;
            raylib::Rectangle rect{transform.position.x - width * 0.5f, transform.position.y - height * 0.5f, width,
                                   height};

            const raylib::Vector2 origin{width * 0.5f, height * 0.5f};
            rect.Draw(origin, transform.rotation, tint.value);
        });
        camera.EndMode();
    }

private:
    static void DrawGrid(const raylib::Rectangle& worldBounds) {
        constexpr int spacing = 64;
        const int startX = static_cast<int>(worldBounds.x);
        const int endX = static_cast<int>(worldBounds.x + worldBounds.width);
        const int startY = static_cast<int>(worldBounds.y);
        const int endY = static_cast<int>(worldBounds.y + worldBounds.height);

        for (int x = startX; x <= endX; x += spacing) {
            DrawLine(x, startY, x, endY, raylib::Color{28, 43, 54, 180});
        }

        for (int y = startY; y <= endY; y += spacing) {
            DrawLine(startX, y, endX, y, raylib::Color{28, 43, 54, 180});
        }
    }
};

}  // namespace systems
