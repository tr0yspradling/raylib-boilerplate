#pragma once

#include <string>

#include "../core/Scene.hpp"

class MenuScene : public Scene {
public:
    void Update(SceneContext& context, [[maybe_unused]] float dt) override {
        if (context.input.IsPressed(KEY_SPACE) || context.input.IsPressed(KEY_ENTER)) {
            context.manager.SwitchTo("sandbox");
        }
    }

    void Draw(SceneContext& context) override {
        const raylib::Vector2 size = context.window.GetSize();
        const raylib::Vector2 center = {size.x * 0.5f, size.y * 0.5f};

        const int titleSize = 64;
        const int bodySize = 24;

        raylib::Color{240, 248, 255, 255}.DrawText("raylib boilerplate", static_cast<int>(center.x) - 300,
                                                    static_cast<int>(center.y) - 80, titleSize);
        raylib::Color{180, 200, 220, 255}.DrawText("Press Space to open the sandbox scene",
                                                   static_cast<int>(center.x) - 260,
                                                   static_cast<int>(center.y) + 10, bodySize);
    }

    [[nodiscard]] std::string_view Name() const override { return "Menu"; }
};
