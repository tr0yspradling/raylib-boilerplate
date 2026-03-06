#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <flecs.h>
#include <numbers>
#include <random>

#include "components/Components.hpp"
#include "core/Scene.hpp"
#include "physics/MovementSystem.hpp"
#include "systems/RenderSystem.hpp"

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
        SetPaused(false);
    }

    void OnExit([[maybe_unused]] SceneContext& context) override { SetPaused(false); }

    void Update(SceneContext& context, float dt) override {
        const bool pauseToggleRequested = context.input.PauseToggled() || context.input.IsPressed(KEY_ESCAPE);
        if (pauseToggleRequested) {
            SetPaused(!paused_);
        }
        if (!paused_ && (raylib::Window::IsResized() || !raylib::Window::IsFocused())) {
            SetPaused(true);
        }
        if (paused_) {
            HandlePauseMenuInput(context);
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
        if (paused_) {
            DrawPauseMenu(context.window.GetSize());
        }

        const raylib::Vector2 size = context.window.GetSize();
        raylib::Color{148, 168, 186, 255}.DrawText(
            "Sandbox: WASD/Arrows pan  |  RMB drag pan  |  Wheel zoom  |  LMB spawn  |  R reset  |  F refocus  |  Esc pause",
            20, static_cast<int>(size.y) - 30, 18);
    }

    [[nodiscard]] bool IsPaused() const override { return paused_; }
    [[nodiscard]] std::string_view Name() const override { return "Sandbox"; }

private:
    enum class PauseAction : std::size_t {
        Resume = 0,
        Save = 1,
        MainMenu = 2,
        Quit = 3,
    };

    void SetPaused(bool paused) {
        if (paused_ == paused) {
            return;
        }

        paused_ = paused;
        if (paused_) {
            pauseSelected_ = 0;
            pauseMessageTimer_ = 0.0f;
        }
    }

    void HandlePauseMenuInput(SceneContext& context) {
        const raylib::Vector2 windowSize = context.window.GetSize();
        const int hoveredIndex = PauseRowFromMouse(context.input.MousePosition(), windowSize);
        if (hoveredIndex >= 0) {
            pauseSelected_ = hoveredIndex;
        }

        if (context.input.IsPressed(KEY_UP)) {
            pauseSelected_ = std::max(0, pauseSelected_ - 1);
        }
        if (context.input.IsPressed(KEY_DOWN)) {
            pauseSelected_ = std::min(static_cast<int>(pauseOptions_.size()) - 1, pauseSelected_ + 1);
        }

        pauseMessageTimer_ = std::max(0.0f, pauseMessageTimer_ - raylib::Window::GetFrameTime());

        bool activateSelection = false;
        const bool mousePressed = context.input.IsMousePressed(MOUSE_BUTTON_LEFT);
        if (mousePressed) {
            if (hoveredIndex < 0) {
                return;
            }
            pauseSelected_ = hoveredIndex;
            activateSelection = true;
        } else if (context.input.SelectPressed()) {
            activateSelection = true;
        }

        if (!activateSelection) {
            return;
        }

        const PauseAction action = static_cast<PauseAction>(pauseSelected_);
        if (action == PauseAction::Resume) {
            SetPaused(false);
            return;
        }

        if (action == PauseAction::Save) {
            TraceLog(LOG_INFO, "Save requested from pause menu (not implemented yet)");
            pauseMessageTimer_ = 2.2f;
            return;
        }

        if (action == PauseAction::MainMenu) {
            SetPaused(false);
            context.manager.SwitchTo("menu");
            return;
        }

        raylib::Window::Close();
    }

    void DrawPauseMenu(const raylib::Vector2& windowSize) const {
        DrawRectangle(0, 0, static_cast<int>(windowSize.x), static_cast<int>(windowSize.y),
                      raylib::Color{0, 0, 0, 115});

        const raylib::Rectangle panel = PausePanelRect(windowSize);
        DrawRectangleRounded(panel, 0.15f, 14, raylib::Color{12, 22, 30, 240});
        DrawRectangleRoundedLinesEx(panel, 0.15f, 14, 2.0f, raylib::Color{72, 190, 200, 180});

        raylib::Color{255, 235, 125, 255}.DrawText("PAUSED", static_cast<int>(panel.x) + 180,
                                                   static_cast<int>(panel.y) + 22, 42);
        raylib::Color{165, 185, 202, 255}.DrawText("Session Menu", static_cast<int>(panel.x) + 178,
                                                   static_cast<int>(panel.y) + 70, 24);

        for (int index = 0; index < static_cast<int>(pauseOptions_.size()); ++index) {
            const bool selected = index == pauseSelected_;
            const raylib::Rectangle row = PauseRowRect(panel, index);
            DrawRectangleRounded(row, 0.2f, 10,
                                 selected ? raylib::Color{34, 75, 86, 245} : raylib::Color{18, 34, 44, 210});
            (selected ? raylib::Color{126, 232, 244, 255} : raylib::Color{174, 196, 214, 255})
                .DrawText(pauseOptions_[static_cast<std::size_t>(index)], static_cast<int>(row.x) + 20,
                          static_cast<int>(row.y) + 8, 30);
        }

        const int hintSize = 18;
        const int hintY = static_cast<int>(panel.y + panel.height - 34.0f);
        if (pauseMessageTimer_ > 0.0f) {
            const char* message = "Save is not implemented yet.";
            const int messageX =
                static_cast<int>(panel.x + (panel.width - static_cast<float>(MeasureText(message, hintSize))) * 0.5f);
            raylib::Color{255, 208, 124, 255}.DrawText(message, messageX, hintY, hintSize);
        } else {
            const char* hint = "Up/Down or click | Enter/A | Esc/P resume";
            const int hintX =
                static_cast<int>(panel.x + (panel.width - static_cast<float>(MeasureText(hint, hintSize))) * 0.5f);
            raylib::Color{120, 145, 170, 255}.DrawText(hint, hintX, hintY, hintSize);
        }
    }

    [[nodiscard]] raylib::Rectangle PausePanelRect(const raylib::Vector2& windowSize) const {
        return {windowSize.x * 0.5f - 250.0f, windowSize.y * 0.5f - 170.0f, 500.0f, 360.0f};
    }

    [[nodiscard]] raylib::Rectangle PauseRowRect(const raylib::Rectangle& panel, int index) const {
        return {panel.x + 26.0f, panel.y + 120.0f + static_cast<float>(index) * 52.0f, panel.width - 52.0f, 44.0f};
    }

    [[nodiscard]] int PauseRowFromMouse(const raylib::Vector2& mousePosition, const raylib::Vector2& windowSize) const {
        const raylib::Rectangle panel = PausePanelRect(windowSize);
        for (int index = 0; index < static_cast<int>(pauseOptions_.size()); ++index) {
            const raylib::Rectangle row = PauseRowRect(panel, index);
            if (CheckCollisionPointRec(mousePosition, row)) {
                return index;
            }
        }

        return -1;
    }

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

        for (std::size_t i = 0; i < 42; ++i) {
            world_.entity()
                .set<Transform2D>({{posX(rng_), posY(rng_)}, {1.0f, 1.0f}, 0.0f})
                .set<Velocity>({{vel(rng_), vel(rng_)}})
                .set<Spin>({spin(rng_)})
                .set<Tint>({palette_[i % palette_.size()]})
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
    bool paused_ = false;
    int pauseSelected_ = 0;
    float pauseMessageTimer_ = 0.0f;
    bool draggingCamera_ = false;
    raylib::Vector2 lastMousePosition_{0.0f, 0.0f};
    raylib::Rectangle worldBounds_{-760.0f, -430.0f, 1520.0f, 860.0f};
    std::mt19937 rng_{std::random_device{}()};
    int spawnIndex_ = 0;
    std::array<const char*, 4> pauseOptions_{
        "Resume session",
        "Save session",
        "Return to main menu",
        "Quit to desktop",
    };
};
