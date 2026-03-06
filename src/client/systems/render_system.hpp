#pragma once

#include <raylib-cpp.hpp>

#include "client/components/components.hpp"
#include "client/ui/debug_overlay.hpp"

namespace client::systems {

class RenderSystem {
public:
    static void DrawFrame(const components::WorldRenderState& world, const components::NetworkDebugState& debug,
                          bool showDebugOverlay) {
        const int width = raylib::Window::GetWidth();
        const int height = raylib::Window::GetHeight();

        raylib::Color{16, 22, 28, 255}.ClearBackground();

        const int groundY = static_cast<int>(static_cast<float>(height) * 0.78f);
        DrawLine(0, groundY, width, groundY, raylib::Color{95, 120, 140, 255});

        if (world.localPlayer.playerId.IsValid()) {
            const raylib::Vector2 position = ToScreen(world.localPlayer.position, width, height);
            DrawCircleV(position, 18.0f, raylib::Color{85, 220, 120, 255});
            raylib::Color{210, 250, 220, 255}.DrawText(world.localPlayer.displayName.c_str(),
                                                        static_cast<int>(position.x) - 30,
                                                        static_cast<int>(position.y) - 36, 18);
        }

        for (const components::PlayerRenderState& remote : world.remotePlayers) {
            const raylib::Vector2 position = ToScreen(remote.position, width, height);
            DrawCircleV(position, 18.0f, raylib::Color{90, 160, 245, 255});
            raylib::Color{220, 235, 255, 255}.DrawText(remote.displayName.c_str(), static_cast<int>(position.x) - 30,
                                                        static_cast<int>(position.y) - 36, 18);
        }

        raylib::Color{140, 165, 185, 255}.DrawText("Move: A/D or arrows | Jump: Space | Tab net overlay", 20,
                                                   height - 36, 20);

        if (showDebugOverlay) {
            ui::DebugOverlay::Draw(debug);
        }
    }

private:
    [[nodiscard]] static raylib::Vector2 ToScreen(const shared::game::Vec2f& worldPosition, int width, int height) {
        constexpr float kPixelsPerUnit = 52.0f;
        const float centerX = static_cast<float>(width) * 0.5f;
        const float groundY = static_cast<float>(height) * 0.78f;
        return {centerX + worldPosition.x * kPixelsPerUnit, groundY - worldPosition.y * kPixelsPerUnit};
    }
};

}  // namespace client::systems
