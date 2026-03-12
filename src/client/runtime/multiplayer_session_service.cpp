#include "client/runtime/multiplayer_session_service.hpp"

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <unordered_set>
#include <utility>
#include <vector>

#include "client/physics/movement_system.hpp"
#include "shared/game/chunk_streaming.hpp"
#include "shared/game/validation.hpp"
#include "shared/game/world.hpp"
#include "shared/net/net_policy.hpp"
#include "shared/net/protocol.hpp"
#include "shared/net/send_policy.hpp"
#include "shared/net/transport_gns.hpp"

namespace client::runtime {

MultiplayerSessionService::MultiplayerSessionService(ClientConfig& config, game::FixedStep& fixedStep) :
    MultiplayerSessionService(config, fixedStep, std::make_unique<net::TransportGns>()) {}

MultiplayerSessionService::MultiplayerSessionService(ClientConfig& config, game::FixedStep& fixedStep,
                                                     std::unique_ptr<net::ITransport> transport) :
    config_(config),
    fixedStep_(fixedStep),
    transport_(std::move(transport)) {}

MultiplayerSessionService::~MultiplayerSessionService() = default;

void MultiplayerSessionService::Shutdown(ClientSessionState& session) {
    if (transport_->IsInitialized() && session.connected && session.serverConnection != net::kInvalidConnectionHandle) {
        transport_->Close(session.serverConnection, net::policy::ToInt(net::policy::DisconnectCode::ClientShutdown),
                          "client shutdown", false);
    }
    if (transport_->IsInitialized()) {
        transport_->Shutdown();
    }
}

void MultiplayerSessionService::CloseConnection(const ClientSessionState& session, int32_t reasonCode,
                                                const std::string& reason, bool linger) {
    if (!transport_->IsInitialized() || session.serverConnection == net::kInvalidConnectionHandle) {
        return;
    }

    transport_->Close(session.serverConnection, reasonCode, reason, linger);
}

bool MultiplayerSessionService::EnsureTransportInitialized(std::string& error) {
    if (transport_->IsInitialized()) {
        error.clear();
        return true;
    }

    if (!transport_->Initialize(
            net::TransportConfig{.isServer = false,
                                 .debugVerbosity = net::policy::kTransportDebugVerbosity,
                                 .allowUnencryptedDev = net::policy::kAllowUnencryptedDevTransport},
            error)) {
        return false;
    }

    error.clear();
    return true;
}

bool MultiplayerSessionService::BeginConnectionAttempt(ClientSessionState& session, std::string& error) {
    session.serverConnection = transport_->Connect(config_.serverHost, config_.serverPort, error);
    return session.serverConnection != net::kInvalidConnectionHandle;
}

void MultiplayerSessionService::Poll(ClientFlowState& flow, LocalServerStartupState& localServer,
                                     ClientSessionState& session) {
    if (!transport_->IsInitialized()) {
        return;
    }

    transport_->Poll();
    HandleConnectionEvents(flow, localServer, session);
    HandleIncomingPackets(flow, session);
}

void MultiplayerSessionService::UpdateCadence(const ClientFlowState& flow, ClientSessionState& session,
                                              std::chrono::steady_clock::time_point now) {
    if (flow.runtime.mode != core::RuntimeMode::Multiplayer || !session.connected) {
        return;
    }

    const auto sinceLastPing = std::chrono::duration_cast<std::chrono::milliseconds>(now - session.lastPingSentAt);
    if (sinceLastPing >= net::policy::client::kPingInterval) {
        SendPing(session);
        session.lastPingSentAt = now;
    }

    const auto sinceLastChunkHint =
        std::chrono::duration_cast<std::chrono::milliseconds>(now - session.lastChunkHintSentAt);
    if (session.serverWelcomed && IsLocalPlayerReady(session) &&
        sinceLastChunkHint >= net::policy::client::kChunkHintInterval) {
        SendChunkInterestHint(session);
        session.lastChunkHintSentAt = now;
    }
}

void MultiplayerSessionService::SendInputFrame(ClientSessionState& session, const game::PlayerInputFrame& frame) {
    net::InputFrameMessage inputMessage;
    inputMessage.clientTick = frame.clientTick;
    inputMessage.sequence = frame.sequence;
    inputMessage.moveX = frame.moveX;
    inputMessage.jumpPressed = frame.jumpPressed;

    const std::vector<uint8_t> payload = net::Serialize(inputMessage);
    const std::vector<uint8_t> packet = net::BuildPacket(net::MessageId::InputFrame, payload);

    std::string error;
    if (!transport_->Send(
            session.serverConnection, packet,
            net::SendOptionsForMessage(net::MessageId::InputFrame, net::MessageDirection::ClientToServer), error)) {
        std::fprintf(stderr, "[net.protocol] input send failed: %s\n", error.c_str());
    }
}

std::optional<net::ConnectionMetrics> MultiplayerSessionService::ConnectionMetrics(const ClientSessionState& session) const {
    if (!transport_->IsInitialized() || session.serverConnection == net::kInvalidConnectionHandle) {
        return std::nullopt;
    }
    return transport_->GetConnectionMetrics(session.serverConnection);
}

void MultiplayerSessionService::HandleConnectionEvents(ClientFlowState& flow, LocalServerStartupState& localServer,
                                                       ClientSessionState& session) {
    const std::vector<net::ConnectionEvent> events = transport_->DrainConnectionEvents();
    for (const net::ConnectionEvent& event : events) {
        if (event.connection != session.serverConnection) {
            continue;
        }

        if (event.type == net::ConnectionEventType::Connected) {
            session.connected = true;
            session.connecting = false;
            flow.statusMessage = localServer.startupInProgress
                ? "Connected to local dedicated server, waiting for server welcome..."
                : "Connected, waiting for server welcome...";
            OnConnectedToServer(flow, session);
            continue;
        }

        if (event.type == net::ConnectionEventType::ClosedByPeer ||
            event.type == net::ConnectionEventType::ProblemDetectedLocally) {
            session.connected = false;
            flow.disconnectReason = event.reason.empty() ? "connection closed" : event.reason;
            ResetSession(flow, session);

            if (localServer.startupInProgress) {
                flow.disconnectReason.clear();
                flow.runtime.disconnectReason.clear();
                flow.runtime.mode = core::RuntimeMode::StartingLocalServer;
                flow.runtime.joiningInProgress = false;
                flow.statusMessage = "Waiting for local dedicated server...";
                continue;
            }

            if (flow.runtime.mode == core::RuntimeMode::JoiningServer) {
                flow.statusMessage = "Join failed: " + flow.disconnectReason;
                flow.disconnectReason.clear();
                flow.runtime.disconnectReason.clear();
            }
        }
    }
}

void MultiplayerSessionService::HandleIncomingPackets(ClientFlowState& flow, ClientSessionState& session) {
    const std::vector<net::ReceivedPacket> packets = transport_->DrainReceivedPackets();
    for (const net::ReceivedPacket& packet : packets) {
        if (packet.connection != session.serverConnection) {
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
            const net::ConnectionHandle connection = session.serverConnection;
            flow.disconnectReason = "protocol version mismatch";
            session.connected = false;
            ResetSession(flow, session);
            if (connection != net::kInvalidConnectionHandle) {
                transport_->Close(connection, net::policy::ToInt(net::policy::DisconnectCode::ClientProtocolVersionMismatch),
                                  flow.disconnectReason, false);
            }
            continue;
        }

        switch (header.messageId) {
            case net::MessageId::ServerWelcome:
                {
                    net::ServerWelcomeMessage welcome;
                    if (net::Deserialize(payload, welcome, parseError)) {
                        HandleServerWelcome(welcome, flow, session);
                    }
                    break;
                }
            case net::MessageId::WorldMetadata:
                {
                    net::WorldMetadataMessage metadata;
                    if (net::Deserialize(payload, metadata, parseError)) {
                        HandleWorldMetadata(metadata, session);
                    }
                    break;
                }
            case net::MessageId::SpawnPlayer:
                {
                    net::SpawnPlayerMessage spawn;
                    if (net::Deserialize(payload, spawn, parseError)) {
                        HandleSpawnPlayer(spawn, session);
                    }
                    break;
                }
            case net::MessageId::DespawnEntity:
                {
                    net::DespawnEntityMessage despawn;
                    if (net::Deserialize(payload, despawn, parseError)) {
                        HandleDespawnEntity(despawn, session);
                    }
                    break;
                }
            case net::MessageId::SnapshotBaseline:
            case net::MessageId::SnapshotDelta:
                {
                    net::ByteReader reader(payload);
                    net::SnapshotPayload snapshot;
                    if (net::DeserializeSnapshotPayload(reader, snapshot, parseError)) {
                        HandleSnapshot(snapshot, session);
                    }
                    break;
                }
            case net::MessageId::ChunkBaseline:
                {
                    net::ChunkBaselineMessage baseline;
                    if (net::Deserialize(payload, baseline, parseError)) {
                        HandleChunkBaseline(baseline, session);
                    }
                    break;
                }
            case net::MessageId::ChunkDelta:
                {
                    net::ChunkDeltaMessage delta;
                    if (net::Deserialize(payload, delta, parseError)) {
                        HandleChunkDelta(delta, session);
                    }
                    break;
                }
            case net::MessageId::ChunkUnsubscribe:
                {
                    net::ChunkUnsubscribeMessage unsubscribe;
                    if (net::Deserialize(payload, unsubscribe, parseError)) {
                        HandleChunkUnsubscribe(unsubscribe, session);
                    }
                    break;
                }
            case net::MessageId::ResyncRequired:
                {
                    net::ResyncRequiredMessage resync;
                    if (net::Deserialize(payload, resync, parseError)) {
                        HandleResyncRequired(resync, flow, session);
                    }
                    break;
                }
            case net::MessageId::DisconnectReason:
                {
                    net::DisconnectReasonMessage reason;
                    if (net::Deserialize(payload, reason, parseError)) {
                        HandleDisconnectReason(reason, flow, session);
                    }
                    break;
                }
            case net::MessageId::Pong:
            default:
                break;
        }
    }
}

void MultiplayerSessionService::OnConnectedToServer(ClientFlowState& flow, ClientSessionState& session) {
    net::ClientHelloMessage hello;
    hello.requestedProtocolVersion = net::kProtocolVersion;
    hello.buildCompatibilityHash = config_.buildCompatibilityHash;
    hello.playerName = config_.playerName;
    hello.authToken = std::string{net::policy::kDevAuthToken};

    const std::vector<uint8_t> payload = net::Serialize(hello);
    const std::vector<uint8_t> packet = net::BuildPacket(net::MessageId::ClientHello, payload);

    std::string error;
    if (!transport_->Send(
            session.serverConnection, packet,
            net::SendOptionsForMessage(net::MessageId::ClientHello, net::MessageDirection::ClientToServer), error)) {
        const net::ConnectionHandle connection = session.serverConnection;
        flow.disconnectReason = "failed to send ClientHello: " + error;
        session.connected = false;
        ResetSession(flow, session);
        if (connection != net::kInvalidConnectionHandle) {
            transport_->Close(connection, net::policy::ToInt(net::policy::DisconnectCode::ClientHelloSendFailed),
                              flow.disconnectReason, false);
        }
    }
}

void MultiplayerSessionService::HandleServerWelcome(const net::ServerWelcomeMessage& message, ClientFlowState& flow,
                                                    ClientSessionState& session) {
    if (message.protocolVersion != net::kProtocolVersion) {
        const net::ConnectionHandle connection = session.serverConnection;
        flow.disconnectReason = "protocol mismatch";
        session.connected = false;
        ResetSession(flow, session);
        if (connection != net::kInvalidConnectionHandle) {
            transport_->Close(connection, net::policy::ToInt(net::policy::DisconnectCode::ClientWelcomeProtocolMismatch),
                              flow.disconnectReason, false);
        }
        return;
    }

    const game::PlayerKinematicsValidationError kinematicsValidation =
        game::ValidatePlayerKinematicsConfig(message.playerKinematics);
    if (kinematicsValidation != game::PlayerKinematicsValidationError::None) {
        const net::ConnectionHandle connection = session.serverConnection;
        flow.disconnectReason = std::string{"invalid server kinematics: "} + game::ToString(kinematicsValidation);
        session.connected = false;
        ResetSession(flow, session);
        if (connection != net::kInvalidConnectionHandle) {
            transport_->Close(connection, net::policy::ToInt(net::policy::DisconnectCode::ClientInvalidServerKinematics),
                              flow.disconnectReason, false);
        }
        return;
    }

    session.serverWelcomed = true;
    session.localPlayerId = message.playerId;
    session.latestServerTick = message.serverTick;
    session.renderInterpolationTick = static_cast<float>(message.serverTick);
    session.serverTickRateHz = std::max<uint16_t>(1, message.serverTickRateHz);
    session.serverSnapshotRateHz = std::max<uint16_t>(1, message.snapshotRateHz);
    session.serverKinematics = message.playerKinematics;
    fixedStep_.SetStepSeconds(1.0 / static_cast<double>(session.serverTickRateHz));

    session.predictedLocalPlayer.playerId = message.playerId;
    session.predictedLocalPlayer.entityId = game::EntityId{message.playerId.Value()};
    session.predictedLocalPlayer.displayName = config_.playerName;
    session.predictedLocalPlayer.position = {0.0f, 0.0f};
    session.predictedLocalPlayer.velocity = {0.0f, 0.0f};
    session.predictedLocalPlayer.onGround = true;
    session.pendingInputs.clear();
    session.chunksByCoord.clear();
    session.chunkVersionConflictCount = 0;
    session.serverWorldConfig = game::WorldConfig{};
    session.hasWorldMetadata = false;
}

void MultiplayerSessionService::HandleWorldMetadata(const net::WorldMetadataMessage& message, ClientSessionState& session) {
    if (message.chunkWidthTiles == 0 || message.chunkHeightTiles == 0 || message.tileSize == 0 ||
        message.defaultInterestRadiusChunks == 0) {
        std::fprintf(stderr, "[world.chunk] invalid world metadata ignored\n");
        return;
    }

    session.serverWorldConfig.chunkWidthTiles = static_cast<int>(message.chunkWidthTiles);
    session.serverWorldConfig.chunkHeightTiles = static_cast<int>(message.chunkHeightTiles);
    session.serverWorldConfig.tileSize = static_cast<int>(message.tileSize);
    session.serverWorldConfig.interestRadiusChunks = static_cast<int>(message.defaultInterestRadiusChunks);
    session.hasWorldMetadata = true;
}

void MultiplayerSessionService::HandleSpawnPlayer(const net::SpawnPlayerMessage& message, ClientSessionState& session) {
    if (message.playerId == session.localPlayerId) {
        session.predictedLocalPlayer.position = message.spawnPosition;
        session.predictedLocalPlayer.entityId = message.entityId;
        session.predictedLocalPlayer.displayName = message.displayName;
        return;
    }

    RemotePlayerView& remote = session.remotePlayers[message.playerId];
    remote.playerId = message.playerId;
    remote.entityId = message.entityId;
    remote.displayName = message.displayName;
    remote.latestPosition = message.spawnPosition;
    remote.interpolation.Push({.tick = session.latestServerTick, .position = message.spawnPosition});
}

void MultiplayerSessionService::HandleDespawnEntity(const net::DespawnEntityMessage& message, ClientSessionState& session) {
    for (auto it = session.remotePlayers.begin(); it != session.remotePlayers.end(); ++it) {
        if (it->second.entityId == message.entityId) {
            session.remotePlayers.erase(it);
            return;
        }
    }
}

void MultiplayerSessionService::HandleSnapshot(const net::SnapshotPayload& snapshot, ClientSessionState& session) {
    session.latestServerTick = snapshot.serverTick;

    std::unordered_set<game::PlayerId, game::IdHash<game::PlayerIdTag>> seenRemotePlayers;
    for (const net::SnapshotEntity& entity : snapshot.entities) {
        if (entity.playerId == session.localPlayerId) {
            ReconcileFromSnapshot(session, entity);
            continue;
        }

        RemotePlayerView& remote = session.remotePlayers[entity.playerId];
        remote.playerId = entity.playerId;
        remote.entityId = entity.entityId;
        remote.displayName = entity.displayName;
        remote.latestPosition = entity.position;
        remote.interpolation.Push({.tick = snapshot.serverTick, .position = entity.position});
        seenRemotePlayers.insert(entity.playerId);
    }

    for (auto it = session.remotePlayers.begin(); it != session.remotePlayers.end();) {
        if (!seenRemotePlayers.contains(it->first)) {
            it = session.remotePlayers.erase(it);
        } else {
            ++it;
        }
    }
}

void MultiplayerSessionService::HandleChunkBaseline(const net::ChunkBaselineMessage& message, ClientSessionState& session) {
    if (!message.chunk.IsValid()) {
        std::fprintf(stderr, "[world.chunk] drop invalid baseline %d,%d\n", message.chunk.coord.x, message.chunk.coord.y);
        return;
    }

    ClientChunkState& chunk = session.chunksByCoord[message.chunk.coord];
    chunk.chunk = message.chunk;
    session.chunkResyncRequestedAt.erase(message.chunk.coord);
}

void MultiplayerSessionService::HandleChunkDelta(const net::ChunkDeltaMessage& message, ClientSessionState& session) {
    auto chunkIt = session.chunksByCoord.find(message.delta.coord);
    if (chunkIt == session.chunksByCoord.end()) {
        ++session.chunkVersionConflictCount;
        std::fprintf(stderr, "[world.chunk] delta for unknown chunk %d,%d\n", message.delta.coord.x, message.delta.coord.y);
        RequestChunkResync(session, message.delta.coord, 0U);
        return;
    }

    game::ChunkData& chunk = chunkIt->second.chunk;
    if (chunk.version.value != message.delta.baseVersion.value) {
        ++session.chunkVersionConflictCount;
        std::fprintf(stderr, "[world.chunk] version mismatch chunk %d,%d local=%u base=%u\n", message.delta.coord.x,
                     message.delta.coord.y, chunk.version.value, message.delta.baseVersion.value);
        RequestChunkResync(session, message.delta.coord, chunk.version.value);
        return;
    }

    if (!game::ApplyChunkDelta(chunk, message.delta)) {
        ++session.chunkVersionConflictCount;
        std::fprintf(stderr, "[world.chunk] invalid delta ops chunk %d,%d ops=%zu\n", message.delta.coord.x,
                     message.delta.coord.y, message.delta.operations.size());
        RequestChunkResync(session, message.delta.coord, chunk.version.value);
    }
}

void MultiplayerSessionService::HandleChunkUnsubscribe(const net::ChunkUnsubscribeMessage& message,
                                                       ClientSessionState& session) {
    const game::ChunkCoord coord{
        .x = message.chunkX,
        .y = message.chunkY,
    };
    session.chunksByCoord.erase(coord);
    session.chunkResyncRequestedAt.erase(coord);
}

void MultiplayerSessionService::HandleResyncRequired(const net::ResyncRequiredMessage& message, ClientFlowState& flow,
                                                     ClientSessionState& session) {
    session.chunksByCoord.clear();
    session.chunkResyncRequestedAt.clear();
    session.chunkVersionConflictCount = 0;
    flow.disconnectReason = message.reason;
    session.lastChunkHintSentAt = std::chrono::steady_clock::now() - net::policy::client::kPingInterval;
}

void MultiplayerSessionService::HandleDisconnectReason(const net::DisconnectReasonMessage& message, ClientFlowState& flow,
                                                       ClientSessionState& session) {
    flow.disconnectReason = message.reason;
    session.connected = false;
    ResetSession(flow, session);

    if (flow.runtime.mode == core::RuntimeMode::JoiningServer) {
        flow.statusMessage = "Join failed: " + flow.disconnectReason;
        flow.disconnectReason.clear();
        flow.runtime.disconnectReason.clear();
    }
}

void MultiplayerSessionService::SendPing(ClientSessionState& session) {
    net::PingMessage ping{.sequence = session.nextPingSequence++};
    const std::vector<uint8_t> payload = net::Serialize(ping);
    const std::vector<uint8_t> packet = net::BuildPacket(net::MessageId::Ping, payload);
    std::string error;
    transport_->Send(session.serverConnection, packet,
                     net::SendOptionsForMessage(net::MessageId::Ping, net::MessageDirection::ClientToServer), error);
}

void MultiplayerSessionService::SendChunkInterestHint(ClientSessionState& session) {
    const game::WorldConfig& worldConfig = session.hasWorldMetadata ? session.serverWorldConfig : game::WorldConfig{};
    const game::ChunkCoord center = game::WorldToChunkCoord(session.predictedLocalPlayer.position, worldConfig);
    const net::ChunkInterestHintMessage hint{
        .centerChunkX = center.x,
        .centerChunkY = center.y,
        .radiusChunks = static_cast<uint16_t>(std::clamp(worldConfig.interestRadiusChunks,
                                                         static_cast<int>(net::policy::chunk_interest::kMinRequestedRadius),
                                                         static_cast<int>(net::policy::chunk_interest::kExpandedMaxRequestedRadius))),
    };

    const std::vector<uint8_t> payload = net::Serialize(hint);
    const std::vector<uint8_t> packet = net::BuildPacket(net::MessageId::ChunkInterestHint, payload);

    std::string error;
    transport_->Send(session.serverConnection, packet,
                     net::SendOptionsForMessage(net::MessageId::ChunkInterestHint, net::MessageDirection::ClientToServer),
                     error);
}

void MultiplayerSessionService::RequestChunkResync(ClientSessionState& session, const game::ChunkCoord& coord,
                                                   uint32_t clientVersion) {
    if (!session.connected || !session.serverWelcomed) {
        return;
    }

    const auto now = std::chrono::steady_clock::now();
    const auto it = session.chunkResyncRequestedAt.find(coord);
    if (it != session.chunkResyncRequestedAt.end()) {
        const auto since = std::chrono::duration_cast<std::chrono::milliseconds>(now - it->second);
        if (since < net::policy::client::kChunkResyncCooldown) {
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
    if (transport_->Send(
            session.serverConnection, packet,
            net::SendOptionsForMessage(net::MessageId::ChunkResyncRequest, net::MessageDirection::ClientToServer),
            error)) {
        session.chunkResyncRequestedAt[coord] = now;
    }
}

void MultiplayerSessionService::ReconcileFromSnapshot(ClientSessionState& session,
                                                      const net::SnapshotEntity& localEntity) {
    physics::MovementSystem::Reconcile(session.predictedLocalPlayer, localEntity, session.pendingInputs,
                                       static_cast<float>(fixedStep_.StepSeconds()), session.serverKinematics);
}

void MultiplayerSessionService::ResetSession(ClientFlowState& flow, ClientSessionState& session) const {
    flow.runtime.joiningInProgress = false;
    ResetClientSessionState(session);
}

bool MultiplayerSessionService::IsLocalPlayerReady(const ClientSessionState& session) const {
    return session.serverWelcomed && session.localPlayerId.IsValid();
}

}  // namespace client::runtime
