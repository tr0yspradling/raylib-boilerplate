#include "server/app/server_app.hpp"

#include <chrono>
#include <thread>

namespace server::app {

ServerApp::ServerApp(ServerConfig config) : runtime_(std::move(config)) {}

bool ServerApp::Initialize() {
    world_.set<modules::ServerRuntimeRef>({.runtime = &runtime_});
    world_.import<modules::ServerRuntimeModule>();

    initialized_ = runtime_.Initialize();
    return initialized_;
}

int ServerApp::Run() {
    if (!initialized_) {
        return 1;
    }

    using clock = std::chrono::steady_clock;
    auto lastFrameAt = clock::now();
    runtime_.Start();

    while (runtime_.IsRunning()) {
        const auto now = clock::now();
        const std::chrono::duration<double> frameDelta = now - lastFrameAt;
        lastFrameAt = now;
        world_.progress(static_cast<ecs_ftime_t>(frameDelta.count()));
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    runtime_.Shutdown();
    return 0;
}

void ServerApp::RequestStop() { runtime_.RequestStop(); }

}  // namespace server::app
