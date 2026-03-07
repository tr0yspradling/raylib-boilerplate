#include "client/app/client_app.hpp"

#include <chrono>

namespace client::app {

ClientApp::ClientApp(ClientConfig config) : runtime_(std::move(config)) {}

bool ClientApp::Initialize() {
    world_.set<modules::ClientRuntimeRef>({.runtime = &runtime_});
    world_.set<components::WorldRenderState>({});
    world_.set<components::StatusRenderState>({});
    world_.set<components::NetworkDebugState>({});
    world_.set<ui::UiDocument>({});
    world_.set<ui::UiInputState>({});
    world_.set<ui::UiInteractionState>({});
    world_.set<ui::UiCommandQueue>({});
    world_.set<ui::MenuScreenState>({});
    world_.set<ui::JoinServerScreenState>({});
    world_.set<ui::ScreenState>({});
    world_.set<runtime::ClientFlowState>({});
    world_.set<runtime::LocalServerStartupState>({});
    world_.set<runtime::ClientSessionState>({});
    world_.import<modules::ClientRuntimeModule>();

    initialized_ = runtime_.Initialize(world_);
    return initialized_;
}

int ClientApp::Run() {
    if (!initialized_) {
        return 1;
    }

    using clock = std::chrono::steady_clock;
    auto lastFrameAt = clock::now();

    while (!runtime_.ShouldExit()) {
        const auto now = clock::now();
        const std::chrono::duration<double> frameDelta = now - lastFrameAt;
        lastFrameAt = now;
        world_.progress(static_cast<ecs_ftime_t>(frameDelta.count()));
    }

    runtime_.Shutdown();
    return 0;
}

}  // namespace client::app
