#pragma once

#include <chrono>
#include <cstdint>
#include <deque>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include <flecs.h>
#include <raylib-cpp.hpp>

#include "client/components/components.hpp"
#include "client/core/application.hpp"
#include "client/core/client_config.hpp"
#include "client/core/menu_model.hpp"
#include "client/core/runtime_state.hpp"
#include "client/core/scene_manager.hpp"
#include "client/core/server_launcher.hpp"
#include "client/core/singleplayer_runtime.hpp"
#include "client/input/input_manager.hpp"
#include "client/physics/movement_system.hpp"
#include "client/systems/render_system.hpp"
#include "client/ui/ui_document.hpp"
#include "client/ui/ui_state.hpp"
#include "shared/game/chunk.hpp"
#include "shared/game/entity.hpp"
#include "shared/game/fixed_step.hpp"
#include "shared/game/interpolation.hpp"
#include "shared/net/protocol.hpp"
#include "shared/net/transport_gns.hpp"

namespace client {

namespace game = shared::game;
namespace net = shared::net;

namespace runtime {

class ClientRuntime {
public:
    explicit ClientRuntime(ClientConfig config);

    [[nodiscard]] bool Initialize(flecs::world world);
    void Shutdown();

    void CaptureInput(flecs::world world);
    void ProcessRuntimeIntent(flecs::world world);
    void BuildUiState(flecs::world world);
    void HandleUiInteraction(flecs::world world);
    void PollTransport();
    void RefreshSessionState(flecs::world world);
    void AdvancePrediction(float frameSeconds);
    void PublishPresentation(flecs::world world, float frameSeconds);
    void RenderPublishedFrame(const flecs::world& world);

    [[nodiscard]] bool ShouldExit() const;

private:
    struct RemotePlayerView {
        game::PlayerId playerId{};
        std::string displayName;
        game::EntityId entityId{};
        game::Vec2f latestPosition{};
        game::PositionInterpolationBuffer interpolation;
    };

    struct ClientChunkState {
        game::ChunkData chunk;
    };

    void HandleConnectionEvents();
    void HandleIncomingPackets();
    void ConsumeUiCommands(flecs::world world);
    void HandlePlaceholderScreenInput(flecs::world world);
    void UpdateLocalServerStartup(flecs::world world, std::chrono::steady_clock::time_point now);
    void HandleUiPointerFocus(flecs::world world, const ui::UiDocument& document, const ui::UiInputState& inputState);
    void HandleMenuInteraction(flecs::world world, const ui::UiDocument& document, const ui::UiInputState& inputState);
    void HandleJoinFormInteraction(flecs::world world, const ui::UiDocument& document, const ui::UiInputState& inputState);
    void HandleOptionsInteraction(flecs::world world, const ui::UiDocument& document, const ui::UiInputState& inputState);
    void ApplyJoinFormTextInput(ui::JoinServerScreenState& joinScreenState, const ui::UiInputState& inputState) const;
    void ApplyOptionsTextInput(ui::OptionsScreenState& optionsScreenState, const ui::UiInputState& inputState) const;
    void ActivateMenuAction(flecs::world world, core::MenuAction action);
    void BeginSingleplayer(flecs::world world);
    bool BeginJoinServer(flecs::world world);
    [[nodiscard]] bool EnsureTransportInitialized(std::string& error);
    [[nodiscard]] bool BeginConnectionAttempt(std::string& error);
    bool ApplyJoinFormToConfig(const ui::JoinServerScreenState& joinScreenState);
    bool ApplyOptionsToConfig(flecs::world world, ui::OptionsScreenState& optionsScreenState);
    void ReturnToMenu(flecs::world world, std::string statusMessage = {});
    void FailLocalServerStartup(flecs::world world, const std::string& message);
    void StopOwnedLocalServer();
    void PublishScreenState(flecs::world world);
    [[nodiscard]] ui::UiDocument BuildMenuDocument(const ui::ScreenState& screenState,
                                                   const ui::MenuScreenState& menuScreenState,
                                                   const ui::UiInteractionState& interactionState) const;
    [[nodiscard]] ui::UiDocument BuildJoinDocument(const ui::ScreenState& screenState,
                                                   const ui::JoinServerScreenState& joinScreenState,
                                                   const ui::UiInteractionState& interactionState) const;
    [[nodiscard]] ui::UiDocument BuildOptionsDocument(const ui::ScreenState& screenState,
                                                      const ui::OptionsScreenState& optionsScreenState,
                                                      const ui::UiInteractionState& interactionState) const;

    void OnConnectedToServer();

    void HandleServerWelcome(const net::ServerWelcomeMessage& message);
    void HandleWorldMetadata(const net::WorldMetadataMessage& message);
    void HandleSpawnPlayer(const net::SpawnPlayerMessage& message);
    void HandleDespawnEntity(const net::DespawnEntityMessage& message);
    void HandleSnapshot(const net::SnapshotPayload& snapshot);
    void HandleChunkBaseline(const net::ChunkBaselineMessage& message);
    void HandleChunkDelta(const net::ChunkDeltaMessage& message);
    void HandleChunkUnsubscribe(const net::ChunkUnsubscribeMessage& message);
    void HandleResyncRequired(const net::ResyncRequiredMessage& message);
    void HandleDisconnectReason(const net::DisconnectReasonMessage& message);
    void ResetSessionState();
    void RefreshRuntimeState();

    void StepSimulation();
    void SendInputFrame(const game::PlayerInputFrame& frame);
    void SendChunkInterestHint();
    void RequestChunkResync(const game::ChunkCoord& coord, uint32_t clientVersion);
    void ReconcileFromSnapshot(const net::SnapshotEntity& localEntity);

    [[nodiscard]] components::WorldRenderState BuildWorldRenderState() const;
    [[nodiscard]] components::StatusRenderState BuildStatusRenderState() const;
    [[nodiscard]] components::NetworkDebugState BuildDebugState() const;

    [[nodiscard]] bool IsLocalPlayerReady() const;

    ClientConfig config_;

    std::optional<raylib::Window> window_;

    net::TransportGns transport_;
    net::ConnectionHandle serverConnection_ = net::kInvalidConnectionHandle;
    bool connecting_ = false;
    bool connected_ = false;
    bool serverWelcomed_ = false;
    std::string disconnectReason_;
    std::string runtimeStatusMessage_;
    bool debugOverlayEnabled_ = true;
    bool exitRequested_ = false;
    std::chrono::steady_clock::time_point splashStartedAt_{};
    std::unique_ptr<core::IServerLauncher> serverLauncher_;
    bool ownsLocalServerProcess_ = false;
    bool localServerStartupInProgress_ = false;
    std::chrono::steady_clock::time_point localServerLaunchStartedAt_{};
    std::chrono::steady_clock::time_point lastLocalServerConnectAttemptAt_{};

    game::FixedStep fixedStep_;
    core::SceneManager sceneManager_{};
    core::RuntimeState runtimeState_{};
    input::InputManager inputManager_{};
    core::SingleplayerRuntime singleplayerRuntime_{};
    game::TickId clientTick_ = 0;
    game::TickId latestServerTick_ = 0;
    float renderInterpolationTick_ = 0.0f;
    uint16_t serverTickRateHz_ = 30;
    uint16_t serverSnapshotRateHz_ = 15;
    game::PlayerKinematicsConfig serverKinematics_{};
    game::WorldConfig serverWorldConfig_{};
    bool hasWorldMetadata_ = false;

    game::PlayerId localPlayerId{};
    game::PlayerState predictedLocalPlayer_{};
    std::deque<game::PlayerInputFrame> pendingInputs_;
    uint32_t nextInputSequence_ = 1;

    std::unordered_map<game::PlayerId, RemotePlayerView, game::IdHash<game::PlayerIdTag>> remotePlayers_;
    std::unordered_map<game::ChunkCoord, ClientChunkState, game::ChunkCoordHash> chunksByCoord_;
    std::unordered_map<game::ChunkCoord, std::chrono::steady_clock::time_point, game::ChunkCoordHash>
        chunkResyncRequestedAt_;
    uint32_t chunkVersionConflictCount_ = 0;

    std::chrono::steady_clock::time_point lastPingSentAt_{};
    std::chrono::steady_clock::time_point lastChunkHintSentAt_{};
    uint32_t nextPingSequence_ = 1;
};

}  // namespace runtime

}  // namespace client
