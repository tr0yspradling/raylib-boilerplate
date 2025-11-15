#pragma once

#include <array>
#include <cstdio>

#include <raylib-cpp.hpp>

#include "../core/SceneManager.hpp"

namespace ui {

class DebugOverlay {
public:
    static void Draw(SceneManager& manager, float dt) {
        const Scene* scene = manager.ActiveScene();
        const char* sceneName = scene != nullptr ? scene->Name().data() : "None";

        std::array<char, 128> text{};
        std::snprintf(text.data(), text.size(), "Scene: %s | FPS: %d | dt: %.2f ms",
                      sceneName, raylib::Window::GetFPS(), dt * 1000.0f);

        raylib::Color{255, 255, 255, 255}.DrawText(text.data(), 16, 16, 20);
    }
};

}  // namespace ui
