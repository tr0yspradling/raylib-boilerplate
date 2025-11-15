#pragma once

#include <flecs.h>
#include <raylib-cpp.hpp>

#include "../components/Components.hpp"

namespace systems {

class RenderSystem {
public:
    static void DrawWorld(flecs::world& world, raylib::Camera2D& camera) {
        camera.BeginMode();
        world.each([&](const Transform2D& transform, const Drawable& drawable, const Tint& tint) {
            const float width = drawable.size * transform.scale.x;
            const float height = drawable.size * transform.scale.y;
            raylib::Rectangle rect{
                transform.position.x - width * 0.5f,
                transform.position.y - height * 0.5f,
                width,
                height};

            const raylib::Vector2 origin{width * 0.5f, height * 0.5f};
            rect.Draw(origin, transform.rotation, tint.value);
        });
        camera.EndMode();
    }
};

}  // namespace systems
