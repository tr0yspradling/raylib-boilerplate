#pragma once

#include <chrono>

#include "server/runtime/server_runtime_context.hpp"

namespace server::runtime {

class ServerRuntimeOps {
public:
    static void HandleConnectionEvents(ServerRuntimeContext& context);
    static void HandleIncomingPackets(ServerRuntimeContext& context);
    static void AdvanceSimulation(ServerRuntimeContext& context, float frameSeconds);
    static void RunReplicationPhase(ServerRuntimeContext& context);
    static void RunMetricsPhase(ServerRuntimeContext& context, std::chrono::steady_clock::time_point now);
};

}  // namespace server::runtime
