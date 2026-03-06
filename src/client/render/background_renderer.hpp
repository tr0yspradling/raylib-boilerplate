#pragma once

#include <algorithm>

#include <raylib-cpp.hpp>

namespace client::render {

class BackgroundRenderer {
public:
    static void Draw(int width, int height) {
        raylib::Color{16, 22, 28, 255}.ClearBackground();

        const int topBandHeight = std::max(80, height / 6);
        DrawRectangleGradientV(0, 0, width, topBandHeight, raylib::Color{28, 44, 62, 190},
                               raylib::Color{16, 22, 28, 0});
    }
};

}  // namespace client::render
