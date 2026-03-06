#include "client/game_client.hpp"

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <span>
#include <string>
#include <unordered_set>
#include <utility>

#include "client/scenes/menu_scene.hpp"
#include "client/scenes/sandbox_scene.hpp"
#include "client/scenes/splash_scene.hpp"
#include "shared/game/chunk_streaming.hpp"
#include "shared/game/validation.hpp"
#include "shared/game/world.hpp"
#include "shared/net/send_policy.hpp"

namespace client {

namespace {

[[nodiscard]] std::string ComposeSceneLabel(core::SceneKind sceneKind) {
    switch (sceneKind) {
    case core::SceneKind::Splash:
        return std::string{core::SceneName(sceneKind)} + " - " + std::string{scenes::SplashCaption()};
    case core::SceneKind::MainMenu:
        return std::string{core::SceneName(sceneKind)} + " - Select mode";
    case core::SceneKind::JoinServer:
        return std::string{core::SceneName(sceneKind)} + " - " + std::string{scenes::ConnectingCaption()};
    case core::SceneKind::StartingServer:
        return std::string{core::SceneName(sceneKind)} + " - Booting local dedicated";
    case core::SceneKind::Connecting:
        return std::string{core::SceneName(sceneKind)} + " - " + std::string{scenes::ConnectingCaption()};
    case core::SceneKind::GameplayMultiplayer:
        return std::string{core::SceneName(sceneKind)} + " - " + std::string{scenes::GameplayCaption()};
    case core::SceneKind::GameplaySingleplayer:
        return std::string{core::SceneName(sceneKind)} + " - Local sandbox";
    case core::SceneKind::Options:
        return std::string{core::SceneName(sceneKind)} + " - Settings";
    case core::SceneKind::Disconnected:
        return std::string{core::SceneName(sceneKind)};
    }

    return "Unknown";
}

}  // namespace

GameClient::GameClient(ClientConfig config)
    : config_(std::move(config)), fixedStep_(1.0 / static_cast<double>(std::max(1, config_.simulationTickHz))) {
    serverTickRateHz_ = static_cast<uint16_t>(std::clamp(config_.simulationTickHz, 1, 65535));
}

bool GameClient::Initialize() {
    window_.emplace(config_.windowWidth, config_.windowHeight, "raylib boilerplate - multiplayer client");
    window_->SetTargetFPS(config_.targetFps);

    runtimeState_.mode = core::RuntimeMode::Boot;
    runtimeState_.splashCompleted = false;
    runtimeState_.disconnectReason.clear();
    sceneManager_.SwitchTo(core::SceneKind::Splash);

    std::string error;
    if (!transport_.Initialize(net::TransportConfig{.isServer = false, .debugVerbosity = 4, .allowUnencryptedDev = true},
                               error)) {
        std::fprintf(stderr, "[net.transport] client transport init failed: %s\n", error.c_str());
        return false;
    }

    serverConnection_ = transport_.Connect(config_.serverHost, config_.serverPort, error);
    if (serverConnection_ == net::kInvalidConnectionHandle) {
        std::fprintf(stderr, "[net.transport] connect failed: %s\n", error.c_str());
        return false;
    }

    connecting_ = true;
    runtimeState_.requestedJoin = true;
    runtimeState_.mode = core::RuntimeMode::JoiningServer;
    sceneManager_.SwitchTo(core::SceneKind::JoinServer);
    return true;
}

int GameClient::Run() {
    using clock = std::chrono::steady_clock;
    auto lastFrameAt = clock::now();
    lastPingSentAt_ = lastFrameAt;
    lastChunkHintSentAt_ = lastFrameAt;

    while (window_.has_value() && !raylib::Window::ShouldClose()) {
        inputManager_.Update();
        if (inputManager_.DebugOverlayToggled()) {
            debugOverlayEnabled_ = !debugOverlayEnabled_;
        }
        if (inputManager_.QuitRequested()) {
            break;
        }

        const auto now = clock::now();
        const std::chrono::duration<double> frameDelta = now - lastFrameAt;
        const float frameSeconds = static_cast<float>(frameDelta.count());
        lastFrameAt = now;

        PollTransport();

        const int simSteps = fixedStep_.Accumulate(frameSeconds);
        for (int i = 0; i < simSteps; ++i) {
            StepSimulation();
        }

        RefreshRuntimeState();
        core::Application::UpdateScene(sceneManager_, runtimeState_);

        const auto sinceLastPing = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastPingSentAt_);
        if (connected_ && sinceLastPing.count() >= 1000) {
            net::PingMessage ping{.sequence = nextPingSequence_++};
            const std::vector<uint8_t> payload = net::Serialize(ping);
            const std::vector<uint8_t> packet = net::BuildPacket(net::MessageId::Ping, payload);
            std::string error;
            transport_.Send(serverConnection_, packet,
                            net::SendOptionsForMessage(net::MessageId::Ping, net::MessageDirection::ClientToServer),
                            error);
            lastPingSentAt_ = now;
        }

        const auto sinceLastChunkHint = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastChunkHintSentAt_);
        if (connected_ && serverWelcomed_ && IsLocalPlayerReady() && sinceLastChunkHint.count() >= 500) {
            SendChunkInterestHint();
            lastChunkHintSentAt_ = now;
        }

        DrawFrame(frameSeconds);
    }

    if (connected_) {
        transport_.Close(serverConnection_, 0, "client shutdown", false);
    }
    transport_.Shutdown();

    return 0;
}

void GameClient::PollTransport() {
    transport_.Poll();
    HandleConnectionEvents();
    HandleIncomingPackets();
}

void GameClient::HandleConnectionEvents() {
    const std::vector<net::ConnectionEvent> events = transport_.DrainConnectionEvents();
    for (const net::ConnectionEvent& event : events) {
        if (event.connection != serverConnection_) {
            continue;
        }

        if (event.type == net::ConnectionEventType::Connected) {
            connected_ = true;
            connecting_ = false;
            OnConnectedToServer();
            continue;
        }

        if (event.type == net::ConnectionEventType::ClosedByPeer ||
            event.type == net::ConnectionEventType::ProblemDetectedLocally) {
            connected_ = false;
            disconnectReason_ = event.reason.empty() ? "connection closed" : event.reason;
            ResetSessionState();
        }
    }
}

void GameClient::HandleIncomingPackets() {
    const std::vector<net::ReceivedPacket> packets = transport_.DrainReceivedPackets();
    for (const net::ReceivedPacket& packet : packets) {
        if (packet.connection != serverConnection_) {
            continue;
        }

        net::EnvelopeHeader header;
        std::span<const uint8_t> payload;
        std::string parseError;
        if (!net::ParsePacket(packet.bytes, header, payload, parseError)) {
            std::fprintf(stderr, "[net.protocol] drop malformed packet: %s\n", parseError.c_str());
            continue;
        }

        if (header.protocolVersion != net::kProtocolVersion) {
            disconnectReason_ = "protocol version mismatch";
            connected_ = false;
            ResetSessionState();
            transport_.Close(serverConnection_, 4002, disconnectReason_, false);
            continue;
        }

        switch (header.messageId) {
        case net::MessageId::ServerWelcome: {
            net::ServerWelcomeMessage welcome;
            if (net::Deserialize(payload, welcome, parseError)) {
                HandleServerWelcome(welcome);
            }
            break;
        }
        case net::MessageId::WorldMetadata: {
            net::WorldMetadataMessage metadata;
            if (net::Deserialize(payload, metadata, parseError)) {
                HandleWorldMetadata(metadata);
            }
            break;
        }
        case net::MessageId::SpawnPlayer: {
            net::SpawnPlayerMessage spawn;
            if (net::Deserialize(payload, spawn, parseError)) {
                HandleSpawnPlayer(spawn);
            }
            break;
        }
        case net::MessageId::DespawnEntity: {
            net::DespawnEntityMessage despawn;
            if (net::Deserialize(payload, despawn, parseError)) {
                HandleDespawnEntity(despawn);
            }
            break;
        }
        case net::MessageId::SnapshotBaseline:
        case net::MessageId::SnapshotDelta: {
            net::ByteReader reader(payload);
            net::SnapshotPayload snapshot;
            if (net::DeserializeSnapshotPayload(reader, snapshot, parseError)) {
                HandleSnapshot(snapshot);
            }
            break;
        }
        case net::MessageId::ChunkBaseline: {
            net::ChunkBaselineMessage baseline;
            if (net::Deserialize(payload, baseline, parseError)) {
                HandleChunkBaseline(baseline);
            }
            break;
        }
        case net::MessageId::ChunkDelta: {
            net::ChunkDeltaMessage delta;
            if (net::Deserialize(payload, delta, parseError)) {
                HandleChunkDelta(delta);
            }
            break;
        }
        case net::MessageId::ChunkUnsubscribe: {
            net::ChunkUnsubscribeMessage unsubscribe;
            if (net::Deserialize(payload, unsubscribe, parseError)) {
                HandleChunkUnsubscribe(unsubscribe);
            }
            break;
        }
        case net::MessageId::ResyncRequired: {
            net::ResyncRequiredMessage resync;
            if (net::Deserialize(payload, resync, parseError)) {
                HandleResyncRequired(resync);
            }
            break;
        }
        case net::MessageId::DisconnectReason: {
            net::DisconnectReasonMessage reason;
            if (net::Deserialize(payload, reason, parseError)) {
                HandleDisconnectReason(reason);
            }
            break;
        }
        case net::MessageId::Pong:
        default:
            break;
        }
    }
}

void GameClient::OnConnectedToServer() {
    net::ClientHelloMessage hello;
    hello.requestedProtocolVersion = net::kProtocolVersion;
    hello.buildCompatibilityHash = config_.buildCompatibilityHash;
    hello.playerName = config_.playerName;
    hello.authToken = "dev";

    const std::vector<uint8_t> payload = net::Serialize(hello);
    const std::vector<uint8_t> packet = net::BuildPacket(net::MessageId::ClientHello, payload);

    std::string error;
    if (!transport_.Send(serverConnection_, packet,
                         net::SendOptionsForMessage(net::MessageId::ClientHello, net::MessageDirection::ClientToServer),
                         error)) {
        disconnectReason_ = "failed to send ClientHello: " + error;
        connected_ = false;
        ResetSessionState();
    }
}

void GameClient::HandleServerWelcome(const net::ServerWelcomeMessage& message) {
    if (message.protocolVersion != net::kProtocolVersion) {
        disconnectReason_ = "protocol mismatch";
        connected_ = false;
        ResetSessionState();
        transport_.Close(serverConnection_, 4003, disconnectReason_, false);
        return;
    }

    const game::PlayerKinematicsValidationError kinematicsValidation =
        game::ValidatePlayerKinematicsConfig(message.playerKinematics);
    if (kinematicsValidation != game::PlayerKinematicsValidationError::None) {
        disconnectReason_ = std::string{"invalid server kinematics: "} + game::ToString(kinematicsValidation);
        connected_ = false;
        ResetSessionState();
        transport_.Close(serverConnection_, 4004, disconnectReason_, false);
        return;
    }

    serverWelcomed_ = true;
    localPlayerId = message.playerId;
    latestServerTick_ = message.serverTick;
    renderInterpolationTick_ = static_cast<float>(message.serverTick);
    serverTickRateHz_ = std::max<uint16_t>(1, message.serverTickRateHz);
    serverSnapshotRateHz_ = std::max<uint16_t>(1, message.snapshotRateHz);
    serverKinematics_ = message.playerKinematics;
    fixedStep_.SetStepSeconds(1.0 / static_cast<double>(serverTickRateHz_));

    predictedLocalPlayer_.playerId = message.playerId;
    predictedLocalPlayer_.entityId = game::EntityId{message.playerId.Value()};
    predictedLocalPlayer_.displayName = config_.playerName;
    predictedLocalPlayer_.position = {0.0f, 0.0f};
    predictedLocalPlayer_.velocity = {0.0f, 0.0f};
    predictedLocalPlayer_.onGround = true;
    pendingInputs_.clear();
    chunksByCoord_.clear();
    chunkVersionConflictCount_ = 0;
    serverWorldConfig_ = game::WorldConfig{};
    hasWorldMetadata_ = false;
}

void GameClient::HandleWorldMetadata(const net::WorldMetadataMessage& message) {
    if (message.chunkWidthTiles == 0 || message.chunkHeightTiles == 0 || message.tileSize == 0 ||
        message.defaultInterestRadiusChunks == 0) {
        std::fprintf(stderr, "[world.chunk] invalid world metadata ignored\n");
        return;
    }

    serverWorldConfig_.chunkWidthTiles = static_cast<int>(message.chunkWidthTiles);
    serverWorldConfig_.chunkHeightTiles = static_cast<int>(message.chunkHeightTiles);
    serverWorldConfig_.tileSize = static_cast<int>(message.tileSize);
    serverWorldConfig_.interestRadiusChunks = static_cast<int>(message.defaultInterestRadiusChunks);
    hasWorldMetadata_ = true;
}

void GameClient::HandleSpawnPlayer(const net::SpawnPlayerMessage& message) {
    if (message.playerId == localPlayerId) {
        predictedLocalPlayer_.position = message.spawnPosition;
        predictedLocalPlayer_.entityId = message.entityId;
        predictedLocalPlayer_.displayName = message.displayName;
        return;
    }

    RemotePlayerView& remote = remotePlayers_[message.playerId];
    remote.playerId = message.playerId;
    remote.entityId = message.entityId;
    remote.displayName = message.displayName;
    remote.latestPosition = message.spawnPosition;
    remote.interpolation.Push({.tick = latestServerTick_, .position = message.spawnPosition});
}

void GameClient::HandleDespawnEntity(const net::DespawnEntityMessage& message) {
    for (auto it = remotePlayers_.begin(); it != remotePlayers_.end(); ++it) {
        if (it->second.entityId == message.entityId) {
            remotePlayers_.erase(it);
            return;
        }
    }
}

void GameClient::HandleSnapshot(const net::SnapshotPayload& snapshot) {
    latestServerTick_ = snapshot.serverTick;

    std::unordered_set<game::PlayerId, game::IdHash<game::PlayerIdTag>> seenRemotePlayers;
    for (const net::SnapshotEntity& entity : snapshot.entities) {
        if (entity.playerId == localPlayerId) {
            ReconcileFromSnapshot(entity);
            continue;
        }

        RemotePlayerView& remote = remotePlayers_[entity.playerId];
        remote.playerId = entity.playerId;
        remote.entityId = entity.entityId;
        remote.displayName = entity.displayName;
        remote.latestPosition = entity.position;
        remote.interpolation.Push({.tick = snapshot.serverTick, .position = entity.position});
        seenRemotePlayers.insert(entity.playerId);
    }

    for (auto it = remotePlayers_.begin(); it != remotePlayers_.end();) {
        if (!seenRemotePlayers.contains(it->first)) {
            it = remotePlayers_.erase(it);
        } else {
            ++it;
        }
    }
}

void GameClient::HandleChunkBaseline(const net::ChunkBaselineMessage& message) {
    if (!message.chunk.IsValid()) {
        std::fprintf(stderr, "[world.chunk] drop invalid baseline %d,%d\n", message.chunk.coord.x, message.chunk.coord.y);
        return;
    }

    ClientChunkState& chunk = chunksByCoord_[message.chunk.coord];
    chunk.chunk = message.chunk;
    chunkResyncRequestedAt_.erase(message.chunk.coord);
}

void GameClient::HandleChunkDelta(const net::ChunkDeltaMessage& message) {
    auto chunkIt = chunksByCoord_.find(message.delta.coord);
    if (chunkIt == chunksByCoord_.end()) {
        ++chunkVersionConflictCount_;
        std::fprintf(stderr, "[world.chunk] delta for unknown chunk %d,%d\n", message.delta.coord.x, message.delta.coord.y);
        RequestChunkResync(message.delta.coord, 0U);
        return;
    }

    game::ChunkData& chunk = chunkIt->second.chunk;
    if (chunk.version.value != message.delta.baseVersion.value) {
        ++chunkVersionConflictCount_;
        std::fprintf(stderr, "[world.chunk] version mismatch chunk %d,%d local=%u base=%u\n", message.delta.coord.x,
                     message.delta.coord.y, chunk.version.value, message.delta.baseVersion.value);
        RequestChunkResync(message.delta.coord, chunk.version.value);
        return;
    }

    if (!game::ApplyChunkDelta(chunk, message.delta)) {
        ++chunkVersionConflictCount_;
        std::fprintf(stderr, "[world.chunk] invalid delta ops chunk %d,%d ops=%zu\n", message.delta.coord.x,
                     message.delta.coord.y, message.delta.operations.size());
        RequestChunkResync(message.delta.coord, chunk.version.value);
    }
}

void GameClient::HandleChunkUnsubscribe(const net::ChunkUnsubscribeMessage& message) {
    const game::ChunkCoord coord{
        .x = message.chunkX,
        .y = message.chunkY,
    };
    chunksByCoord_.erase(coord);
    chunkResyncRequestedAt_.erase(coord);
}

void GameClient::HandleResyncRequired(const net::ResyncRequiredMessage& message) {
    chunksByCoord_.clear();
    chunkResyncRequestedAt_.clear();
    chunkVersionConflictCount_ = 0;
    disconnectReason_ = message.reason;
    lastChunkHintSentAt_ = std::chrono::steady_clock::now() - std::chrono::milliseconds(1000);
}

void GameClient::HandleDisconnectReason(const net::DisconnectReasonMessage& message) {
    disconnectReason_ = message.reason;
    connected_ = false;
    ResetSessionState();
}

void GameClient::ResetSessionState() {
    connecting_ = false;
    serverWelcomed_ = false;
    localPlayerId = {};
    predictedLocalPlayer_ = {};
    pendingInputs_.clear();
    remotePlayers_.clear();
    chunksByCoord_.clear();
    chunkResyncRequestedAt_.clear();
    chunkVersionConflictCount_ = 0;
    serverWorldConfig_ = game::WorldConfig{};
    hasWorldMetadata_ = false;
    clientTick_ = 0;
    latestServerTick_ = 0;
    renderInterpolationTick_ = 0.0f;
    nextInputSequence_ = 1;
}

void GameClient::RefreshRuntimeState() {
    runtimeState_.disconnectReason = disconnectReason_;

    if (!disconnectReason_.empty() || (!connected_ && !connecting_)) {
        runtimeState_.mode = core::RuntimeMode::Disconnected;
        return;
    }

    runtimeState_.disconnectReason.clear();
    runtimeState_.splashCompleted = true;

    if (connecting_ || (connected_ && !serverWelcomed_)) {
        runtimeState_.mode = core::RuntimeMode::JoiningServer;
        return;
    }

    if (connected_ && serverWelcomed_) {
        runtimeState_.mode = core::RuntimeMode::Multiplayer;
        return;
    }

    runtimeState_.mode = core::RuntimeMode::Menu;
}

void GameClient::StepSimulation() {
    if (!connected_ || !serverWelcomed_ || !IsLocalPlayerReady()) {
        return;
    }

    game::PlayerInputFrame inputFrame = inputManager_.BuildPlayerInputFrame(clientTick_, nextInputSequence_++);
    ++clientTick_;

    SendInputFrame(inputFrame);
    pendingInputs_.push_back(inputFrame);

    physics::MovementSystem::Predict(predictedLocalPlayer_, inputFrame, static_cast<float>(fixedStep_.StepSeconds()),
                                     serverKinematics_);
}

void GameClient::SendInputFrame(const game::PlayerInputFrame& frame) {
    net::InputFrameMessage inputMessage;
    inputMessage.clientTick = frame.clientTick;
    inputMessage.sequence = frame.sequence;
    inputMessage.moveX = frame.moveX;
    inputMessage.jumpPressed = frame.jumpPressed;

    const std::vector<uint8_t> payload = net::Serialize(inputMessage);
    const std::vector<uint8_t> packet = net::BuildPacket(net::MessageId::InputFrame, payload);

    std::string error;
    if (!transport_.Send(
            serverConnection_, packet,
            net::SendOptionsForMessage(net::MessageId::InputFrame, net::MessageDirection::ClientToServer), error)) {
        std::fprintf(stderr, "[net.protocol] input send failed: %s\n", error.c_str());
    }
}

void GameClient::SendChunkInterestHint() {
    if (!connected_ || !serverWelcomed_ || !IsLocalPlayerReady()) {
        return;
    }

    const game::WorldConfig& worldConfig = hasWorldMetadata_ ? serverWorldConfig_ : game::WorldConfig{};
    const game::ChunkCoord center = game::WorldToChunkCoord(predictedLocalPlayer_.position, worldConfig);
    const net::ChunkInterestHintMessage hint{
        .centerChunkX = center.x,
        .centerChunkY = center.y,
        .radiusChunks = static_cast<uint16_t>(std::clamp(worldConfig.interestRadiusChunks, 1, 24)),
    };

    const std::vector<uint8_t> payload = net::Serialize(hint);
    const std::vector<uint8_t> packet = net::BuildPacket(net::MessageId::ChunkInterestHint, payload);

    std::string error;
    transport_.Send(serverConnection_, packet,
                    net::SendOptionsForMessage(net::MessageId::ChunkInterestHint, net::MessageDirection::ClientToServer),
                    error);
}

void GameClient::RequestChunkResync(const game::ChunkCoord& coord, uint32_t clientVersion) {
    if (!connected_ || !serverWelcomed_) {
        return;
    }

    const auto now = std::chrono::steady_clock::now();
    const auto it = chunkResyncRequestedAt_.find(coord);
    if (it != chunkResyncRequestedAt_.end()) {
        const auto since = std::chrono::duration_cast<std::chrono::milliseconds>(now - it->second);
        if (since.count() < 250) {
            return;
        }
    }

    const net::ChunkResyncRequestMessage request{
        .chunkX = coord.x,
        .chunkY = coord.y,
        .clientVersion = clientVersion,
    };

    const std::vector<uint8_t> payload = net::Serialize(request);
    const std::vector<uint8_t> packet = net::BuildPacket(net::MessageId::ChunkResyncRequest, payload);
    std::string error;
    if (transport_.Send(
            serverConnection_, packet,
            net::SendOptionsForMessage(net::MessageId::ChunkResyncRequest, net::MessageDirection::ClientToServer),
            error)) {
        chunkResyncRequestedAt_[coord] = now;
    }
}

void GameClient::ReconcileFromSnapshot(const net::SnapshotEntity& localEntity) {
    physics::MovementSystem::Reconcile(predictedLocalPlayer_, localEntity, pendingInputs_,
                                       static_cast<float>(fixedStep_.StepSeconds()), serverKinematics_);
}

void GameClient::DrawFrame(float frameSeconds) {
    if (!window_.has_value()) {
        return;
    }

    if (latestServerTick_ > static_cast<game::TickId>(config_.interpolationDelayTicks)) {
        const float maxTick =
            static_cast<float>(latestServerTick_ - static_cast<game::TickId>(config_.interpolationDelayTicks));
        renderInterpolationTick_ = std::min(renderInterpolationTick_ + frameSeconds * static_cast<float>(serverTickRateHz_),
                                            maxTick);
    }

    window_->BeginDrawing();
    systems::RenderSystem::DrawFrame(BuildWorldRenderState(), BuildDebugState(), debugOverlayEnabled_);
    window_->EndDrawing();
}

components::WorldRenderState GameClient::BuildWorldRenderState() const {
    components::WorldRenderState state;

    if (IsLocalPlayerReady()) {
        state.localPlayer = {
            .playerId = predictedLocalPlayer_.playerId,
            .entityId = predictedLocalPlayer_.entityId,
            .displayName = predictedLocalPlayer_.displayName,
            .position = predictedLocalPlayer_.position,
            .isLocal = true,
        };
    }

    state.remotePlayers.reserve(remotePlayers_.size());
    for (const auto& [playerId, remote] : remotePlayers_) {
        (void)playerId;
        state.remotePlayers.push_back({
            .playerId = remote.playerId,
            .entityId = remote.entityId,
            .displayName = remote.displayName,
            .position = remote.interpolation.SampleAt(renderInterpolationTick_),
            .isLocal = false,
        });
    }

    return state;
}

components::NetworkDebugState GameClient::BuildDebugState() const {
    components::NetworkDebugState state;
    state.sceneName = ComposeSceneLabel(sceneManager_.ActiveScene());
    state.connecting = connecting_;
    state.connected = connected_;
    state.welcomed = serverWelcomed_;
    state.disconnectReason = disconnectReason_;
    state.clientTick = clientTick_;
    state.serverTick = latestServerTick_;
    state.pendingInputCount = pendingInputs_.size();
    state.loadedChunkCount = chunksByCoord_.size();
    state.chunkVersionConflicts = chunkVersionConflictCount_;
    state.metrics = transport_.GetConnectionMetrics(serverConnection_);
    return state;
}

bool GameClient::IsLocalPlayerReady() const {
    return serverWelcomed_ && localPlayerId.IsValid();
}

}  // namespace client
