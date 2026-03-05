#pragma once

#include <array>
#include <cstdio>
#include <raylib-cpp.hpp>

#include "../core/SceneManager.hpp"

namespace ui {

struct DebugOverlayState {
    bool paused = false;
    float rawDtMs = 0.0f;
    float simDtMs = 0.0f;
    raylib::Vector2 moveAxis{0.0f, 0.0f};
    float mouseWheel = 0.0f;
    int activeGamepad = -1;
};

class DebugOverlay {
public:
    static void Draw(SceneManager& manager, const DebugOverlayState& state) {
        const Scene* scene = manager.ActiveScene();
        const char* sceneName = scene != nullptr ? scene->Name().data() : "None";

        DrawRectangleRounded({12.0f, 10.0f, 640.0f, 106.0f}, 0.14f, 12, raylib::Color{0, 0, 0, 140});

        std::array<char, 256> lineOne{};
        std::array<char, 256> lineTwo{};
        std::array<char, 256> lineThree{};
        std::snprintf(lineOne.data(), lineOne.size(), "Scene: %s | FPS: %d | Paused: %s", sceneName,
                      raylib::Window::GetFPS(), state.paused ? "yes" : "no");
        std::snprintf(lineTwo.data(), lineTwo.size(), "dt(raw): %.2f ms | dt(sim): %.2f ms | Wheel: %.2f",
                      state.rawDtMs, state.simDtMs, state.mouseWheel);
        std::snprintf(lineThree.data(), lineThree.size(), "Axis: [%.2f, %.2f] | Gamepad: %s", state.moveAxis.x,
                      state.moveAxis.y, state.activeGamepad >= 0 ? "connected" : "none");

        raylib::Color{255, 255, 255, 255}.DrawText(lineOne.data(), 20, 18, 20);
        raylib::Color{205, 220, 235, 255}.DrawText(lineTwo.data(), 20, 46, 20);
        raylib::Color{205, 220, 235, 255}.DrawText(lineThree.data(), 20, 74, 20);
    }
};

}  // namespace ui
