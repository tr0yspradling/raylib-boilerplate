#pragma once

#include <algorithm>
#include <array>
#include <string>

#include "../core/Scene.hpp"

class MenuScene : public Scene {
public:
    void OnEnter([[maybe_unused]] SceneContext& context) override {
        selected_ = 0;
        blinkTimer_ = 0.0f;
    }

    void Update(SceneContext& context, float dt) override {
        blinkTimer_ += dt;

        if (context.input.IsPressed(KEY_UP)) {
            selected_ = std::max(0, selected_ - 1);
        }
        if (context.input.IsPressed(KEY_DOWN)) {
            selected_ = std::min(static_cast<int>(options_.size()) - 1, selected_ + 1);
        }

        if (context.input.SelectPressed()) {
            if (selected_ == 0) {
                context.manager.SwitchTo("sandbox");
            } else {
                raylib::Window::Close();
            }
        }

        if (context.input.QuitRequested()) {
            raylib::Window::Close();
        }
    }

    void Draw(SceneContext& context) override {
        const raylib::Vector2 size = context.window.GetSize();
        const raylib::Vector2 center = {size.x * 0.5f, size.y * 0.5f};
        const raylib::Color titleColor{240, 248, 255, 255};
        const raylib::Color bodyColor{180, 200, 220, 255};
        const raylib::Color optionColor{125, 145, 165, 255};
        const raylib::Color selectedColor{72, 190, 200, 255};

        titleColor.DrawText("raylib boilerplate", static_cast<int>(center.x) - 300, static_cast<int>(center.y) - 230,
                            64);
        bodyColor.DrawText("A slightly more complete starter scene flow", static_cast<int>(center.x) - 300,
                           static_cast<int>(center.y) - 170, 28);

        const int itemHeight = 56;
        const int itemWidth = 520;
        const int startY = static_cast<int>(center.y) - 70;
        for (int index = 0; index < static_cast<int>(options_.size()); ++index) {
            const bool isSelected = index == selected_;
            const raylib::Rectangle rowRect{center.x - static_cast<float>(itemWidth) * 0.5f,
                                            static_cast<float>(startY + index * itemHeight),
                                            static_cast<float>(itemWidth), static_cast<float>(itemHeight - 8)};

            DrawRectangleRounded(rowRect, 0.18f, 10,
                                 isSelected ? raylib::Color{30, 64, 74, 230} : raylib::Color{17, 27, 38, 180});
            (isSelected ? selectedColor : optionColor)
                .DrawText(options_[static_cast<std::size_t>(index)], static_cast<int>(rowRect.x) + 24,
                          static_cast<int>(rowRect.y) + 12, 30);
        }

        if (std::sin(blinkTimer_ * 4.0f) > -0.1f) {
            raylib::Color{220, 230, 240, 255}.DrawText("Use Up/Down + Enter (or gamepad A)",
                                                       static_cast<int>(center.x) - 235,
                                                       static_cast<int>(center.y) + 110, 22);
        }

        raylib::Color{120, 145, 170, 255}.DrawText("Tab debug  |  Ctrl+F fullscreen  |  P pause (in sandbox)",
                                                   static_cast<int>(center.x) - 300, static_cast<int>(center.y) + 170,
                                                   20);
    }

    [[nodiscard]] bool AllowsPause() const override { return false; }
    [[nodiscard]] std::string_view Name() const override { return "Menu"; }

private:
    std::array<const char*, 2> options_{
        "Play sandbox",
        "Quit to desktop",
    };
    int selected_ = 0;
    float blinkTimer_ = 0.0f;
};
