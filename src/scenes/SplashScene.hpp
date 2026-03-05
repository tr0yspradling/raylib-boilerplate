#pragma once

#include <algorithm>
#include <cmath>
#include <string_view>

#include "../core/Scene.hpp"

class SplashScene : public Scene {
public:
    void OnEnter([[maybe_unused]] SceneContext& context) override { elapsed_ = 0.0f; }

    void Update(SceneContext& context, float dt) override {
        elapsed_ += dt;
        if (elapsed_ >= 1.6f || context.input.SelectPressed()) {
            context.manager.SwitchTo("menu");
        }
    }

    void Draw(SceneContext& context) override {
        const raylib::Vector2 size = context.window.GetSize();
        const float t = std::clamp(elapsed_ / 1.6f, 0.0f, 1.0f);
        const float pulse = std::sin(elapsed_ * 5.0f) * 0.5f + 0.5f;

        const int orbRadius = static_cast<int>(120.0f + pulse * 80.0f);
        const raylib::Vector2 center{size.x * 0.5f, size.y * 0.5f - 10.0f};
        DrawCircleGradient(static_cast<int>(center.x), static_cast<int>(center.y), static_cast<float>(orbRadius),
                           raylib::Color{30, 205, 210, 180}, raylib::Color{20, 24, 28, 0});

        const int titleSize = 64;
        const int hintSize = 24;
        const raylib::Color titleColor{static_cast<unsigned char>(180.0f + 70.0f * (1.0f - t)),
                                       static_cast<unsigned char>(220.0f + 35.0f * (1.0f - t)), 255, 255};
        const raylib::Color hintColor{static_cast<unsigned char>(130.0f + 80.0f * t),
                                      static_cast<unsigned char>(165.0f + 60.0f * t),
                                      static_cast<unsigned char>(200.0f + 40.0f * t), 255};

        titleColor.DrawText("raylib boilerplate", static_cast<int>(center.x) - 305, static_cast<int>(center.y) - 35,
                            titleSize);
        hintColor.DrawText("Press Enter / Space / A to continue", static_cast<int>(center.x) - 225,
                           static_cast<int>(center.y) + 75, hintSize);
    }

    [[nodiscard]] std::string_view Name() const override { return "Splash"; }
    [[nodiscard]] bool AllowsPause() const override { return false; }

private:
    float elapsed_ = 0.0f;
};
