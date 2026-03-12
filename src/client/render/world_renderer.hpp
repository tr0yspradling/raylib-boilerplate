#pragma once

#include <raylib-cpp.hpp>

#include "client/components/components.hpp"
#include "client/render/render_policy.hpp"
#include "client/render/render_theme.hpp"

namespace client::render {

class WorldRenderer {
public:
    static void Draw(const components::WorldRenderState& world, int width, int height) {
        const int groundY = static_cast<int>(static_cast<float>(height) * policy::world::kGroundHeightRatio);
        DrawLine(0, groundY, width, groundY, theme::world::kGroundLine);

        if (world.localPlayer.playerId.IsValid()) {
            const raylib::Vector2 position = ToScreen(world.localPlayer.position, width, height);
            DrawCircleV(position, policy::world::kPlayerRadius, theme::world::kLocalPlayerFill);
            theme::world::kLocalPlayerText.DrawText(
                world.localPlayer.displayName.c_str(), static_cast<int>(position.x) - policy::world::kPlayerNameCenterOffset,
                static_cast<int>(position.y) - policy::world::kPlayerNameVerticalOffset, policy::world::kLabelFontSize);
        }

        for (const components::PlayerRenderState& remote : world.remotePlayers) {
            const raylib::Vector2 position = ToScreen(remote.position, width, height);
            DrawCircleV(position, policy::world::kPlayerRadius, theme::world::kRemotePlayerFill);
            theme::world::kRemotePlayerText.DrawText(
                remote.displayName.c_str(), static_cast<int>(position.x) - policy::world::kPlayerNameCenterOffset,
                static_cast<int>(position.y) - policy::world::kPlayerNameVerticalOffset, policy::world::kLabelFontSize);
        }

        theme::world::kControlsText.DrawText(std::string{policy::world::kControlsHint}.c_str(),
                                             policy::world::kControlsLeftMargin,
                                             height - policy::world::kControlsBottomMargin,
                                             policy::world::kControlsFontSize);
    }

private:
    [[nodiscard]] static raylib::Vector2 ToScreen(const shared::game::Vec2f& worldPosition, int width, int height) {
        const float centerX = static_cast<float>(width) * 0.5f;
        const float groundY = static_cast<float>(height) * policy::world::kGroundHeightRatio;
        return {
            centerX + worldPosition.x * policy::world::kPixelsPerWorldUnit,
            groundY - worldPosition.y * policy::world::kPixelsPerWorldUnit,
        };
    }
};

}  // namespace client::render
