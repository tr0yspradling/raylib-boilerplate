#pragma once

#include <flecs.h>

#include "server/config/server_config.hpp"
#include "server/modules/server_runtime_module.hpp"
#include "server/runtime/server_runtime.hpp"

namespace server::app {

class ServerApp {
public:
    explicit ServerApp(ServerConfig config);

    [[nodiscard]] bool Initialize();
    int Run();
    void RequestStop();

private:
    flecs::world world_;
    runtime::ServerRuntime runtime_;
    bool initialized_ = false;
};

}  // namespace server::app
