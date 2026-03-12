#include "server/runtime/server_runtime.hpp"

#include <algorithm>
#include <cstdio>
#include <utility>

#include "server/runtime/server_runtime_ops.hpp"
#include "server/world_persistence.hpp"
#include "shared/net/net_policy.hpp"

namespace server {

namespace runtime {

ServerRuntime::ServerRuntime(ServerConfig config)
    : config_(std::move(config)),
      gameState_(config_.worldConfig, config_.playerKinematics),
      fixedStep_(1.0 / static_cast<double>(std::max(1, config_.simulationTickHz))) {}

bool ServerRuntime::Initialize() {
    if (config_.authMode == shared::net::AuthMode::DevInsecure) {
        authProvider_ = std::make_unique<shared::net::DevAuthProvider>();
    } else {
        authProvider_ = std::make_unique<shared::net::BackendTokenAuthProvider>();
    }

    std::string error;
    if (!transport_.Initialize(shared::net::TransportConfig{.isServer = true,
                                                            .debugVerbosity = shared::net::policy::kTransportDebugVerbosity,
                                                            .allowUnencryptedDev = shared::net::policy::kAllowUnencryptedDevTransport},
                               error)) {
        std::fprintf(stderr, "[net.transport] init failed: %s\n", error.c_str());
        return false;
    }

    if (!transport_.StartServer(config_.listenPort, error)) {
        std::fprintf(stderr, "[net.transport] listen failed on %u: %s\n", config_.listenPort, error.c_str());
        return false;
    }

    if (!LoadPersistence() && !persistenceWarning_.empty()) {
        std::fprintf(stderr, "[persistence.save] %s\n", persistenceWarning_.c_str());
    }

    lastMetricsLogAt_ = std::chrono::steady_clock::now();
    initialized_ = true;
    return true;
}

void ServerRuntime::Start() {
    if (!initialized_ || running_) {
        return;
    }
    running_ = true;

    std::fprintf(stderr, "[server] listening on %s:%u tick=%d snapshot=%d\n", config_.bindAddress.c_str(),
                 config_.listenPort, config_.simulationTickHz, config_.snapshotRateHz);
}

void ServerRuntime::Shutdown() {
    if (!initialized_) {
        return;
    }

    SavePersistence();
    transport_.Shutdown();
    running_ = false;
    initialized_ = false;
}

void ServerRuntime::RequestStop() { running_ = false; }

void ServerRuntime::PollTransportPump() {
    if (!initialized_) {
        return;
    }
    transport_.Poll();
}

void ServerRuntime::DecodeTransportMessages() {
    if (!initialized_) {
        return;
    }
    ServerRuntimeContext context = MakeContext();
    ServerRuntimeOps::HandleConnectionEvents(context);
    ServerRuntimeOps::HandleIncomingPackets(context);
}

void ServerRuntime::RunAuthAndSessionPhase() {}

void ServerRuntime::RunInputApplyPhase() {}

void ServerRuntime::AdvanceSimulation(float frameSeconds) {
    if (!running_) {
        return;
    }

    ServerRuntimeContext context = MakeContext();
    ServerRuntimeOps::AdvanceSimulation(context, frameSeconds);
}

void ServerRuntime::RunReplicationPhase() {
    ServerRuntimeContext context = MakeContext();
    ServerRuntimeOps::RunReplicationPhase(context);
}

void ServerRuntime::RunPersistencePhase() {}

void ServerRuntime::RunMetricsPhase() {
    ServerRuntimeContext context = MakeContext();
    ServerRuntimeOps::RunMetricsPhase(context, std::chrono::steady_clock::now());
}

bool ServerRuntime::IsRunning() const { return running_; }

bool ServerRuntime::LoadPersistence() {
    return LoadWorldState(config_.persistencePath, gameState_, persistenceWarning_);
}

void ServerRuntime::SavePersistence() {
    std::string error;
    if (!SaveWorldState(config_.persistencePath, gameState_, error)) {
        std::fprintf(stderr, "[persistence.save] save failed: %s\n", error.c_str());
        return;
    }

    std::fprintf(stderr, "[persistence.save] world state saved to %s\n", config_.persistencePath.c_str());
}

ServerRuntimeContext ServerRuntime::MakeContext() {
    return {
        .config = config_,
        .transport = transport_,
        .gameState = gameState_,
        .fixedStep = fixedStep_,
        .authProvider = authProvider_,
        .sessionsByConnection = sessionsByConnection_,
        .connectionByPlayer = connectionByPlayer_,
        .chunksByCoord = chunksByCoord_,
        .nextPlayerId = nextPlayerId_,
        .lastMetricsLogAt = lastMetricsLogAt_,
        .pendingReplicationSteps = pendingReplicationSteps_,
    };
}

}  // namespace runtime

}  // namespace server
