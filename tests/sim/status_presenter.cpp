#include <cassert>

#include "client/render/status_presenter.hpp"

int main() {
    const client::components::StatusRenderState splash =
        client::render::BuildStatusRenderState(client::core::SceneKind::Splash, "", "");
    assert(splash.title == "Authoritative Multiplayer");
    assert(splash.subtitle == "raylib runtime bootstrap");
    assert(splash.footer == "Press Enter/Space to continue");

    const client::components::StatusRenderState connecting =
        client::render::BuildStatusRenderState(client::core::SceneKind::Connecting, "Connecting...", "");
    assert(connecting.title == "Joining Dedicated Server");
    assert(connecting.subtitle == "Connecting...");
    assert(connecting.footer == "Press Esc to cancel");

    const client::components::StatusRenderState disconnected =
        client::render::BuildStatusRenderState(client::core::SceneKind::Disconnected, "", "server closed");
    assert(disconnected.title == "Disconnected");
    assert(disconnected.subtitle == "server closed");
    assert(disconnected.footer == "Press Enter or Esc to return to menu");

    const client::components::StatusRenderState gameplay =
        client::render::BuildStatusRenderState(client::core::SceneKind::GameplayMultiplayer, "", "");
    assert(gameplay.title.empty());
    assert(gameplay.subtitle.empty());
    assert(gameplay.footer.empty());

    return 0;
}
