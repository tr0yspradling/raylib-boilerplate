#pragma once

#include <flecs.h>

namespace server::modules {

struct TransportPollPhase {};
struct MessageDecodePhase {};
struct AuthAndSessionPhase {};
struct InputApplyPhase {};
struct SimulationPhase {};
struct ReplicationPhase {};
struct PersistencePhase {};
struct MetricsPhase {};

inline flecs::entity EnsureTransportPollPhase(flecs::world& world) {
    auto phase = world.entity<TransportPollPhase>("server.phases.TransportPoll");
    phase.add(flecs::Phase);
    phase.depends_on(flecs::OnUpdate);
    return phase;
}

inline flecs::entity EnsureMessageDecodePhase(flecs::world& world) {
    auto phase = world.entity<MessageDecodePhase>("server.phases.MessageDecode");
    phase.add(flecs::Phase);
    phase.depends_on(EnsureTransportPollPhase(world));
    return phase;
}

inline flecs::entity EnsureAuthAndSessionPhase(flecs::world& world) {
    auto phase = world.entity<AuthAndSessionPhase>("server.phases.AuthAndSession");
    phase.add(flecs::Phase);
    phase.depends_on(EnsureMessageDecodePhase(world));
    return phase;
}

inline flecs::entity EnsureInputApplyPhase(flecs::world& world) {
    auto phase = world.entity<InputApplyPhase>("server.phases.InputApply");
    phase.add(flecs::Phase);
    phase.depends_on(EnsureAuthAndSessionPhase(world));
    return phase;
}

inline flecs::entity EnsureSimulationPhase(flecs::world& world) {
    auto phase = world.entity<SimulationPhase>("server.phases.Simulation");
    phase.add(flecs::Phase);
    phase.depends_on(EnsureInputApplyPhase(world));
    return phase;
}

inline flecs::entity EnsureReplicationPhase(flecs::world& world) {
    auto phase = world.entity<ReplicationPhase>("server.phases.Replication");
    phase.add(flecs::Phase);
    phase.depends_on(EnsureSimulationPhase(world));
    return phase;
}

inline flecs::entity EnsurePersistencePhase(flecs::world& world) {
    auto phase = world.entity<PersistencePhase>("server.phases.Persistence");
    phase.add(flecs::Phase);
    phase.depends_on(EnsureReplicationPhase(world));
    return phase;
}

inline flecs::entity EnsureMetricsPhase(flecs::world& world) {
    auto phase = world.entity<MetricsPhase>("server.phases.Metrics");
    phase.add(flecs::Phase);
    phase.depends_on(EnsurePersistencePhase(world));
    return phase;
}

inline void RegisterServerPhases(flecs::world& world) {
    EnsureMetricsPhase(world);
}

}  // namespace server::modules
