#include "client/runtime/client_presentation_builder.hpp"

#include <algorithm>
#include <chrono>
#include <string>
#include <string_view>

#include "client/components/components.hpp"
#include "client/render/status_presenter.hpp"
#include "client/runtime/client_runtime_flow.hpp"
#include "client/runtime/client_runtime_policy.hpp"
#include "client/scenes/menu_scene.hpp"
#include "client/scenes/sandbox_scene.hpp"
#include "client/scenes/splash_scene.hpp"

namespace client::runtime {

namespace {

[[nodiscard]] std::string ComposeSceneLabel(core::SceneKind sceneKind, std::string_view statusMessage) {
    switch (sceneKind) {
    case core::SceneKind::Splash:
        return std::string{core::SceneName(sceneKind)} + " - " + std::string{scenes::SplashCaption()};
    case core::SceneKind::MainMenu:
        if (!statusMessage.empty()) {
            return std::string{core::SceneName(sceneKind)} + " - " + std::string{statusMessage};
        }
        return std::string{core::SceneName(sceneKind)} + " - " + std::string{policy::kMenuDefaultStatus};
    case core::SceneKind::JoinServer:
        if (!statusMessage.empty()) {
            return std::string{core::SceneName(sceneKind)} + " - " + std::string{statusMessage};
        }
        return std::string{core::SceneName(sceneKind)} + " - " + std::string{policy::kJoinDefaultStatus};
    case core::SceneKind::StartingServer:
        if (!statusMessage.empty()) {
            return std::string{core::SceneName(sceneKind)} + " - " + std::string{statusMessage};
        }
        return std::string{core::SceneName(sceneKind)} + " - " + std::string{policy::kLocalDedicatedBootLabel};
    case core::SceneKind::Connecting:
        if (!statusMessage.empty()) {
            return std::string{core::SceneName(sceneKind)} + " - " + std::string{statusMessage};
        }
        return std::string{core::SceneName(sceneKind)} + " - " + std::string{scenes::ConnectingCaption()};
    case core::SceneKind::GameplayMultiplayer:
        return std::string{core::SceneName(sceneKind)} + " - " + std::string{scenes::GameplayCaption()};
    case core::SceneKind::GameplaySingleplayer:
        if (!statusMessage.empty()) {
            return std::string{core::SceneName(sceneKind)} + " - " + std::string{statusMessage};
        }
        return std::string{core::SceneName(sceneKind)} + " - " + std::string{policy::kLocalSandboxLabel};
    case core::SceneKind::Options:
        if (!statusMessage.empty()) {
            return std::string{core::SceneName(sceneKind)} + " - " + std::string{statusMessage};
        }
        return std::string{core::SceneName(sceneKind)} + " - " + std::string{policy::kSettingsLabel};
    case core::SceneKind::Disconnected:
        if (!statusMessage.empty()) {
            return std::string{core::SceneName(sceneKind)} + " - " + std::string{statusMessage};
        }
        return std::string{core::SceneName(sceneKind)};
    }

    return "Unknown";
}

[[nodiscard]] bool IsLocalPlayerReady(const ClientSessionState& session) {
    return session.serverWelcomed && session.localPlayerId.IsValid();
}

[[nodiscard]] components::WorldRenderState BuildWorldRenderState(const ClientRuntimeContext& context) {
    components::WorldRenderState state;
    const ClientFlowState& flow = context.world.get<ClientFlowState>();
    const ClientSessionState& session = context.world.get<ClientSessionState>();

    if ((flow.runtime.mode == core::RuntimeMode::Singleplayer && context.singleplayerSession.IsActive() &&
         session.localPlayerId.IsValid()) ||
        IsLocalPlayerReady(session)) {
        state.localPlayer = {
            .playerId = session.predictedLocalPlayer.playerId,
            .entityId = session.predictedLocalPlayer.entityId,
            .displayName = session.predictedLocalPlayer.displayName,
            .position = session.predictedLocalPlayer.position,
            .isLocal = true,
        };
    }

    state.remotePlayers.reserve(session.remotePlayers.size());
    for (const auto& [playerId, remote] : session.remotePlayers) {
        (void)playerId;
        state.remotePlayers.push_back({
            .playerId = remote.playerId,
            .entityId = remote.entityId,
            .displayName = remote.displayName,
            .position = remote.interpolation.SampleAt(session.renderInterpolationTick),
            .isLocal = false,
        });
    }

    return state;
}

[[nodiscard]] components::StatusRenderState BuildStatusRenderState(const ClientRuntimeContext& context) {
    const ClientFlowState& flow = context.world.get<ClientFlowState>();
    return render::BuildStatusRenderState(context.ActiveScene(), flow.statusMessage, flow.disconnectReason);
}

[[nodiscard]] components::NetworkDebugState BuildDebugState(const ClientRuntimeContext& context) {
    const ClientFlowState& flow = context.world.get<ClientFlowState>();
    const ClientSessionState& session = context.world.get<ClientSessionState>();
    components::NetworkDebugState state;
    state.activeScene = context.ActiveScene();
    const std::string status = ActiveScreenStatusMessage(flow);
    state.sceneName = ComposeSceneLabel(state.activeScene, status);
    state.connecting = session.connecting;
    state.connected = session.connected;
    state.welcomed = session.serverWelcomed;
    state.disconnectReason = flow.disconnectReason;
    state.runtimeStatusMessage = flow.statusMessage;
    state.clientTick = session.clientTick;
    state.serverTick = session.latestServerTick;
    state.pendingInputCount = session.pendingInputs.size();
    state.loadedChunkCount = session.chunksByCoord.size();
    state.chunkVersionConflicts = session.chunkVersionConflictCount;
    state.metrics = context.multiplayerSession.ConnectionMetrics(session);
    return state;
}

}  // namespace

void ClientPresentationBuilder::Publish(ClientRuntimeContext& context, float frameSeconds) {
    ClientSessionState& session = context.SessionState();
    if (session.latestServerTick > static_cast<shared::game::TickId>(context.config.interpolationDelayTicks)) {
        const float maxTick =
            static_cast<float>(session.latestServerTick -
                               static_cast<shared::game::TickId>(context.config.interpolationDelayTicks));
        session.renderInterpolationTick =
            std::min(session.renderInterpolationTick + frameSeconds * static_cast<float>(session.serverTickRateHz), maxTick);
    }

    ClientRuntimeFlowController::RefreshRuntimeState(context);
    context.multiplayerSession.UpdateCadence(context.FlowState(), session, std::chrono::steady_clock::now());
    ClientRuntimeFlowController::PublishScreenState(context);
    context.world.set<components::WorldRenderState>(BuildWorldRenderState(context));
    context.world.set<components::StatusRenderState>(BuildStatusRenderState(context));
    context.world.set<components::NetworkDebugState>(BuildDebugState(context));
}

}  // namespace client::runtime
