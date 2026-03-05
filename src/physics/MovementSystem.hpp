#pragma once

#include <algorithm>
#include <cmath>
#include <flecs.h>
#include <numbers>

#include "../components/Components.hpp"

namespace physics {

class MovementSystem {
public:
    static void Register(flecs::world& world) {
        world.system<Transform2D, const Velocity>("Movement")
            .each([&world](Transform2D& transform, const Velocity& velocity) {
                const float dt = static_cast<float>(world.delta_time());
                transform.position += velocity.value * dt;
            });

        world.system<Transform2D, const Spin>("Spin").each([&world](Transform2D& transform, const Spin& spin) {
            transform.rotation += spin.degreesPerSecond * static_cast<float>(world.delta_time());
        });

        world.system<Transform2D, Pulse>("Pulse").each([&world](Transform2D& transform, Pulse& pulse) {
            pulse.elapsed += static_cast<float>(world.delta_time());
            const float oscillation = pulse.baseScale +
                std::sin(pulse.elapsed * 2.0f * std::numbers::pi_v<float> * pulse.frequency) * pulse.amplitude;
            transform.scale = raylib::Vector2{oscillation, oscillation};
        });

        world.system<Transform2D, Velocity, const Bounds>("Bounds").each(
            [](Transform2D& transform, Velocity& velocity, const Bounds& bounds) {
                if (transform.position.x < bounds.min.x) {
                    transform.position.x = bounds.min.x;
                    velocity.value.x = std::abs(velocity.value.x);
                } else if (transform.position.x > bounds.max.x) {
                    transform.position.x = bounds.max.x;
                    velocity.value.x = -std::abs(velocity.value.x);
                }

                if (transform.position.y < bounds.min.y) {
                    transform.position.y = bounds.min.y;
                    velocity.value.y = std::abs(velocity.value.y);
                } else if (transform.position.y > bounds.max.y) {
                    transform.position.y = bounds.max.y;
                    velocity.value.y = -std::abs(velocity.value.y);
                }
            });

        world.system<Lifetime>("Lifetime").each([&world](flecs::entity entity, Lifetime& lifetime) {
            lifetime.remaining -= static_cast<float>(world.delta_time());
            if (lifetime.remaining <= 0.0f) {
                entity.destruct();
            }
        });
    }
};

}  // namespace physics
