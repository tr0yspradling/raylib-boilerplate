#include "client/app/client_app.hpp"

#include <chrono>

namespace client::app {

ClientApp::ClientApp(ClientConfig config) : runtime_(std::move(config)) {}

bool ClientApp::Initialize() {
    world_.set<modules::ClientRuntimeRef>({.runtime = &runtime_});
    world_.set<components::WorldRenderState>({});
    world_.set<components::NetworkDebugState>({});
    world_.import<modules::ClientRuntimeModule>();

    initialized_ = runtime_.Initialize();
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
