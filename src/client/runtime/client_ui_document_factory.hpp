#pragma once

#include "client/runtime/client_runtime_context.hpp"

namespace client::runtime {

class ClientUiDocumentFactory {
public:
    static void BuildPublishedDocument(ClientRuntimeContext& context);
};

}  // namespace client::runtime
