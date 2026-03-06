#include <cassert>
#include <string>
#include <vector>

#include <flecs.h>

#include "server/modules/server_phases.hpp"

int main() {
    flecs::world world;
    server::modules::RegisterServerPhases(world);

    std::vector<std::string> order;

    world.system("TestServerTransportPoll")
        .kind(server::modules::EnsureTransportPollPhase(world))
        .run([&](flecs::iter&) { order.emplace_back("TransportPoll"); });
    world.system("TestServerMessageDecode")
        .kind(server::modules::EnsureMessageDecodePhase(world))
        .run([&](flecs::iter&) { order.emplace_back("MessageDecode"); });
    world.system("TestServerAuthAndSession")
        .kind(server::modules::EnsureAuthAndSessionPhase(world))
        .run([&](flecs::iter&) { order.emplace_back("AuthAndSession"); });
    world.system("TestServerInputApply")
        .kind(server::modules::EnsureInputApplyPhase(world))
        .run([&](flecs::iter&) { order.emplace_back("InputApply"); });
    world.system("TestServerSimulation")
        .kind(server::modules::EnsureSimulationPhase(world))
        .run([&](flecs::iter&) { order.emplace_back("Simulation"); });
    world.system("TestServerReplication")
        .kind(server::modules::EnsureReplicationPhase(world))
        .run([&](flecs::iter&) { order.emplace_back("Replication"); });
    world.system("TestServerPersistence")
        .kind(server::modules::EnsurePersistencePhase(world))
        .run([&](flecs::iter&) { order.emplace_back("Persistence"); });
    world.system("TestServerMetrics")
        .kind(server::modules::EnsureMetricsPhase(world))
        .run([&](flecs::iter&) { order.emplace_back("Metrics"); });

    world.progress();

    const std::vector<std::string> expected{
        "TransportPoll",
        "MessageDecode",
        "AuthAndSession",
        "InputApply",
        "Simulation",
        "Replication",
        "Persistence",
        "Metrics",
    };
    assert(order == expected);

    return 0;
}
