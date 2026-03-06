#pragma once

#include <flecs.h>

#include "client/components/components.hpp"
#include "client/modules/client_phases.hpp"
#include "client/runtime/client_runtime.hpp"

namespace client::modules {

struct ClientRuntimeRef {
    runtime::ClientRuntime* runtime = nullptr;
};

inline runtime::ClientRuntime* ResolveClientRuntime(flecs::world world) {
    ClientRuntimeRef& ref = world.get_mut<ClientRuntimeRef>();
    return ref.runtime;
}

struct ClientRuntimeModule {
    explicit ClientRuntimeModule(flecs::world& world) {
        world.module<ClientRuntimeModule>();
        RegisterClientPhases(world);
        world.component<ClientRuntimeRef>();

        world.system("ClientCaptureInput")
            .kind(EnsureInputCapturePhase(world))
            .run([](flecs::iter& it) {
                if (auto* runtime = ResolveClientRuntime(it.world())) {
                    runtime->CaptureInput(it.world());
                }
            });

        world.system("ClientRuntimeIntent")
            .kind(EnsureRuntimeIntentPhase(world))
            .run([](flecs::iter& it) {
                if (auto* runtime = ResolveClientRuntime(it.world())) {
                    runtime->ProcessRuntimeIntent(it.world());
                }
            });

        world.system("ClientUiBuild")
            .kind(EnsureUiBuildPhase(world))
            .run([](flecs::iter& it) {
                if (auto* runtime = ResolveClientRuntime(it.world())) {
                    runtime->BuildUiState(it.world());
                }
            });

        world.system("ClientUiInteraction")
            .kind(EnsureUiInteractionPhase(world))
            .run([](flecs::iter& it) {
                if (auto* runtime = ResolveClientRuntime(it.world())) {
                    runtime->HandleUiInteraction(it.world());
                }
            });

        world.system("ClientTransportPoll")
            .kind(EnsureTransportPollPhase(world))
            .run([](flecs::iter& it) {
                if (auto* runtime = ResolveClientRuntime(it.world())) {
                    runtime->PollTransport();
                }
            });

        world.system("ClientSessionUpdate")
            .kind(EnsureSessionUpdatePhase(world))
            .run([](flecs::iter& it) {
                if (auto* runtime = ResolveClientRuntime(it.world())) {
                    runtime->RefreshSessionState(it.world());
                }
            });

        world.system("ClientPrediction")
            .kind(EnsurePredictionPhase(world))
            .run([](flecs::iter& it) {
                if (auto* runtime = ResolveClientRuntime(it.world())) {
                    runtime->AdvancePrediction(static_cast<float>(it.delta_time()));
                }
            });

        world.system("ClientPresentationBuild")
            .kind(EnsurePresentationBuildPhase(world))
            .run([](flecs::iter& it) {
                if (auto* runtime = ResolveClientRuntime(it.world())) {
                    runtime->PublishPresentation(it.world(), static_cast<float>(it.delta_time()));
                }
            });

        world.system("ClientRender")
            .kind(EnsureRenderPhase(world))
            .run([](flecs::iter& it) {
                if (auto* runtime = ResolveClientRuntime(it.world())) {
                    runtime->RenderPublishedFrame(it.world());
                }
            });
    }
};

}  // namespace client::modules
