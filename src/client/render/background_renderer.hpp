#pragma once

#include <algorithm>
#include <raylib-cpp.hpp>

#include "client/render/render_policy.hpp"
#include "client/ui/ui_policy.hpp"

namespace client::render {

class BackgroundRenderer {
public:
    static void Draw(int width, int height) {
        ClearBackground(ui::policy::color::kBackgroundClear);

        const int topBandHeight =
            std::max(policy::background::kMinimumTopBandHeight, height / policy::background::kTopBandDivisor);
        DrawRectangleGradientV(0, 0, width, topBandHeight, ui::policy::color::kBackgroundGradientTop,
                               ui::policy::color::kBackgroundGradientBottom);
    }
};

}  // namespace client::render
