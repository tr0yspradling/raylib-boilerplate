#pragma once

#include <algorithm>
#include <array>
#include <string>

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

        DrawBackground(width, height);

        switch (debug.activeScene) {
        case core::SceneKind::Splash:
            DrawSplash(width, height);
            break;
        case core::SceneKind::MainMenu:
            DrawMenu(debug, width, height);
            break;
        case core::SceneKind::JoinServer:
            DrawJoinForm(debug, width, height);
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

    static void DrawMenu(const components::NetworkDebugState& debug, int width, int height) {
        raylib::Color{238, 246, 255, 255}.DrawText("Main Menu", width / 2 - 120, 120, 56);
        raylib::Color{168, 194, 214, 255}.DrawText("Select runtime mode", width / 2 - 104, 182, 24);

        int menuY = 260;
        for (size_t i = 0; i < debug.menuActions.size(); ++i) {
            const bool selected = i == debug.selectedMenuIndex;
            const raylib::Color textColor = selected ? raylib::Color{32, 52, 72, 255} : raylib::Color{214, 228, 240, 255};
            const raylib::Color itemColor = selected ? raylib::Color{118, 198, 255, 255} : raylib::Color{46, 62, 78, 255};

            DrawRectangleRounded({static_cast<float>(width / 2 - 210), static_cast<float>(menuY - 6), 420.0f, 44.0f},
                                 0.22f, 12, itemColor);
            textColor.DrawText(debug.menuActions[i].c_str(), width / 2 - 180, menuY, 28);
            menuY += 56;
        }

        if (!debug.runtimeStatusMessage.empty()) {
            raylib::Color{255, 209, 140, 255}.DrawText(debug.runtimeStatusMessage.c_str(), width / 2 - 300, menuY + 8, 20);
        }

        raylib::Color{168, 196, 214, 255}.DrawText("Navigate: W/S or Up/Down | Select: Enter/Space (A)", width / 2 - 260,
                                                   height - 62, 20);
    }

    static void DrawCenteredStatus(const char* title, const char* subtitle, int width, int height) {
        raylib::Color{236, 246, 255, 255}.DrawText(title, width / 2 - 220, height / 2 - 34, 46);
        raylib::Color{184, 208, 226, 255}.DrawText(subtitle, width / 2 - 260, height / 2 + 22, 24);
    }

    static void DrawJoinForm(const components::NetworkDebugState& debug, int width, int height) {
        raylib::Color{238, 246, 255, 255}.DrawText("Join Server", width / 2 - 126, 112, 56);
        raylib::Color{168, 194, 214, 255}.DrawText("Configure host, port, and player name", width / 2 - 190, 176, 24);

        std::array<std::string, 5> rows{
            "Host: " + (debug.joinHost.empty() ? std::string{"<required>"} : debug.joinHost),
            "Port: " + (debug.joinPort.empty() ? std::string{"<required>"} : debug.joinPort),
            "Name: " + (debug.joinPlayerName.empty() ? std::string{"<required>"} : debug.joinPlayerName),
            "Connect",
            "Back",
        };

        if (debug.joinEditing && debug.selectedJoinFieldIndex < 3U) {
            rows[debug.selectedJoinFieldIndex] += "_";
        }

        int rowY = 250;
        for (size_t i = 0; i < rows.size(); ++i) {
            const bool selected = i == debug.selectedJoinFieldIndex;
            const raylib::Color textColor = selected ? raylib::Color{32, 52, 72, 255} : raylib::Color{214, 228, 240, 255};
            const raylib::Color itemColor = selected ? raylib::Color{118, 198, 255, 255} : raylib::Color{46, 62, 78, 255};

            DrawRectangleRounded({static_cast<float>(width / 2 - 260), static_cast<float>(rowY - 6), 520.0f, 44.0f}, 0.22f,
                                 12, itemColor);
            textColor.DrawText(rows[i].c_str(), width / 2 - 230, rowY, 28);
            rowY += 56;
        }

        if (!debug.runtimeStatusMessage.empty()) {
            raylib::Color{255, 209, 140, 255}.DrawText(debug.runtimeStatusMessage.c_str(), width / 2 - 300, rowY + 8, 20);
        }

        const char* controls = debug.joinEditing
            ? "Editing: type text, Backspace to erase, Enter/Esc to finish"
            : "Navigate: W/S or Up/Down | Select: Enter/Space (A) | Back: Esc (B)";
        raylib::Color{168, 196, 214, 255}.DrawText(controls, width / 2 - 330, height - 62, 20);
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
