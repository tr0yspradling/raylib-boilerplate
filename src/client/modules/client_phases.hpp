#pragma once

#include <flecs.h>

namespace client::modules {

struct InputCapturePhase {};
struct RuntimeIntentPhase {};
struct UiBuildPhase {};
struct UiInteractionPhase {};
struct TransportPollPhase {};
struct SessionUpdatePhase {};
struct PredictionPhase {};
struct PresentationBuildPhase {};
struct RenderPhase {};

inline flecs::entity EnsureInputCapturePhase(flecs::world& world) {
    auto phase = world.entity<InputCapturePhase>("client.phases.InputCapture");
    phase.add(flecs::Phase);
    phase.depends_on(flecs::OnUpdate);
    return phase;
}

inline flecs::entity EnsureRuntimeIntentPhase(flecs::world& world) {
    auto phase = world.entity<RuntimeIntentPhase>("client.phases.RuntimeIntent");
    phase.add(flecs::Phase);
    phase.depends_on(EnsureInputCapturePhase(world));
    return phase;
}

inline flecs::entity EnsureUiBuildPhase(flecs::world& world) {
    auto phase = world.entity<UiBuildPhase>("client.phases.UiBuild");
    phase.add(flecs::Phase);
    phase.depends_on(EnsureRuntimeIntentPhase(world));
    return phase;
}

inline flecs::entity EnsureUiInteractionPhase(flecs::world& world) {
    auto phase = world.entity<UiInteractionPhase>("client.phases.UiInteraction");
    phase.add(flecs::Phase);
    phase.depends_on(EnsureUiBuildPhase(world));
    return phase;
}

inline flecs::entity EnsureTransportPollPhase(flecs::world& world) {
    auto phase = world.entity<TransportPollPhase>("client.phases.TransportPoll");
    phase.add(flecs::Phase);
    phase.depends_on(EnsureUiInteractionPhase(world));
    return phase;
}

inline flecs::entity EnsureSessionUpdatePhase(flecs::world& world) {
    auto phase = world.entity<SessionUpdatePhase>("client.phases.SessionUpdate");
    phase.add(flecs::Phase);
    phase.depends_on(EnsureTransportPollPhase(world));
    return phase;
}

inline flecs::entity EnsurePredictionPhase(flecs::world& world) {
    auto phase = world.entity<PredictionPhase>("client.phases.Prediction");
    phase.add(flecs::Phase);
    phase.depends_on(EnsureSessionUpdatePhase(world));
    return phase;
}

inline flecs::entity EnsurePresentationBuildPhase(flecs::world& world) {
    auto phase = world.entity<PresentationBuildPhase>("client.phases.PresentationBuild");
    phase.add(flecs::Phase);
    phase.depends_on(EnsurePredictionPhase(world));
    return phase;
}

inline flecs::entity EnsureRenderPhase(flecs::world& world) {
    auto phase = world.entity<RenderPhase>("client.phases.Render");
    phase.add(flecs::Phase);
    phase.depends_on(EnsurePresentationBuildPhase(world));
    return phase;
}

inline void RegisterClientPhases(flecs::world& world) {
    EnsureRenderPhase(world);
}

}  // namespace client::modules
