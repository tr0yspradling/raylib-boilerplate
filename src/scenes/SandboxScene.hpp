#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <flecs.h>
#include <numbers>
#include <random>

#include "../components/Components.hpp"
#include "../core/Scene.hpp"
#include "../physics/MovementSystem.hpp"
#include "../systems/RenderSystem.hpp"

class SandboxScene : public Scene {
public:
    SandboxScene() {
        camera_.SetOffset({static_cast<float>(raylib::Window::GetWidth()) * 0.5f,
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

        if (context.input.IsPressed(KEY_R)) {
            ResetWorld();
        }

        if (context.input.IsMousePressed(MOUSE_BUTTON_LEFT)) {
            SpawnBurst(camera_.GetScreenToWorld(context.input.MousePosition()));
        }

        if (context.input.IsPressed(KEY_F)) {
            camera_.SetTarget({0.0f, 0.0f});
            camera_.SetZoom(1.0f);
        }

        HandleCameraInput(context.input, dt);
        world_.progress(dt);
    }

    void Draw(SceneContext& context) override {
        systems::RenderSystem::DrawWorld(world_, camera_, worldBounds_);

        const raylib::Vector2 size = context.window.GetSize();
        raylib::Color{148, 168, 186, 255}.DrawText(
            "Sandbox: WASD/Arrows pan  |  RMB drag pan  |  Wheel zoom  |  LMB spawn  |  R reset  |  F refocus", 20,
            static_cast<int>(size.y) - 30, 18);
    }

    [[nodiscard]] std::string_view Name() const override { return "Sandbox"; }

private:
    void InitializeWorld() {
        physics::MovementSystem::Register(world_);
        const Bounds worldBounds{{worldBounds_.x, worldBounds_.y},
                                 {worldBounds_.x + worldBounds_.width, worldBounds_.y + worldBounds_.height}};

        std::uniform_real_distribution<float> posX(worldBounds.min.x, worldBounds.max.x);
        std::uniform_real_distribution<float> posY(worldBounds.min.y, worldBounds.max.y);
        std::uniform_real_distribution<float> vel(-95.0f, 95.0f);
        std::uniform_real_distribution<float> size(20.0f, 44.0f);
        std::uniform_real_distribution<float> spin(-80.0f, 80.0f);
        std::uniform_real_distribution<float> pulseFreq(0.25f, 1.0f);

        for (int i = 0; i < 42; ++i) {
            world_.entity()
                .set<Transform2D>({{posX(rng_), posY(rng_)}, {1.0f, 1.0f}, 0.0f})
                .set<Velocity>({{vel(rng_), vel(rng_)}})
                .set<Spin>({spin(rng_)})
                .set<Tint>({palette_[static_cast<std::size_t>(i % palette_.size())]})
                .set<Drawable>({size(rng_), 8.0f})
                .set<Pulse>({pulseFreq(rng_), 0.25f, 1.0f, 0.0f})
                .set<Bounds>(worldBounds);
        }
    }

    void ResetWorld() {
        world_.reset();
        InitializeWorld();
    }

    void SpawnBurst(const raylib::Vector2& worldPosition) {
        const Bounds worldBounds{{worldBounds_.x, worldBounds_.y},
                                 {worldBounds_.x + worldBounds_.width, worldBounds_.y + worldBounds_.height}};
        std::uniform_real_distribution<float> angle(0.0f, std::numbers::pi_v<float> * 2.0f);
        std::uniform_real_distribution<float> speed(120.0f, 280.0f);
        std::uniform_real_distribution<float> size(10.0f, 24.0f);
        std::uniform_real_distribution<float> spin(-180.0f, 180.0f);
        std::uniform_real_distribution<float> lifetime(1.2f, 3.0f);
        std::uniform_real_distribution<float> pulseFreq(0.4f, 2.2f);

        for (int i = 0; i < 14; ++i) {
            const float theta = angle(rng_);
            const float initialSpeed = speed(rng_);
            const raylib::Vector2 direction{std::cos(theta), std::sin(theta)};

            world_.entity()
                .set<Transform2D>({worldPosition, {1.0f, 1.0f}, 0.0f})
                .set<Velocity>({direction * initialSpeed})
                .set<Spin>({spin(rng_)})
                .set<Tint>({palette_[static_cast<std::size_t>(spawnIndex_++ % static_cast<int>(palette_.size()))]})
                .set<Drawable>({size(rng_), 5.0f})
                .set<Pulse>({pulseFreq(rng_), 0.15f, 1.0f, 0.0f})
                .set<Bounds>(worldBounds)
                .set<Lifetime>({lifetime(rng_)});
        }
    }

    void HandleCameraInput(InputManager& input, float dt) {
        const float moveSpeed = 340.0f * dt;
        raylib::Vector2 target = camera_.GetTarget();
        const raylib::Vector2 axis = input.MoveAxis();
        target.x += axis.x * moveSpeed;
        target.y += axis.y * moveSpeed;

        if (input.IsMouseDown(MOUSE_BUTTON_RIGHT)) {
            if (!draggingCamera_) {
                draggingCamera_ = true;
                lastMousePosition_ = input.MousePosition();
            } else {
                const raylib::Vector2 mouseDelta = input.MousePosition() - lastMousePosition_;
                target -= mouseDelta * (1.0f / camera_.GetZoom());
                lastMousePosition_ = input.MousePosition();
            }
        } else {
            draggingCamera_ = false;
        }

        float zoom = camera_.GetZoom();
        zoom = std::clamp(zoom + input.MouseWheel() * 0.08f, 0.25f, 3.0f);

        camera_.SetTarget(target);
        camera_.SetZoom(zoom);
    }

    const std::array<raylib::Color, 8> palette_ = {
        raylib::Color{0xf4, 0x72, 0xb6, 0xff}, raylib::Color{0x12, 0xc5, 0xed, 0xff},
        raylib::Color{0xf9, 0xc8, 0x0e, 0xff}, raylib::Color{0x65, 0xa3, 0xff, 0xff},
        raylib::Color{0x93, 0xff, 0x94, 0xff}, raylib::Color{0xfd, 0x79, 0xa8, 0xff},
        raylib::Color{0xf9, 0x73, 0x16, 0xff}, raylib::Color{0x22, 0xc5, 0x5e, 0xff},
    };

    flecs::world world_;
    raylib::Camera2D camera_;
    bool initialized_ = false;
    bool draggingCamera_ = false;
    raylib::Vector2 lastMousePosition_{0.0f, 0.0f};
    raylib::Rectangle worldBounds_{-760.0f, -430.0f, 1520.0f, 860.0f};
    std::mt19937 rng_{std::random_device{}()};
    int spawnIndex_ = 0;
};
