#pragma once

#include <raylib-cpp.hpp>

#include "client/components/components.hpp"
#include "client/render/background_renderer.hpp"
#include "client/render/splash_renderer.hpp"
#include "client/render/status_renderer.hpp"
#include "client/render/world_renderer.hpp"
#include "client/ui/debug_overlay.hpp"
#include "client/ui/ui_document.hpp"
#include "client/ui/ui_renderer.hpp"

namespace client::systems {

class RenderSystem {
public:
    static void DrawFrame(const components::WorldRenderState& world, const ui::UiDocument& document,
                          const components::StatusRenderState& status,
                          const components::NetworkDebugState& debug,
                          bool showDebugOverlay) {
        const int width = raylib::Window::GetWidth();
        const int height = raylib::Window::GetHeight();

        render::BackgroundRenderer::Draw(width, height);

        switch (debug.activeScene) {
        case core::SceneKind::Splash:
            render::SplashRenderer::Draw(status, width, height);
            break;
        case core::SceneKind::MainMenu:
        case core::SceneKind::JoinServer:
            ui::UiRenderer::Draw(document, width, height);
            break;
        case core::SceneKind::Connecting:
        case core::SceneKind::StartingServer:
        case core::SceneKind::GameplaySingleplayer:
        case core::SceneKind::Options:
        case core::SceneKind::Disconnected:
            render::StatusRenderer::Draw(status, width, height);
            break;
        case core::SceneKind::GameplayMultiplayer:
            render::WorldRenderer::Draw(world, width, height);
            break;
        }

        if (showDebugOverlay) {
            ui::DebugOverlay::Draw(debug);
        }
    }
};

}  // namespace client::systems
