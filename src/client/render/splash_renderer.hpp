#pragma once

#include <raylib-cpp.hpp>

#include "client/components/components.hpp"

namespace client::render {

class SplashRenderer {
public:
    static void Draw(const components::StatusRenderState& splashState, int width, int height) {
        raylib::Color{240, 248, 255, 255}.DrawText(splashState.title.c_str(), width / 2 - 210, height / 2 - 28, 44);
        raylib::Color{188, 212, 228, 255}.DrawText(splashState.subtitle.c_str(), width / 2 - 132, height / 2 + 22, 26);
        if (!splashState.footer.empty()) {
            raylib::Color{156, 184, 204, 255}.DrawText(splashState.footer.c_str(), width / 2 - 145, height / 2 + 70, 20);
        }
    }
};

}  // namespace client::render
