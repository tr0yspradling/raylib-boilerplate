#pragma once

#include "client/runtime/client_runtime_context.hpp"

namespace client::runtime {

class ClientPresentationBuilder {
public:
    static void Publish(ClientRuntimeContext& context, float frameSeconds);
};

}  // namespace client::runtime
