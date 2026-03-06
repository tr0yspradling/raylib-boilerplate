#pragma once

#include <algorithm>

#include <raylib-cpp.hpp>

#include "client/components/components.hpp"
#include "client/ui/debug_overlay.hpp"
#include "client/ui/ui_document.hpp"
#include "client/ui/ui_renderer.hpp"

namespace client::systems {

class RenderSystem {
public:
    static void DrawFrame(const components::WorldRenderState& world, const ui::UiDocument& document,
                          const components::NetworkDebugState& debug,
                          bool showDebugOverlay) {
        const int width = raylib::Window::GetWidth();
        const int height = raylib::Window::GetHeight();

        DrawBackground(width, height);

        switch (debug.activeScene) {
        case core::SceneKind::Splash:
            DrawSplash(width, height);
            break;
        case core::SceneKind::MainMenu:
        case core::SceneKind::JoinServer:
            ui::UiRenderer::Draw(document, width, height);
            break;
        case core::SceneKind::Connecting:
            DrawCenteredStatus("Joining Dedicated Server",
                               debug.runtimeStatusMessage.empty() ? "Connecting to authoritative host..."
                                                                  : debug.runtimeStatusMessage.c_str(),
                               width, height);
            raylib::Color{168, 196, 214, 255}.DrawText("Press Esc to cancel", width / 2 - 88, height / 2 + 66, 20);
            break;
        case core::SceneKind::StartingServer:
            DrawCenteredStatus("Start Server (Placeholder)",
                               debug.runtimeStatusMessage.empty() ? "Not implemented in this slice"
                                                                  : debug.runtimeStatusMessage.c_str(),
                               width, height);
            break;
        case core::SceneKind::GameplaySingleplayer:
            DrawCenteredStatus("Singleplayer (Placeholder)",
                               debug.runtimeStatusMessage.empty() ? "Not implemented in this slice"
                                                                  : debug.runtimeStatusMessage.c_str(),
                               width, height);
            break;
        case core::SceneKind::Options:
            DrawCenteredStatus("Options (Placeholder)",
                               debug.runtimeStatusMessage.empty() ? "Not implemented in this slice"
                                                                  : debug.runtimeStatusMessage.c_str(),
                               width, height);
            break;
        case core::SceneKind::Disconnected:
            DrawCenteredStatus("Disconnected",
                               debug.disconnectReason.empty() ? "Connection closed" : debug.disconnectReason.c_str(),
                               width, height);
            raylib::Color{196, 212, 224, 255}.DrawText("Press Enter or Esc to return to menu", width / 2 - 210,
                                                       height / 2 + 72, 20);
            break;
        case core::SceneKind::GameplayMultiplayer:
            DrawGameplay(world, width, height);
            break;
        }

        if (showDebugOverlay) {
            ui::DebugOverlay::Draw(debug);
        }
    }

private:
    static void DrawBackground(int width, int height) {
        raylib::Color{16, 22, 28, 255}.ClearBackground();

        const int topBandHeight = std::max(80, height / 6);
        DrawRectangleGradientV(0, 0, width, topBandHeight, raylib::Color{28, 44, 62, 190}, raylib::Color{16, 22, 28, 0});
    }

    static void DrawSplash(int width, int height) {
        raylib::Color{240, 248, 255, 255}.DrawText("Authoritative Multiplayer", width / 2 - 210, height / 2 - 28, 44);
        raylib::Color{188, 212, 228, 255}.DrawText("raylib runtime bootstrap", width / 2 - 132, height / 2 + 22, 26);
        raylib::Color{156, 184, 204, 255}.DrawText("Press Enter/Space to continue", width / 2 - 145, height / 2 + 70,
                                                   20);
    }

    static void DrawCenteredStatus(const char* title, const char* subtitle, int width, int height) {
        raylib::Color{236, 246, 255, 255}.DrawText(title, width / 2 - 220, height / 2 - 34, 46);
        raylib::Color{184, 208, 226, 255}.DrawText(subtitle, width / 2 - 260, height / 2 + 22, 24);
    }

    static void DrawGameplay(const components::WorldRenderState& world, int width, int height) {
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
    }

    [[nodiscard]] static raylib::Vector2 ToScreen(const shared::game::Vec2f& worldPosition, int width, int height) {
        constexpr float kPixelsPerUnit = 52.0f;
        const float centerX = static_cast<float>(width) * 0.5f;
        const float groundY = static_cast<float>(height) * 0.78f;
        return {centerX + worldPosition.x * kPixelsPerUnit, groundY - worldPosition.y * kPixelsPerUnit};
    }
};

}  // namespace client::systems
