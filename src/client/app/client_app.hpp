#pragma once

#include <flecs.h>

#include "client/components/components.hpp"
#include "client/core/client_config.hpp"
#include "client/modules/client_runtime_module.hpp"
#include "client/runtime/client_runtime.hpp"
#include "client/ui/ui_document.hpp"
#include "client/ui/ui_state.hpp"

namespace client::app {

class ClientApp {
public:
    explicit ClientApp(ClientConfig config);

    [[nodiscard]] bool Initialize();
    int Run();

private:
    flecs::world world_;
    runtime::ClientRuntime runtime_;
    bool initialized_ = false;
};

}  // namespace client::app
