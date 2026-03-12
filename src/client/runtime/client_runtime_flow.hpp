#pragma once

#include <string>

#include "client/runtime/client_runtime_context.hpp"

namespace client::runtime {

class ClientRuntimeFlowController {
public:
    static void InitializeWorldState(ClientRuntimeContext& context);
    static void ProcessRuntimeIntent(ClientRuntimeContext& context);
    static void RefreshRuntimeState(ClientRuntimeContext& context);
    static void PublishScreenState(ClientRuntimeContext& context);
    static void ReturnToMenu(ClientRuntimeContext& context, std::string statusMessage = {});
};

}  // namespace client::runtime
