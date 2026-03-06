#pragma once

#include <flecs.h>

#include "server/modules/server_phases.hpp"
#include "server/runtime/server_runtime.hpp"

namespace server::modules {

struct ServerRuntimeRef {
    runtime::ServerRuntime* runtime = nullptr;
};

inline runtime::ServerRuntime* ResolveServerRuntime(flecs::world world) {
    ServerRuntimeRef& ref = world.get_mut<ServerRuntimeRef>();
    return ref.runtime;
}

struct ServerRuntimeModule {
    explicit ServerRuntimeModule(flecs::world& world) {
        world.module<ServerRuntimeModule>();
        RegisterServerPhases(world);
        world.component<ServerRuntimeRef>();

        world.system("ServerTransportPoll")
            .kind(EnsureTransportPollPhase(world))
            .run([](flecs::iter& it) {
                if (auto* runtime = ResolveServerRuntime(it.world())) {
                    runtime->PollTransportPump();
                }
            });

        world.system("ServerMessageDecode")
            .kind(EnsureMessageDecodePhase(world))
            .run([](flecs::iter& it) {
                if (auto* runtime = ResolveServerRuntime(it.world())) {
                    runtime->DecodeTransportMessages();
                }
            });

        world.system("ServerAuthAndSession")
            .kind(EnsureAuthAndSessionPhase(world))
            .run([](flecs::iter& it) {
                if (auto* runtime = ResolveServerRuntime(it.world())) {
                    runtime->RunAuthAndSessionPhase();
                }
            });

        world.system("ServerInputApply")
            .kind(EnsureInputApplyPhase(world))
            .run([](flecs::iter& it) {
                if (auto* runtime = ResolveServerRuntime(it.world())) {
                    runtime->RunInputApplyPhase();
                }
            });

        world.system("ServerSimulation")
            .kind(EnsureSimulationPhase(world))
            .run([](flecs::iter& it) {
                if (auto* runtime = ResolveServerRuntime(it.world())) {
                    runtime->AdvanceSimulation(static_cast<float>(it.delta_time()));
                }
            });

        world.system("ServerReplication")
            .kind(EnsureReplicationPhase(world))
            .run([](flecs::iter& it) {
                if (auto* runtime = ResolveServerRuntime(it.world())) {
                    runtime->RunReplicationPhase();
                }
            });

        world.system("ServerPersistence")
            .kind(EnsurePersistencePhase(world))
            .run([](flecs::iter& it) {
                if (auto* runtime = ResolveServerRuntime(it.world())) {
                    runtime->RunPersistencePhase();
                }
            });

        world.system("ServerMetrics")
            .kind(EnsureMetricsPhase(world))
            .run([](flecs::iter& it) {
                if (auto* runtime = ResolveServerRuntime(it.world())) {
                    runtime->RunMetricsPhase();
                }
            });
    }
};

}  // namespace server::modules
