#include "server/runtime/server_runtime_ops.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <optional>
#include <span>
#include <unordered_set>
#include <vector>

#include "server/runtime/server_runtime_policy.hpp"
#include "shared/game/chunk_streaming.hpp"
#include "shared/game/validation.hpp"
#include "shared/game/world.hpp"
#include "shared/net/net_policy.hpp"
#include "shared/net/protocol.hpp"
#include "shared/net/send_policy.hpp"

namespace server::runtime {

namespace {

[[nodiscard]] int SnapshotIntervalTicks(const ServerRuntimeContext& context) {
    const int tickRate = std::max(1, context.config.simulationTickHz);
    const int snapshotRate = std::max(1, context.config.snapshotRateHz);
    return std::max(1, tickRate / snapshotRate);
}

void ResetRateWindowIfNeeded(ClientSession& session, std::chrono::steady_clock::time_point now) {
    if (now - session.rateWindowStart <= policy::kRateWindow) {
        return;
    }

    session.rateWindowStart = now;
    session.receivedInputThisWindow = 0;
    session.receivedChunkHintsThisWindow = 0;
    session.receivedChunkResyncThisWindow = 0;
}

bool SendMessage(ServerRuntimeContext& context, shared::net::ConnectionHandle connection,
                 shared::net::MessageId messageId, const std::vector<uint8_t>& payload,
                 const shared::net::SendOptions& options) {
    const std::vector<uint8_t> packet = shared::net::BuildPacket(messageId, payload);
    std::string error;
    if (!context.transport.Send(connection, packet, options, error)) {
        std::fprintf(stderr, "[net.protocol] send failed conn=%u msg=%u err=%s\n", connection,
                     static_cast<unsigned int>(messageId), error.c_str());
        return false;
    }

    return true;
}

bool SendMessage(ServerRuntimeContext& context, shared::net::ConnectionHandle connection,
                 shared::net::MessageId messageId, const std::vector<uint8_t>& payload) {
    return SendMessage(context, connection, messageId, payload,
                       shared::net::SendOptionsForMessage(messageId, shared::net::MessageDirection::ServerToClient));
}

void RemoveSessionByConnection(ServerRuntimeContext& context, shared::net::ConnectionHandle connection,
                               const std::string& reason);

void SendDisconnect(ServerRuntimeContext& context, shared::net::ConnectionHandle connection,
                    shared::net::policy::DisconnectCode code, const std::string& reason) {
    const shared::net::DisconnectReasonMessage message{
        .reasonCode = shared::net::policy::ToInt(code),
        .reason = reason,
    };
    SendMessage(context, connection, shared::net::MessageId::DisconnectReason, shared::net::Serialize(message));

    context.transport.Close(connection, shared::net::policy::ToInt(code), reason, true);
    RemoveSessionByConnection(context, connection, reason);
}

[[nodiscard]] shared::game::PlayerId AllocatePlayerId(ServerRuntimeContext& context) {
    const shared::game::PlayerId allocated = context.nextPlayerId;
    context.nextPlayerId = shared::game::PlayerId{context.nextPlayerId.Value() + 1U};
    return allocated;
}

void SendWelcome(ServerRuntimeContext& context, ClientSession& session) {
    const shared::net::ServerWelcomeMessage welcome{
        .protocolVersion = shared::net::kProtocolVersion,
        .playerId = session.playerId,
        .serverTick = context.gameState.CurrentTick(),
        .serverTickRateHz = static_cast<uint16_t>(context.config.simulationTickHz),
        .snapshotRateHz = static_cast<uint16_t>(context.config.snapshotRateHz),
        .playerKinematics = context.gameState.Kinematics(),
    };

    SendMessage(context, session.connection, shared::net::MessageId::ServerWelcome, shared::net::Serialize(welcome));
}

void SendWorldMetadata(ServerRuntimeContext& context, ClientSession& session) {
    const shared::game::WorldConfig& world = context.gameState.World();
    const shared::net::WorldMetadataMessage metadata{
        .chunkWidthTiles = static_cast<uint16_t>(std::clamp(
            world.chunkWidthTiles, static_cast<int>(shared::net::policy::protocol_limits::kMinWorldMetadataDimension),
            static_cast<int>(shared::net::policy::protocol_limits::kMaxWorldMetadataDimension))),
        .chunkHeightTiles = static_cast<uint16_t>(std::clamp(
            world.chunkHeightTiles, static_cast<int>(shared::net::policy::protocol_limits::kMinWorldMetadataDimension),
            static_cast<int>(shared::net::policy::protocol_limits::kMaxWorldMetadataDimension))),
        .tileSize = static_cast<uint16_t>(std::clamp(
            world.tileSize, static_cast<int>(shared::net::policy::protocol_limits::kMinWorldMetadataDimension),
            static_cast<int>(shared::net::policy::protocol_limits::kMaxWorldMetadataDimension))),
        .defaultInterestRadiusChunks = static_cast<uint16_t>(std::clamp(
            world.interestRadiusChunks,
            static_cast<int>(shared::net::policy::protocol_limits::kMinWorldMetadataDimension),
            static_cast<int>(shared::net::policy::protocol_limits::kMaxWorldMetadataDimension))),
    };

    SendMessage(context, session.connection, shared::net::MessageId::WorldMetadata, shared::net::Serialize(metadata));
}

void SendSpawnForPlayerToConnection(ServerRuntimeContext& context, const shared::game::SnapshotPlayerView& player,
                                    shared::net::ConnectionHandle connection) {
    const shared::net::SpawnPlayerMessage spawn{
        .playerId = player.playerId,
        .entityId = player.entityId,
        .displayName = player.displayName,
        .spawnPosition = player.position,
    };

    SendMessage(context, connection, shared::net::MessageId::SpawnPlayer, shared::net::Serialize(spawn));
}

void BroadcastSpawn(ServerRuntimeContext& context, const shared::game::SnapshotPlayerView& player) {
    const shared::net::SpawnPlayerMessage spawn{
        .playerId = player.playerId,
        .entityId = player.entityId,
        .displayName = player.displayName,
        .spawnPosition = player.position,
    };
    const std::vector<uint8_t> payload = shared::net::Serialize(spawn);

    for (const auto& [connection, session] : context.sessionsByConnection) {
        if (session.playerId == player.playerId) {
            continue;
        }

        SendMessage(context, connection, shared::net::MessageId::SpawnPlayer, payload);
    }
}

void BroadcastDespawn(ServerRuntimeContext& context, shared::game::EntityId entityId) {
    const shared::net::DespawnEntityMessage despawn{.entityId = entityId};
    const std::vector<uint8_t> payload = shared::net::Serialize(despawn);
    for (const auto& [connection, _] : context.sessionsByConnection) {
        SendMessage(context, connection, shared::net::MessageId::DespawnEntity, payload);
    }
}

void BroadcastSnapshot(ServerRuntimeContext& context) {
    const shared::game::SnapshotView view = context.gameState.BuildSnapshotView();
    const shared::net::SnapshotPayload payload = shared::net::BuildSnapshotPayload(view);

    shared::net::ByteWriter writer;
    shared::net::SerializeSnapshotPayload(payload, writer);
    const std::vector<uint8_t> bytes = std::move(writer).Take();

    for (const auto& [connection, _] : context.sessionsByConnection) {
        SendMessage(context, connection, shared::net::MessageId::SnapshotDelta, bytes);
    }
}

[[nodiscard]] ChunkRecord& GetOrCreateChunk(ServerRuntimeContext& context, const shared::game::ChunkCoord& coord) {
    auto [it, inserted] = context.chunksByCoord.try_emplace(coord);
    if (inserted) {
        it->second.data = shared::game::BuildProceduralChunk(context.gameState.World(), coord);
    }

    return it->second;
}

void SendChunkBaseline(ServerRuntimeContext& context, shared::net::ConnectionHandle connection,
                       const shared::game::ChunkData& chunk) {
    const shared::net::ChunkBaselineMessage message{.chunk = chunk};
    SendMessage(context, connection, shared::net::MessageId::ChunkBaseline, shared::net::Serialize(message));
}

void SendChunkDelta(ServerRuntimeContext& context, shared::net::ConnectionHandle connection,
                    const shared::game::ChunkDelta& delta) {
    const shared::net::ChunkDeltaMessage message{.delta = delta};
    SendMessage(context, connection, shared::net::MessageId::ChunkDelta, shared::net::Serialize(message));
}

void SendChunkUnsubscribe(ServerRuntimeContext& context, shared::net::ConnectionHandle connection,
                          const shared::game::ChunkCoord& coord) {
    const shared::net::ChunkUnsubscribeMessage message{
        .chunkX = coord.x,
        .chunkY = coord.y,
    };
    SendMessage(context, connection, shared::net::MessageId::ChunkUnsubscribe, shared::net::Serialize(message));
}

void SendResyncRequired(ServerRuntimeContext& context, shared::net::ConnectionHandle connection,
                        shared::net::policy::ResyncReasonCode reasonCode, const std::string& reason) {
    const shared::net::ResyncRequiredMessage message{
        .reasonCode = shared::net::policy::ToInt(reasonCode),
        .reason = reason,
    };
    SendMessage(context, connection, shared::net::MessageId::ResyncRequired, shared::net::Serialize(message));
}

[[nodiscard]] bool IsChunkVisibleForSession(const ServerRuntimeContext& context, const ClientSession& session,
                                            const shared::game::ChunkCoord& chunk) {
    const auto playerIt = context.gameState.Players().find(session.playerId);
    if (playerIt == context.gameState.Players().end()) {
        return false;
    }

    const shared::game::ChunkCoord center =
        shared::game::WorldToChunkCoord(playerIt->second.position, context.gameState.World());
    const int radius = static_cast<int>(
        std::clamp(session.requestedInterestRadius, shared::net::policy::chunk_interest::kMinRequestedRadius,
                   shared::net::policy::chunk_interest::kExpandedMaxRequestedRadius));

    const int32_t dx = std::abs(chunk.x - center.x);
    const int32_t dy = std::abs(chunk.y - center.y);
    return dx <= radius && dy <= radius;
}

void SyncChunkSubscriptions(ServerRuntimeContext& context, ClientSession& session) {
    const auto playerIt = context.gameState.Players().find(session.playerId);
    if (playerIt == context.gameState.Players().end()) {
        return;
    }

    const shared::game::ChunkCoord center =
        shared::game::WorldToChunkCoord(playerIt->second.position, context.gameState.World());
    const int radius = static_cast<int>(
        std::clamp(session.requestedInterestRadius, shared::net::policy::chunk_interest::kMinRequestedRadius,
                   shared::net::policy::chunk_interest::kExpandedMaxRequestedRadius));
    const std::vector<shared::game::ChunkCoord> visible = shared::game::BuildChunkInterestArea(center, radius);
    std::unordered_set<shared::game::ChunkCoord, shared::game::ChunkCoordHash> visibleSet;
    visibleSet.reserve(visible.size());

    for (const shared::game::ChunkCoord& coord : visible) {
        visibleSet.insert(coord);
        ChunkRecord& record = GetOrCreateChunk(context, coord);
        if (!record.data.IsValid()) {
            continue;
        }

        const auto subscribedIt = session.subscribedChunks.find(coord);
        if (subscribedIt == session.subscribedChunks.end()) {
            SendChunkBaseline(context, session.connection, record.data);
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
            SendChunkDelta(context, session.connection, delta);
            session.sentChunkVersions[coord] = record.data.version.value;
        }
    }

    for (auto it = session.subscribedChunks.begin(); it != session.subscribedChunks.end();) {
        if (!visibleSet.contains(*it)) {
            SendChunkUnsubscribe(context, session.connection, *it);
            session.sentChunkVersions.erase(*it);
            it = session.subscribedChunks.erase(it);
        } else {
            ++it;
        }
    }
}

void SpawnSessionPlayer(ServerRuntimeContext& context, ClientSession& session) {
    const float spawnX = static_cast<float>(
        (static_cast<int>(session.playerId.Value()) % policy::kSpawnColumnCount) * policy::kSpawnSpacingUnits -
        policy::kSpawnHorizontalOffset);
    context.gameState.SpawnPlayer(session.playerId, session.playerName, {spawnX, 0.0f});

    SendWelcome(context, session);
    SendWorldMetadata(context, session);

    const shared::game::SnapshotView snapshot = context.gameState.BuildSnapshotView();
    for (const shared::game::SnapshotPlayerView& player : snapshot.players) {
        SendSpawnForPlayerToConnection(context, player, session.connection);
    }

    for (const shared::game::SnapshotPlayerView& player : snapshot.players) {
        if (player.playerId == session.playerId) {
            BroadcastSpawn(context, player);
            break;
        }
    }

    if (context.config.fakeLagMs > 0.0f || context.config.fakeJitterMs > 0.0f || context.config.fakeLossSendPct > 0.0f ||
        context.config.fakeLossRecvPct > 0.0f) {
        context.transport.ApplyConnectionNetworkSimulation(
            session.connection,
            shared::net::NetSimConfig{
                .fakeLagMs = context.config.fakeLagMs,
                .fakeJitterMs = context.config.fakeJitterMs,
                .fakeLossSendPct = context.config.fakeLossSendPct,
                .fakeLossRecvPct = context.config.fakeLossRecvPct,
            });
    }

    SyncChunkSubscriptions(context, session);
}

void RemoveSessionByConnection(ServerRuntimeContext& context, shared::net::ConnectionHandle connection,
                               const std::string& reason) {
    auto sessionIt = context.sessionsByConnection.find(connection);
    if (sessionIt == context.sessionsByConnection.end()) {
        return;
    }

    const ClientSession session = sessionIt->second;
    context.sessionsByConnection.erase(sessionIt);
    context.connectionByPlayer.erase(session.playerId);

    const auto playerIt = context.gameState.Players().find(session.playerId);
    if (playerIt != context.gameState.Players().end()) {
        const shared::game::EntityId entityId = playerIt->second.entityId;
        context.gameState.RemovePlayer(session.playerId);
        BroadcastDespawn(context, entityId);
    }

    std::fprintf(stderr, "[net.transport] player disconnected id=%u reason=%s\n", session.playerId.Value(),
                 reason.c_str());
}

void HandleClientHello(ServerRuntimeContext& context, shared::net::ConnectionHandle connection,
                       std::span<const uint8_t> payload) {
    shared::net::ClientHelloMessage hello;
    std::string error;
    if (!shared::net::Deserialize(payload, hello, error)) {
        SendDisconnect(context, connection, shared::net::policy::DisconnectCode::InvalidClientHello, "invalid ClientHello");
        return;
    }

    if (hello.requestedProtocolVersion != shared::net::kProtocolVersion) {
        SendDisconnect(context, connection, shared::net::policy::DisconnectCode::ProtocolMismatch, "protocol mismatch");
        return;
    }

    if (context.config.enforceBuildHash && hello.buildCompatibilityHash != context.config.requiredBuildHash) {
        SendDisconnect(context, connection, shared::net::policy::DisconnectCode::BuildHashMismatch,
                       "build hash mismatch");
        return;
    }

    if (context.sessionsByConnection.contains(connection)) {
        return;
    }

    const shared::net::AuthRequest authRequest{
        .playerName = hello.playerName,
        .token = hello.authToken,
        .sessionResumeToken = {},
    };
    const shared::net::AuthContext authContext{.remoteAddress = {}, .transportUserData = 0};
    const shared::net::AuthDecision decision = context.authProvider->Validate(authRequest, authContext);
    if (!decision.accepted) {
        SendDisconnect(context, connection, shared::net::policy::DisconnectCode::AuthRejected,
                       decision.rejectionReason.empty() ? "auth rejected" : decision.rejectionReason);
        return;
    }

    ClientSession session;
    session.connection = connection;
    session.playerId = AllocatePlayerId(context);
    session.playerName = hello.playerName;
    session.rateWindowStart = std::chrono::steady_clock::now();
    session.requestedInterestRadius = static_cast<uint16_t>(
        std::clamp(context.gameState.World().interestRadiusChunks,
                   static_cast<int>(shared::net::policy::chunk_interest::kMinRequestedRadius),
                   static_cast<int>(shared::net::policy::chunk_interest::kDefaultMaxRequestedRadius)));

    context.sessionsByConnection.emplace(connection, session);
    context.connectionByPlayer[session.playerId] = connection;
    context.transport.SetConnectionUserData(connection, session.playerId.Value());

    SpawnSessionPlayer(context, context.sessionsByConnection.at(connection));
}

void HandleInputFrame(ServerRuntimeContext& context, shared::net::ConnectionHandle connection,
                      std::span<const uint8_t> payload) {
    auto sessionIt = context.sessionsByConnection.find(connection);
    if (sessionIt == context.sessionsByConnection.end()) {
        SendDisconnect(context, connection, shared::net::policy::DisconnectCode::SessionNotAuthenticated,
                       "session not authenticated");
        return;
    }

    shared::net::InputFrameMessage input;
    std::string error;
    if (!shared::net::Deserialize(payload, input, error)) {
        ClientSession& session = sessionIt->second;
        ++session.badInputCount;
        if (session.badInputCount >= policy::kMalformedInputThreshold) {
            SendDisconnect(context, connection, shared::net::policy::DisconnectCode::MalformedInputFrame,
                           "too many malformed input frames");
        }
        return;
    }

    ClientSession& session = sessionIt->second;
    const auto now = std::chrono::steady_clock::now();
    ResetRateWindowIfNeeded(session, now);

    ++session.receivedInputThisWindow;
    if (session.receivedInputThisWindow > static_cast<uint32_t>(context.config.maxInputFramesPerSecond)) {
        ++session.badInputCount;
        if (session.badInputCount >= policy::kMalformedInputThreshold) {
            SendDisconnect(context, connection, shared::net::policy::DisconnectCode::InputFloodDetected,
                           "input flood detected");
        }
        std::fprintf(stderr, "[net.protocol] rate limit hit type=input conn=%u count=%u limit=%d\n", connection,
                     session.receivedInputThisWindow, context.config.maxInputFramesPerSecond);
        return;
    }

    const uint32_t lastReceivedSequence = [&]() -> uint32_t {
        const auto playerIt = context.gameState.Players().find(session.playerId);
        return playerIt != context.gameState.Players().end() ? playerIt->second.lastReceivedInputSequence : 0U;
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
        if (session.badInputCount >= policy::kMalformedInputThreshold) {
            SendDisconnect(context, connection, shared::net::policy::DisconnectCode::InvalidInputFrame,
                           shared::game::ToString(validation));
        }
        return;
    }

    context.gameState.ApplyInputFrame(session.playerId, frame);
}

void HandleChunkInterestHint(ServerRuntimeContext& context, shared::net::ConnectionHandle connection,
                             std::span<const uint8_t> payload) {
    auto sessionIt = context.sessionsByConnection.find(connection);
    if (sessionIt == context.sessionsByConnection.end()) {
        SendDisconnect(context, connection, shared::net::policy::DisconnectCode::SessionNotAuthenticated,
                       "session not authenticated");
        return;
    }

    shared::net::ChunkInterestHintMessage hint;
    std::string error;
    if (!shared::net::Deserialize(payload, hint, error)) {
        ClientSession& session = sessionIt->second;
        ++session.badInputCount;
        if (session.badInputCount >= policy::kMalformedInputThreshold) {
            SendDisconnect(context, connection, shared::net::policy::DisconnectCode::MalformedChunkHint,
                           "too many malformed chunk hints");
        }
        return;
    }

    ClientSession& session = sessionIt->second;
    const auto now = std::chrono::steady_clock::now();
    ResetRateWindowIfNeeded(session, now);
    ++session.receivedChunkHintsThisWindow;
    if (session.receivedChunkHintsThisWindow > static_cast<uint32_t>(context.config.maxChunkHintsPerSecond)) {
        ++session.badInputCount;
        if (session.badInputCount >= policy::kMalformedInputThreshold) {
            SendDisconnect(context, connection, shared::net::policy::DisconnectCode::ChunkHintFloodDetected,
                           "chunk hint flood detected");
        }
        std::fprintf(stderr, "[net.protocol] rate limit hit type=chunk_hint conn=%u count=%u limit=%d\n", connection,
                     session.receivedChunkHintsThisWindow, context.config.maxChunkHintsPerSecond);
        return;
    }

    if (hint.radiusChunks == 0U) {
        session.requestedInterestRadius = static_cast<uint16_t>(
            std::clamp(context.gameState.World().interestRadiusChunks,
                       static_cast<int>(shared::net::policy::chunk_interest::kMinRequestedRadius),
                       static_cast<int>(shared::net::policy::chunk_interest::kDefaultMaxRequestedRadius)));
        SendResyncRequired(context, connection, shared::net::policy::ResyncReasonCode::InvalidChunkInterestHint,
                           "invalid chunk interest hint; reset to default radius");
        SyncChunkSubscriptions(context, session);
        return;
    }

    const uint16_t maxRadius = static_cast<uint16_t>(
        std::clamp(context.gameState.World().interestRadiusChunks * 2,
                   static_cast<int>(shared::net::policy::chunk_interest::kMinRequestedRadius),
                   static_cast<int>(shared::net::policy::chunk_interest::kExpandedMaxRequestedRadius)));
    session.requestedInterestRadius =
        std::clamp(hint.radiusChunks, shared::net::policy::chunk_interest::kMinRequestedRadius, maxRadius);
    SyncChunkSubscriptions(context, session);
}

void HandleChunkResyncRequest(ServerRuntimeContext& context, shared::net::ConnectionHandle connection,
                              std::span<const uint8_t> payload) {
    auto sessionIt = context.sessionsByConnection.find(connection);
    if (sessionIt == context.sessionsByConnection.end()) {
        SendDisconnect(context, connection, shared::net::policy::DisconnectCode::SessionNotAuthenticated,
                       "session not authenticated");
        return;
    }

    shared::net::ChunkResyncRequestMessage request;
    std::string error;
    if (!shared::net::Deserialize(payload, request, error)) {
        ClientSession& session = sessionIt->second;
        ++session.badInputCount;
        if (session.badInputCount >= policy::kMalformedInputThreshold) {
            SendDisconnect(context, connection, shared::net::policy::DisconnectCode::MalformedChunkResyncRequest,
                           "too many malformed chunk resync requests");
        }
        return;
    }

    ClientSession& session = sessionIt->second;
    const auto now = std::chrono::steady_clock::now();
    ResetRateWindowIfNeeded(session, now);
    ++session.receivedChunkResyncThisWindow;
    if (session.receivedChunkResyncThisWindow > static_cast<uint32_t>(context.config.maxChunkResyncRequestsPerSecond)) {
        ++session.badInputCount;
        if (session.badInputCount >= policy::kMalformedInputThreshold) {
            SendDisconnect(context, connection, shared::net::policy::DisconnectCode::ChunkResyncFloodDetected,
                           "chunk resync flood detected");
        }
        std::fprintf(stderr, "[net.protocol] rate limit hit type=chunk_resync conn=%u count=%u limit=%d\n", connection,
                     session.receivedChunkResyncThisWindow, context.config.maxChunkResyncRequestsPerSecond);
        return;
    }

    const shared::game::ChunkCoord requested{
        .x = request.chunkX,
        .y = request.chunkY,
    };

    if (!IsChunkVisibleForSession(context, session, requested)) {
        SendResyncRequired(context, connection, shared::net::policy::ResyncReasonCode::ChunkOutsideInterestSet,
                           "chunk resync request outside interest set");
        return;
    }

    ChunkRecord& chunk = GetOrCreateChunk(context, requested);
    if (!chunk.data.IsValid()) {
        SendResyncRequired(context, connection, shared::net::policy::ResyncReasonCode::ChunkUnavailable,
                           "chunk resync unavailable");
        return;
    }

    SendChunkBaseline(context, connection, chunk.data);
    session.subscribedChunks.insert(requested);
    session.sentChunkVersions[requested] = chunk.data.version.value;
}

void HandlePing(ServerRuntimeContext& context, shared::net::ConnectionHandle connection, std::span<const uint8_t> payload) {
    shared::net::PingMessage ping;
    std::string error;
    if (!shared::net::Deserialize(payload, ping, error)) {
        return;
    }

    const shared::net::PongMessage pong{.sequence = ping.sequence};
    SendMessage(context, connection, shared::net::MessageId::Pong, shared::net::Serialize(pong));
}

void LogConnectionMetricsIfDue(ServerRuntimeContext& context, std::chrono::steady_clock::time_point now) {
    if (context.config.metricsLogIntervalSeconds <= 0) {
        return;
    }

    if (now - context.lastMetricsLogAt < std::chrono::seconds(context.config.metricsLogIntervalSeconds)) {
        return;
    }
    context.lastMetricsLogAt = now;

    for (const auto& [connection, session] : context.sessionsByConnection) {
        const std::optional<shared::net::ConnectionMetrics> metrics = context.transport.GetConnectionMetrics(connection);
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

}  // namespace

void ServerRuntimeOps::HandleConnectionEvents(ServerRuntimeContext& context) {
    const std::vector<shared::net::ConnectionEvent> events = context.transport.DrainConnectionEvents();
    for (const shared::net::ConnectionEvent& event : events) {
        if (event.type == shared::net::ConnectionEventType::Connected) {
            if (static_cast<int>(context.sessionsByConnection.size()) >= context.config.maxClients) {
                SendDisconnect(context, event.connection, shared::net::policy::DisconnectCode::ServerFull, "server full");
            }
            continue;
        }

        if (event.type == shared::net::ConnectionEventType::ClosedByPeer ||
            event.type == shared::net::ConnectionEventType::ProblemDetectedLocally) {
            RemoveSessionByConnection(context, event.connection, event.reason);
        }
    }
}

void ServerRuntimeOps::HandleIncomingPackets(ServerRuntimeContext& context) {
    const std::vector<shared::net::ReceivedPacket> packets = context.transport.DrainReceivedPackets();
    for (const shared::net::ReceivedPacket& packet : packets) {
        shared::net::EnvelopeHeader header;
        std::span<const uint8_t> payload;
        std::string error;
        if (!shared::net::ParsePacket(packet.bytes, header, payload, error)) {
            std::fprintf(stderr, "[net.protocol] malformed packet dropped: %s\n", error.c_str());
            continue;
        }

        if (header.protocolVersion != shared::net::kProtocolVersion) {
            SendDisconnect(context, packet.connection, shared::net::policy::DisconnectCode::ProtocolMismatch,
                           "protocol mismatch");
            continue;
        }

        switch (header.messageId) {
        case shared::net::MessageId::ClientHello:
            HandleClientHello(context, packet.connection, payload);
            break;
        case shared::net::MessageId::InputFrame:
            HandleInputFrame(context, packet.connection, payload);
            break;
        case shared::net::MessageId::ChunkInterestHint:
            HandleChunkInterestHint(context, packet.connection, payload);
            break;
        case shared::net::MessageId::ChunkResyncRequest:
            HandleChunkResyncRequest(context, packet.connection, payload);
            break;
        case shared::net::MessageId::Ping:
            HandlePing(context, packet.connection, payload);
            break;
        default:
            break;
        }
    }
}

void ServerRuntimeOps::AdvanceSimulation(ServerRuntimeContext& context, float frameSeconds) {
    const int simSteps = context.fixedStep.Accumulate(frameSeconds);
    for (int step = 0; step < simSteps; ++step) {
        context.gameState.Step(static_cast<float>(context.fixedStep.StepSeconds()));
        if ((context.gameState.CurrentTick() % static_cast<shared::game::TickId>(SnapshotIntervalTicks(context))) == 0U) {
            ++context.pendingReplicationSteps;
        }
    }
}

void ServerRuntimeOps::RunReplicationPhase(ServerRuntimeContext& context) {
    while (context.pendingReplicationSteps > 0U) {
        for (auto& [connection, session] : context.sessionsByConnection) {
            (void)connection;
            SyncChunkSubscriptions(context, session);
        }
        BroadcastSnapshot(context);
        --context.pendingReplicationSteps;
    }
}

void ServerRuntimeOps::RunMetricsPhase(ServerRuntimeContext& context, std::chrono::steady_clock::time_point now) {
    LogConnectionMetricsIfDue(context, now);
}

}  // namespace server::runtime
