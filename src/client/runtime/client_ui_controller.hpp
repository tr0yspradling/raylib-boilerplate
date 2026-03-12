#pragma once

#include "client/runtime/client_runtime_context.hpp"

namespace client::runtime {

class ClientUiController {
public:
    static void HandleUiInteraction(ClientRuntimeContext& context);
};

}  // namespace client::runtime
