#pragma once

#include <cmath>
#include <numbers>

#include <flecs.h>

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

        world.system<Transform2D, const Spin>("Spin")
            .each([&world](Transform2D& transform, const Spin& spin) {
                transform.rotation += spin.degreesPerSecond * static_cast<float>(world.delta_time());
            });

        world.system<Transform2D, Pulse>("Pulse")
            .each([&world](Transform2D& transform, Pulse& pulse) {
                pulse.elapsed += static_cast<float>(world.delta_time());
                const float oscillation = pulse.baseScale +
                    std::sin(pulse.elapsed * 2.0f * std::numbers::pi_v<float> * pulse.frequency) * pulse.amplitude;
                transform.scale = raylib::Vector2{oscillation, oscillation};
            });
    }
};

}  // namespace physics
