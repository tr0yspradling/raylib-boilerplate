#include "server/game_server.hpp"

#include <algorithm>
#include <cstdlib>
#include <cstdio>
#include <span>
#include <thread>

#include "server/world_persistence.hpp"
#include "shared/game/chunk_streaming.hpp"
#include "shared/game/validation.hpp"
#include "shared/game/world.hpp"
#include "shared/net/send_policy.hpp"

namespace server {

GameServer::GameServer(ServerConfig config)
    : config_(std::move(config)), gameState_(config_.worldConfig, config_.playerKinematics) {}

bool GameServer::Initialize() {
    if (config_.authMode == shared::net::AuthMode::DevInsecure) {
        authProvider_ = std::make_unique<shared::net::DevAuthProvider>();
    } else {
        authProvider_ = std::make_unique<shared::net::BackendTokenAuthProvider>();
    }

    std::string error;
    if (!transport_.Initialize(shared::net::TransportConfig{.isServer = true, .debugVerbosity = 4, .allowUnencryptedDev = true},
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

int GameServer::Run() {
    if (!initialized_) {
        return 1;
    }

    running_ = true;
    shared::game::FixedStep fixedStep{1.0 / static_cast<double>(std::max(1, config_.simulationTickHz))};

    using clock = std::chrono::steady_clock;
    auto lastFrameAt = clock::now();

    std::fprintf(stderr, "[server] listening on %s:%u tick=%d snapshot=%d\n", config_.bindAddress.c_str(),
                 config_.listenPort, config_.simulationTickHz, config_.snapshotRateHz);

    while (running_) {
        const auto now = clock::now();
        const std::chrono::duration<double> frameDelta = now - lastFrameAt;
        lastFrameAt = now;

        PollTransport();
        LogConnectionMetricsIfDue(now);

        const int simSteps = fixedStep.Accumulate(frameDelta.count());
        for (int step = 0; step < simSteps; ++step) {
            gameState_.Step(static_cast<float>(fixedStep.StepSeconds()));
            if ((gameState_.CurrentTick() % static_cast<shared::game::TickId>(SnapshotIntervalTicks())) == 0U) {
                for (auto& [connection, session] : sessionsByConnection_) {
                    (void)connection;
                    SyncChunkSubscriptions(session);
                }
                BroadcastSnapshot();
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    SavePersistence();
    transport_.Shutdown();

    return 0;
}

void GameServer::RequestStop() { running_ = false; }

bool GameServer::LoadPersistence() {
    return LoadWorldState(config_.persistencePath, gameState_, persistenceWarning_);
}

void GameServer::SavePersistence() {
    std::string error;
    if (!SaveWorldState(config_.persistencePath, gameState_, error)) {
        std::fprintf(stderr, "[persistence.save] save failed: %s\n", error.c_str());
        return;
    }

    std::fprintf(stderr, "[persistence.save] world state saved to %s\n", config_.persistencePath.c_str());
}

void GameServer::PollTransport() {
    transport_.Poll();
    HandleConnectionEvents();
    HandleIncomingPackets();
}

void GameServer::HandleConnectionEvents() {
    const std::vector<shared::net::ConnectionEvent> events = transport_.DrainConnectionEvents();
    for (const shared::net::ConnectionEvent& event : events) {
        if (event.type == shared::net::ConnectionEventType::Connected) {
            if (static_cast<int>(sessionsByConnection_.size()) >= config_.maxClients) {
                SendDisconnect(event.connection, 4100, "server full");
            }
            continue;
        }

        if (event.type == shared::net::ConnectionEventType::ClosedByPeer ||
            event.type == shared::net::ConnectionEventType::ProblemDetectedLocally) {
            RemoveSessionByConnection(event.connection, event.reason);
        }
    }
}

void GameServer::HandleIncomingPackets() {
    const std::vector<shared::net::ReceivedPacket> packets = transport_.DrainReceivedPackets();
    for (const shared::net::ReceivedPacket& packet : packets) {
        shared::net::EnvelopeHeader header;
        std::span<const uint8_t> payload;
        std::string error;
        if (!shared::net::ParsePacket(packet.bytes, header, payload, error)) {
            std::fprintf(stderr, "[net.protocol] malformed packet dropped: %s\n", error.c_str());
            continue;
        }

        if (header.protocolVersion != shared::net::kProtocolVersion) {
            SendDisconnect(packet.connection, 4202, "protocol mismatch");
            continue;
        }

        switch (header.messageId) {
        case shared::net::MessageId::ClientHello:
            HandleClientHello(packet.connection, payload);
            break;
        case shared::net::MessageId::InputFrame:
            HandleInputFrame(packet.connection, payload);
            break;
        case shared::net::MessageId::ChunkInterestHint:
            HandleChunkInterestHint(packet.connection, payload);
            break;
        case shared::net::MessageId::ChunkResyncRequest:
            HandleChunkResyncRequest(packet.connection, payload);
            break;
        case shared::net::MessageId::Ping:
            HandlePing(packet.connection, payload);
            break;
        default:
            break;
        }
    }
}

void GameServer::HandleClientHello(shared::net::ConnectionHandle connection, std::span<const uint8_t> payload) {
    shared::net::ClientHelloMessage hello;
    std::string error;
    if (!shared::net::Deserialize(payload, hello, error)) {
        SendDisconnect(connection, 4201, "invalid ClientHello");
        return;
    }

    if (hello.requestedProtocolVersion != shared::net::kProtocolVersion) {
        SendDisconnect(connection, 4202, "protocol mismatch");
        return;
    }

    if (config_.enforceBuildHash && hello.buildCompatibilityHash != config_.requiredBuildHash) {
        SendDisconnect(connection, 4203, "build hash mismatch");
        return;
    }

    if (sessionsByConnection_.contains(connection)) {
        return;
    }

    const shared::net::AuthRequest authRequest{
        .playerName = hello.playerName,
        .token = hello.authToken,
        .sessionResumeToken = {},
    };
    const shared::net::AuthContext authContext{.remoteAddress = {}, .transportUserData = 0};
    const shared::net::AuthDecision decision = authProvider_->Validate(authRequest, authContext);
    if (!decision.accepted) {
        SendDisconnect(connection, 4204, decision.rejectionReason.empty() ? "auth rejected" : decision.rejectionReason);
        return;
    }

    ClientSession session;
    session.connection = connection;
    session.playerId = AllocatePlayerId();
    session.playerName = hello.playerName;
    session.badInputCount = 0;
    session.receivedInputThisWindow = 0;
    session.receivedChunkHintsThisWindow = 0;
    session.receivedChunkResyncThisWindow = 0;
    session.rateWindowStart = std::chrono::steady_clock::now();
    session.requestedInterestRadius = static_cast<uint16_t>(std::clamp(gameState_.World().interestRadiusChunks, 1, 16));

    sessionsByConnection_.emplace(connection, session);
    connectionByPlayer_[session.playerId] = connection;
    transport_.SetConnectionUserData(connection, session.playerId.Value());

    SpawnSessionPlayer(sessionsByConnection_.at(connection));
}

void GameServer::HandleInputFrame(shared::net::ConnectionHandle connection, std::span<const uint8_t> payload) {
    auto sessionIt = sessionsByConnection_.find(connection);
    if (sessionIt == sessionsByConnection_.end()) {
        SendDisconnect(connection, 4301, "session not authenticated");
        return;
    }

    shared::net::InputFrameMessage input;
    std::string error;
    if (!shared::net::Deserialize(payload, input, error)) {
        ClientSession& session = sessionIt->second;
        ++session.badInputCount;
        if (session.badInputCount >= 5) {
            SendDisconnect(connection, 4302, "too many malformed input frames");
        }
        return;
    }

    ClientSession& session = sessionIt->second;
    const auto now = std::chrono::steady_clock::now();
    ResetRateWindowIfNeeded(session, now);

    ++session.receivedInputThisWindow;
    if (session.receivedInputThisWindow > static_cast<uint32_t>(config_.maxInputFramesPerSecond)) {
        ++session.badInputCount;
        if (session.badInputCount >= 5) {
            SendDisconnect(connection, 4303, "input flood detected");
        }
        std::fprintf(stderr, "[net.protocol] rate limit hit type=input conn=%u count=%u limit=%d\n", connection,
                     session.receivedInputThisWindow, config_.maxInputFramesPerSecond);
        return;
    }

    const uint32_t lastReceivedSequence = [&]() -> uint32_t {
        const auto playerIt = gameState_.Players().find(session.playerId);
        return playerIt != gameState_.Players().end() ? playerIt->second.lastReceivedInputSequence : 0U;
    }();
    const shared::game::PlayerInputFrame frame{
        .clientTick = input.clientTick,
        .sequence = input.sequence,
        .moveX = input.moveX,
        .jumpPressed = input.jumpPressed,
    };
    const shared::game::PlayerInputValidationError validation =
        shared::game::ValidatePlayerInputFrame(frame, lastReceivedSequence);
    if (validation != shared::game::PlayerInputValidationError::None) {
        ++session.badInputCount;
        if (session.badInputCount >= 5) {
            SendDisconnect(connection, 4304, shared::game::ToString(validation));
        }
        return;
    }

    gameState_.ApplyInputFrame(session.playerId, frame);
}

void GameServer::HandleChunkInterestHint(shared::net::ConnectionHandle connection, std::span<const uint8_t> payload) {
    auto sessionIt = sessionsByConnection_.find(connection);
    if (sessionIt == sessionsByConnection_.end()) {
        SendDisconnect(connection, 4301, "session not authenticated");
        return;
    }

    shared::net::ChunkInterestHintMessage hint;
    std::string error;
    if (!shared::net::Deserialize(payload, hint, error)) {
        ClientSession& session = sessionIt->second;
        ++session.badInputCount;
        if (session.badInputCount >= 5) {
            SendDisconnect(connection, 4305, "too many malformed chunk hints");
        }
        return;
    }

    ClientSession& session = sessionIt->second;
    const auto now = std::chrono::steady_clock::now();
    ResetRateWindowIfNeeded(session, now);
    ++session.receivedChunkHintsThisWindow;
    if (session.receivedChunkHintsThisWindow > static_cast<uint32_t>(config_.maxChunkHintsPerSecond)) {
        ++session.badInputCount;
        if (session.badInputCount >= 5) {
            SendDisconnect(connection, 4307, "chunk hint flood detected");
        }
        std::fprintf(stderr, "[net.protocol] rate limit hit type=chunk_hint conn=%u count=%u limit=%d\n", connection,
                     session.receivedChunkHintsThisWindow, config_.maxChunkHintsPerSecond);
        return;
    }

    if (hint.radiusChunks == 0U) {
        session.requestedInterestRadius = static_cast<uint16_t>(std::clamp(gameState_.World().interestRadiusChunks, 1, 16));
        SendResyncRequired(connection, 5101, "invalid chunk interest hint; reset to default radius");
        SyncChunkSubscriptions(session);
        return;
    }

    const uint16_t maxRadius = static_cast<uint16_t>(std::clamp(gameState_.World().interestRadiusChunks * 2, 1, 24));
    session.requestedInterestRadius = std::clamp(hint.radiusChunks, static_cast<uint16_t>(1), maxRadius);
    SyncChunkSubscriptions(session);
}

void GameServer::HandleChunkResyncRequest(shared::net::ConnectionHandle connection, std::span<const uint8_t> payload) {
    auto sessionIt = sessionsByConnection_.find(connection);
    if (sessionIt == sessionsByConnection_.end()) {
        SendDisconnect(connection, 4301, "session not authenticated");
        return;
    }

    shared::net::ChunkResyncRequestMessage request;
    std::string error;
    if (!shared::net::Deserialize(payload, request, error)) {
        ClientSession& session = sessionIt->second;
        ++session.badInputCount;
        if (session.badInputCount >= 5) {
            SendDisconnect(connection, 4306, "too many malformed chunk resync requests");
        }
        return;
    }

    ClientSession& session = sessionIt->second;
    const auto now = std::chrono::steady_clock::now();
    ResetRateWindowIfNeeded(session, now);
    ++session.receivedChunkResyncThisWindow;
    if (session.receivedChunkResyncThisWindow > static_cast<uint32_t>(config_.maxChunkResyncRequestsPerSecond)) {
        ++session.badInputCount;
        if (session.badInputCount >= 5) {
            SendDisconnect(connection, 4308, "chunk resync flood detected");
        }
        std::fprintf(stderr, "[net.protocol] rate limit hit type=chunk_resync conn=%u count=%u limit=%d\n", connection,
                     session.receivedChunkResyncThisWindow, config_.maxChunkResyncRequestsPerSecond);
        return;
    }

    const shared::game::ChunkCoord requested{
        .x = request.chunkX,
        .y = request.chunkY,
    };

    if (!IsChunkVisibleForSession(session, requested)) {
        SendResyncRequired(connection, 5102, "chunk resync request outside interest set");
        return;
    }

    ChunkRecord& chunk = GetOrCreateChunk(requested);
    if (!chunk.data.IsValid()) {
        SendResyncRequired(connection, 5103, "chunk resync unavailable");
        return;
    }

    SendChunkBaseline(connection, chunk.data);
    session.subscribedChunks.insert(requested);
    session.sentChunkVersions[requested] = chunk.data.version.value;
}

void GameServer::HandlePing(shared::net::ConnectionHandle connection, std::span<const uint8_t> payload) {
    shared::net::PingMessage ping;
    std::string error;
    if (!shared::net::Deserialize(payload, ping, error)) {
        return;
    }

    const shared::net::PongMessage pong{.sequence = ping.sequence};
    SendMessage(connection, shared::net::MessageId::Pong, shared::net::Serialize(pong));
}

shared::game::PlayerId GameServer::AllocatePlayerId() {
    const shared::game::PlayerId allocated = nextPlayerId_;
    nextPlayerId_ = shared::game::PlayerId{nextPlayerId_.Value() + 1U};
    return allocated;
}

void GameServer::SpawnSessionPlayer(ClientSession& session) {
    const float spawnX = static_cast<float>((static_cast<int>(session.playerId.Value()) % 10) * 6 - 30);
    gameState_.SpawnPlayer(session.playerId, session.playerName, {spawnX, 0.0f});

    SendWelcome(session);
    SendWorldMetadata(session);

    const shared::game::SnapshotView snapshot = gameState_.BuildSnapshotView();
    for (const shared::game::SnapshotPlayerView& player : snapshot.players) {
        SendSpawnForPlayerToConnection(player, session.connection);
    }

    for (const shared::game::SnapshotPlayerView& player : snapshot.players) {
        if (player.playerId == session.playerId) {
            BroadcastSpawn(player);
            break;
        }
    }

    if (config_.fakeLagMs > 0.0f || config_.fakeJitterMs > 0.0f || config_.fakeLossSendPct > 0.0f ||
        config_.fakeLossRecvPct > 0.0f) {
        transport_.ApplyConnectionNetworkSimulation(
            session.connection,
            shared::net::NetSimConfig{
                .fakeLagMs = config_.fakeLagMs,
                .fakeJitterMs = config_.fakeJitterMs,
                .fakeLossSendPct = config_.fakeLossSendPct,
                .fakeLossRecvPct = config_.fakeLossRecvPct,
            });
    }

    SyncChunkSubscriptions(session);
}

void GameServer::RemoveSessionByConnection(shared::net::ConnectionHandle connection, const std::string& reason) {
    auto sessionIt = sessionsByConnection_.find(connection);
    if (sessionIt == sessionsByConnection_.end()) {
        return;
    }

    const ClientSession session = sessionIt->second;
    sessionsByConnection_.erase(sessionIt);
    connectionByPlayer_.erase(session.playerId);

    const auto playerIt = gameState_.Players().find(session.playerId);
    if (playerIt != gameState_.Players().end()) {
        const shared::game::EntityId entityId = playerIt->second.entityId;
        gameState_.RemovePlayer(session.playerId);
        BroadcastDespawn(entityId);
    }

    std::fprintf(stderr, "[net.transport] player disconnected id=%u reason=%s\n", session.playerId.Value(),
                 reason.c_str());
}

void GameServer::SendWelcome(ClientSession& session) {
    const shared::net::ServerWelcomeMessage welcome{
        .protocolVersion = shared::net::kProtocolVersion,
        .playerId = session.playerId,
        .serverTick = gameState_.CurrentTick(),
        .serverTickRateHz = static_cast<uint16_t>(config_.simulationTickHz),
        .snapshotRateHz = static_cast<uint16_t>(config_.snapshotRateHz),
        .playerKinematics = gameState_.Kinematics(),
    };

    SendMessage(session.connection, shared::net::MessageId::ServerWelcome, shared::net::Serialize(welcome));
}

void GameServer::SendWorldMetadata(ClientSession& session) {
    const shared::game::WorldConfig& world = gameState_.World();
    const shared::net::WorldMetadataMessage metadata{
        .chunkWidthTiles = static_cast<uint16_t>(std::clamp(world.chunkWidthTiles, 1, 65535)),
        .chunkHeightTiles = static_cast<uint16_t>(std::clamp(world.chunkHeightTiles, 1, 65535)),
        .tileSize = static_cast<uint16_t>(std::clamp(world.tileSize, 1, 65535)),
        .defaultInterestRadiusChunks = static_cast<uint16_t>(std::clamp(world.interestRadiusChunks, 1, 65535)),
    };

    SendMessage(session.connection, shared::net::MessageId::WorldMetadata, shared::net::Serialize(metadata));
}

void GameServer::SendSpawnForPlayerToConnection(const shared::game::SnapshotPlayerView& player,
                                                 shared::net::ConnectionHandle connection) {
    const shared::net::SpawnPlayerMessage spawn{
        .playerId = player.playerId,
        .entityId = player.entityId,
        .displayName = player.displayName,
        .spawnPosition = player.position,
    };

    SendMessage(connection, shared::net::MessageId::SpawnPlayer, shared::net::Serialize(spawn));
}

void GameServer::BroadcastSpawn(const shared::game::SnapshotPlayerView& player) {
    const shared::net::SpawnPlayerMessage spawn{
        .playerId = player.playerId,
        .entityId = player.entityId,
        .displayName = player.displayName,
        .spawnPosition = player.position,
    };
    const std::vector<uint8_t> payload = shared::net::Serialize(spawn);

    for (const auto& [connection, session] : sessionsByConnection_) {
        if (session.playerId == player.playerId) {
            continue;
        }

        SendMessage(connection, shared::net::MessageId::SpawnPlayer, payload);
    }
}

void GameServer::BroadcastDespawn(shared::game::EntityId entityId) {
    const shared::net::DespawnEntityMessage despawn{.entityId = entityId};
    const std::vector<uint8_t> payload = shared::net::Serialize(despawn);
    for (const auto& [connection, _] : sessionsByConnection_) {
        SendMessage(connection, shared::net::MessageId::DespawnEntity, payload);
    }
}

void GameServer::BroadcastSnapshot() {
    const shared::game::SnapshotView view = gameState_.BuildSnapshotView();
    const shared::net::SnapshotPayload payload = shared::net::BuildSnapshotPayload(view);

    shared::net::ByteWriter writer;
    shared::net::SerializeSnapshotPayload(payload, writer);
    const std::vector<uint8_t> bytes = std::move(writer).Take();

    for (const auto& [connection, _] : sessionsByConnection_) {
        SendMessage(connection, shared::net::MessageId::SnapshotDelta, bytes);
    }
}

GameServer::ChunkRecord& GameServer::GetOrCreateChunk(const shared::game::ChunkCoord& coord) {
    auto [it, inserted] = chunksByCoord_.try_emplace(coord);
    if (inserted) {
        it->second.data = shared::game::BuildProceduralChunk(gameState_.World(), coord);
    }

    return it->second;
}

void GameServer::SendChunkBaseline(shared::net::ConnectionHandle connection, const shared::game::ChunkData& chunk) {
    const shared::net::ChunkBaselineMessage message{.chunk = chunk};
    SendMessage(connection, shared::net::MessageId::ChunkBaseline, shared::net::Serialize(message));
}

void GameServer::SendChunkDelta(shared::net::ConnectionHandle connection, const shared::game::ChunkDelta& delta) {
    const shared::net::ChunkDeltaMessage message{.delta = delta};
    SendMessage(connection, shared::net::MessageId::ChunkDelta, shared::net::Serialize(message));
}

void GameServer::SendChunkUnsubscribe(shared::net::ConnectionHandle connection, const shared::game::ChunkCoord& coord) {
    const shared::net::ChunkUnsubscribeMessage message{
        .chunkX = coord.x,
        .chunkY = coord.y,
    };
    SendMessage(connection, shared::net::MessageId::ChunkUnsubscribe, shared::net::Serialize(message));
}

void GameServer::SendResyncRequired(shared::net::ConnectionHandle connection, int32_t reasonCode, const std::string& reason) {
    const shared::net::ResyncRequiredMessage message{
        .reasonCode = reasonCode,
        .reason = reason,
    };
    SendMessage(connection, shared::net::MessageId::ResyncRequired, shared::net::Serialize(message));
}

bool GameServer::IsChunkVisibleForSession(const ClientSession& session, const shared::game::ChunkCoord& chunk) const {
    const auto playerIt = gameState_.Players().find(session.playerId);
    if (playerIt == gameState_.Players().end()) {
        return false;
    }

    const shared::game::ChunkCoord center = shared::game::WorldToChunkCoord(playerIt->second.position, gameState_.World());
    const int radius = static_cast<int>(std::clamp(session.requestedInterestRadius, static_cast<uint16_t>(1),
                                                   static_cast<uint16_t>(24)));

    const int32_t dx = std::abs(chunk.x - center.x);
    const int32_t dy = std::abs(chunk.y - center.y);
    return dx <= radius && dy <= radius;
}

void GameServer::ResetRateWindowIfNeeded(ClientSession& session, std::chrono::steady_clock::time_point now) const {
    if (now - session.rateWindowStart <= std::chrono::seconds(1)) {
        return;
    }

    session.rateWindowStart = now;
    session.receivedInputThisWindow = 0;
    session.receivedChunkHintsThisWindow = 0;
    session.receivedChunkResyncThisWindow = 0;
}

void GameServer::LogConnectionMetricsIfDue(std::chrono::steady_clock::time_point now) {
    if (config_.metricsLogIntervalSeconds <= 0) {
        return;
    }

    if (now - lastMetricsLogAt_ < std::chrono::seconds(config_.metricsLogIntervalSeconds)) {
        return;
    }
    lastMetricsLogAt_ = now;

    for (const auto& [connection, session] : sessionsByConnection_) {
        const std::optional<shared::net::ConnectionMetrics> metrics = transport_.GetConnectionMetrics(connection);
        if (!metrics.has_value()) {
            continue;
        }

        const float outKbps = metrics->outBytesPerSec * 8.0f / 1000.0f;
        const float inKbps = metrics->inBytesPerSec * 8.0f / 1000.0f;
        std::fprintf(stderr,
                     "[net.transport] conn=%u player=%u ping_ms=%d jitter_us=%d queue_us=%d out_kbps=%.1f "
                     "in_kbps=%.1f pending_rel=%d pending_unrel=%d quality_local=%.2f quality_remote=%.2f\n",
                     connection, session.playerId.Value(), metrics->pingMs, metrics->jitterUsec, metrics->queuedUsec,
                     outKbps, inKbps, metrics->pendingReliableBytes, metrics->pendingUnreliableBytes,
                     metrics->qualityLocal, metrics->qualityRemote);
    }
}

void GameServer::SyncChunkSubscriptions(ClientSession& session) {
    const auto playerIt = gameState_.Players().find(session.playerId);
    if (playerIt == gameState_.Players().end()) {
        return;
    }

    const shared::game::ChunkCoord center = shared::game::WorldToChunkCoord(playerIt->second.position, gameState_.World());
    const int radius = static_cast<int>(std::clamp(session.requestedInterestRadius, static_cast<uint16_t>(1),
                                                   static_cast<uint16_t>(24)));
    const std::vector<shared::game::ChunkCoord> visible = shared::game::BuildChunkInterestArea(center, radius);
    std::unordered_set<shared::game::ChunkCoord, shared::game::ChunkCoordHash> visibleSet;
    visibleSet.reserve(visible.size());

    for (const shared::game::ChunkCoord& coord : visible) {
        visibleSet.insert(coord);
        ChunkRecord& record = GetOrCreateChunk(coord);
        if (!record.data.IsValid()) {
            continue;
        }

        const auto subscribedIt = session.subscribedChunks.find(coord);
        if (subscribedIt == session.subscribedChunks.end()) {
            SendChunkBaseline(session.connection, record.data);
            session.subscribedChunks.insert(coord);
            session.sentChunkVersions[coord] = record.data.version.value;
            continue;
        }

        const uint32_t sentVersion = [&]() -> uint32_t {
            const auto it = session.sentChunkVersions.find(coord);
            return it != session.sentChunkVersions.end() ? it->second : 0U;
        }();

        if (record.data.version.value > sentVersion) {
            shared::game::ChunkDelta delta =
                shared::game::BuildFullChunkDelta(record.data, shared::game::ChunkVersion{sentVersion});
            SendChunkDelta(session.connection, delta);
            session.sentChunkVersions[coord] = record.data.version.value;
        }
    }

    for (auto it = session.subscribedChunks.begin(); it != session.subscribedChunks.end();) {
        if (!visibleSet.contains(*it)) {
            SendChunkUnsubscribe(session.connection, *it);
            session.sentChunkVersions.erase(*it);
            it = session.subscribedChunks.erase(it);
        } else {
            ++it;
        }
    }
}

void GameServer::SendDisconnect(shared::net::ConnectionHandle connection, int32_t code, const std::string& reason) {
    const shared::net::DisconnectReasonMessage message{.reasonCode = code, .reason = reason};
    SendMessage(connection, shared::net::MessageId::DisconnectReason, shared::net::Serialize(message));

    transport_.Close(connection, code, reason, true);
    RemoveSessionByConnection(connection, reason);
}

bool GameServer::SendMessage(shared::net::ConnectionHandle connection, shared::net::MessageId messageId,
                             const std::vector<uint8_t>& payload) {
    return SendMessage(connection, messageId, payload,
                       shared::net::SendOptionsForMessage(messageId, shared::net::MessageDirection::ServerToClient));
}

bool GameServer::SendMessage(shared::net::ConnectionHandle connection, shared::net::MessageId messageId,
                             const std::vector<uint8_t>& payload, const shared::net::SendOptions& options) {
    const std::vector<uint8_t> packet = shared::net::BuildPacket(messageId, payload);
    std::string error;
    if (!transport_.Send(connection, packet, options, error)) {
        std::fprintf(stderr, "[net.protocol] send failed conn=%u msg=%u err=%s\n", connection,
                     static_cast<unsigned int>(messageId), error.c_str());
        return false;
    }

    return true;
}

int GameServer::SnapshotIntervalTicks() const {
    const int tickRate = std::max(1, config_.simulationTickHz);
    const int snapshotRate = std::max(1, config_.snapshotRateHz);
    return std::max(1, tickRate / snapshotRate);
}

}  // namespace server
