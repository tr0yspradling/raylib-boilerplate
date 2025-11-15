#pragma once

#include <algorithm>
#include <array>
#include <random>

#include <flecs.h>

#include "../components/Components.hpp"
#include "../core/Scene.hpp"
#include "../physics/MovementSystem.hpp"
#include "../systems/RenderSystem.hpp"

class SandboxScene : public Scene {
public:
    SandboxScene() {
        camera_.SetOffset(
            {static_cast<float>(raylib::Window::GetWidth()) * 0.5f,
             static_cast<float>(raylib::Window::GetHeight()) * 0.5f});
        camera_.SetTarget({0.0f, 0.0f});
        camera_.SetZoom(1.0f);
    }

    void OnEnter(SceneContext& context) override {
        if (!initialized_) {
            InitializeWorld();
            initialized_ = true;
        }

        const raylib::Vector2 size = context.window.GetSize();
        camera_.SetOffset({size.x * 0.5f, size.y * 0.5f});
    }

    void Update(SceneContext& context, float dt) override {
        if (context.input.IsPressed(KEY_ESCAPE)) {
            context.manager.SwitchTo("menu");
            return;
        }

        HandleCameraInput(context.input, dt);
        world_.progress(dt);
    }

    void Draw([[maybe_unused]] SceneContext& context) override {
        systems::RenderSystem::DrawWorld(world_, camera_);
    }

    [[nodiscard]] std::string_view Name() const override { return "Sandbox"; }

private:
    void InitializeWorld() {
        physics::MovementSystem::Register(world_);

        std::mt19937 rng{std::random_device{}()};
        std::uniform_real_distribution<float> pos(-300.0f, 300.0f);
        std::uniform_real_distribution<float> vel(-40.0f, 40.0f);
        std::uniform_real_distribution<float> size(24.0f, 48.0f);
        std::uniform_real_distribution<float> spin(-45.0f, 45.0f);
        std::uniform_real_distribution<float> pulseFreq(0.25f, 1.0f);

        const std::array<raylib::Color, 6> palette = {
            raylib::Color{0xf4, 0x72, 0xb6, 0xff},
            raylib::Color{0x12, 0xc5, 0xed, 0xff},
            raylib::Color{0xf9, 0xc8, 0x0e, 0xff},
            raylib::Color{0x1b, 0x28, 0x36, 0xff},
            raylib::Color{0x93, 0xff, 0x94, 0xff},
            raylib::Color{0xfd, 0x79, 0xa8, 0xff},
        };

        for (int i = 0; i < 32; ++i) {
            world_.entity()
                .set<Transform2D>({{pos(rng), pos(rng)}, {1.0f, 1.0f}, 0.0f})
                .set<Velocity>({{vel(rng), vel(rng)}})
                .set<Spin>({spin(rng)})
                .set<Tint>({palette[static_cast<std::size_t>(i % palette.size())]})
                .set<Drawable>({size(rng), 8.0f})
                .set<Pulse>({pulseFreq(rng), 0.25f, 1.0f, 0.0f});
        }
    }

    void HandleCameraInput(InputManager& input, float dt) {
        const float moveSpeed = 280.0f * dt;
        raylib::Vector2 target = camera_.GetTarget();

        if (input.IsDown(KEY_A) || input.IsDown(KEY_LEFT)) {
            target.x -= moveSpeed;
        }
        if (input.IsDown(KEY_D) || input.IsDown(KEY_RIGHT)) {
            target.x += moveSpeed;
        }
        if (input.IsDown(KEY_W) || input.IsDown(KEY_UP)) {
            target.y -= moveSpeed;
        }
        if (input.IsDown(KEY_S) || input.IsDown(KEY_DOWN)) {
            target.y += moveSpeed;
        }

        float zoom = camera_.GetZoom();
        zoom = std::clamp(zoom + input.MouseWheel() * 0.05f, 0.25f, 3.0f);

        camera_.SetTarget(target);
        camera_.SetZoom(zoom);
    }

    flecs::world world_;
    raylib::Camera2D camera_;
    bool initialized_ = false;
};
