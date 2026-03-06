#pragma once

#include <raylib-cpp.hpp>

#include "client/components/components.hpp"

namespace client::render {

class StatusRenderer {
public:
    static void Draw(const components::StatusRenderState& statusState, int width, int height) {
        if (statusState.title.empty()) {
            return;
        }

        raylib::Color{236, 246, 255, 255}.DrawText(statusState.title.c_str(), width / 2 - 220, height / 2 - 34, 46);
        raylib::Color{184, 208, 226, 255}.DrawText(statusState.subtitle.c_str(), width / 2 - 260, height / 2 + 22, 24);
        if (!statusState.footer.empty()) {
            raylib::Color{196, 212, 224, 255}.DrawText(statusState.footer.c_str(), width / 2 - 210, height / 2 + 72, 20);
        }
    }
};

}  // namespace client::render
