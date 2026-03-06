#include <cassert>
#include <string>
#include <vector>

#include <flecs.h>

#include "client/modules/client_phases.hpp"

int main() {
    flecs::world world;
    client::modules::RegisterClientPhases(world);

    std::vector<std::string> order;

    world.system("TestClientInputCapture")
        .kind(client::modules::EnsureInputCapturePhase(world))
        .run([&](flecs::iter&) { order.emplace_back("InputCapture"); });
    world.system("TestClientRuntimeIntent")
        .kind(client::modules::EnsureRuntimeIntentPhase(world))
        .run([&](flecs::iter&) { order.emplace_back("RuntimeIntent"); });
    world.system("TestClientUiBuild")
        .kind(client::modules::EnsureUiBuildPhase(world))
        .run([&](flecs::iter&) { order.emplace_back("UiBuild"); });
    world.system("TestClientUiInteraction")
        .kind(client::modules::EnsureUiInteractionPhase(world))
        .run([&](flecs::iter&) { order.emplace_back("UiInteraction"); });
    world.system("TestClientTransportPoll")
        .kind(client::modules::EnsureTransportPollPhase(world))
        .run([&](flecs::iter&) { order.emplace_back("TransportPoll"); });
    world.system("TestClientSessionUpdate")
        .kind(client::modules::EnsureSessionUpdatePhase(world))
        .run([&](flecs::iter&) { order.emplace_back("SessionUpdate"); });
    world.system("TestClientPrediction")
        .kind(client::modules::EnsurePredictionPhase(world))
        .run([&](flecs::iter&) { order.emplace_back("Prediction"); });
    world.system("TestClientPresentationBuild")
        .kind(client::modules::EnsurePresentationBuildPhase(world))
        .run([&](flecs::iter&) { order.emplace_back("PresentationBuild"); });
    world.system("TestClientRender")
        .kind(client::modules::EnsureRenderPhase(world))
        .run([&](flecs::iter&) { order.emplace_back("Render"); });

    world.progress();

    const std::vector<std::string> expected{
        "InputCapture",
        "RuntimeIntent",
        "UiBuild",
        "UiInteraction",
        "TransportPoll",
        "SessionUpdate",
        "Prediction",
        "PresentationBuild",
        "Render",
    };
    assert(order == expected);

    return 0;
}
