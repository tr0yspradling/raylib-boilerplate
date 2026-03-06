#pragma once

#include <cstdint>
#include <span>
#include <string>
#include <vector>

#include "shared/game/chunk.hpp"
#include "shared/game/entity.hpp"
#include "shared/game/ids.hpp"
#include "shared/net/message_ids.hpp"
#include "shared/net/serializer.hpp"
#include "shared/net/snapshot.hpp"

namespace shared::net {

constexpr uint32_t kProtocolMagic = 0x52424e54;  // RBNT
constexpr uint16_t kProtocolVersion = 2;

struct EnvelopeHeader {
    uint32_t magic = kProtocolMagic;
    uint16_t protocolVersion = kProtocolVersion;
    MessageId messageId = MessageId::Invalid;
    uint32_t payloadSize = 0;
};

struct ClientHelloMessage {
    uint16_t requestedProtocolVersion = kProtocolVersion;
    uint32_t buildCompatibilityHash = 0;
    std::string playerName;
    std::string authToken;
};

struct ServerWelcomeMessage {
    uint16_t protocolVersion = kProtocolVersion;
    game::PlayerId playerId{};
    game::TickId serverTick = 0;
    uint16_t serverTickRateHz = 30;
    uint16_t snapshotRateHz = 15;
    game::PlayerKinematicsConfig playerKinematics{};
};

struct SpawnPlayerMessage {
    game::PlayerId playerId{};
    game::EntityId entityId{};
    std::string displayName;
    game::Vec2f spawnPosition{};
};

struct DespawnEntityMessage {
    game::EntityId entityId{};
};

struct InputFrameMessage {
    game::TickId clientTick = 0;
    uint32_t sequence = 0;
    float moveX = 0.0f;
    bool jumpPressed = false;
};

struct ChunkInterestHintMessage {
    int32_t centerChunkX = 0;
    int32_t centerChunkY = 0;
    uint16_t radiusChunks = 0;
};

struct ChunkResyncRequestMessage {
    int32_t chunkX = 0;
    int32_t chunkY = 0;
    uint32_t clientVersion = 0;
};

struct ChunkBaselineMessage {
    game::ChunkData chunk;
};

struct ChunkDeltaMessage {
    game::ChunkDelta delta;
};

struct WorldMetadataMessage {
    uint16_t chunkWidthTiles = 64;
    uint16_t chunkHeightTiles = 64;
    uint16_t tileSize = 1;
    uint16_t defaultInterestRadiusChunks = 4;
};

struct ChunkUnsubscribeMessage {
    int32_t chunkX = 0;
    int32_t chunkY = 0;
};

struct ResyncRequiredMessage {
    int32_t reasonCode = 0;
    std::string reason;
};

struct DisconnectReasonMessage {
    int32_t reasonCode = 0;
    std::string reason;
};

struct PingMessage {
    uint32_t sequence = 0;
};

struct PongMessage {
    uint32_t sequence = 0;
};

inline std::vector<uint8_t> BuildPacket(MessageId messageId, std::span<const uint8_t> payload) {
    ByteWriter writer;
    writer.WriteU32(kProtocolMagic);
    writer.WriteU16(kProtocolVersion);
    writer.WriteU16(static_cast<uint16_t>(messageId));
    writer.WriteU32(static_cast<uint32_t>(payload.size()));
    writer.Append(payload);
    return std::move(writer).Take();
}

inline bool ParsePacket(std::span<const uint8_t> packet, EnvelopeHeader& header, std::span<const uint8_t>& payload,
                        std::string& error) {
    ByteReader reader(packet);

    uint16_t messageRaw = 0;
    if (!reader.ReadU32(header.magic) || !reader.ReadU16(header.protocolVersion) || !reader.ReadU16(messageRaw) ||
        !reader.ReadU32(header.payloadSize)) {
        error = "protocol header underflow";
        return false;
    }

    header.messageId = static_cast<MessageId>(messageRaw);

    if (header.magic != kProtocolMagic) {
        error = "protocol magic mismatch";
        return false;
    }

    if (reader.Remaining() < header.payloadSize) {
        error = "payload length exceeds packet bytes";
        return false;
    }

    if (!reader.ReadFixedBytes(header.payloadSize, payload)) {
        error = "payload read failed";
        return false;
    }

    return true;
}

inline std::vector<uint8_t> Serialize(const ClientHelloMessage& message) {
    ByteWriter writer;
    writer.WriteU16(message.requestedProtocolVersion);
    writer.WriteU32(message.buildCompatibilityHash);
    writer.WriteString(message.playerName);
    writer.WriteString(message.authToken);
    return std::move(writer).Take();
}

inline bool Deserialize(std::span<const uint8_t> payload, ClientHelloMessage& message, std::string& error) {
    ByteReader reader(payload);
    if (!reader.ReadU16(message.requestedProtocolVersion) || !reader.ReadU32(message.buildCompatibilityHash) ||
        !reader.ReadString(message.playerName, 32) || !reader.ReadString(message.authToken, 2048)) {
        error = "invalid ClientHello";
        return false;
    }

    return true;
}

inline std::vector<uint8_t> Serialize(const ServerWelcomeMessage& message) {
    ByteWriter writer;
    writer.WriteU16(message.protocolVersion);
    writer.WriteU32(message.playerId.Value());
    writer.WriteU32(message.serverTick);
    writer.WriteU16(message.serverTickRateHz);
    writer.WriteU16(message.snapshotRateHz);
    writer.WriteFloat(message.playerKinematics.maxMoveSpeed);
    writer.WriteFloat(message.playerKinematics.jumpSpeed);
    writer.WriteFloat(message.playerKinematics.gravity);
    writer.WriteFloat(message.playerKinematics.maxFallSpeed);
    writer.WriteFloat(message.playerKinematics.groundY);
    writer.WriteFloat(message.playerKinematics.minX);
    writer.WriteFloat(message.playerKinematics.maxX);
    return std::move(writer).Take();
}

inline bool Deserialize(std::span<const uint8_t> payload, ServerWelcomeMessage& message, std::string& error) {
    ByteReader reader(payload);
    uint32_t playerIdRaw = 0;
    if (!reader.ReadU16(message.protocolVersion) || !reader.ReadU32(playerIdRaw) || !reader.ReadU32(message.serverTick) ||
        !reader.ReadU16(message.serverTickRateHz) || !reader.ReadU16(message.snapshotRateHz) ||
        !reader.ReadFloat(message.playerKinematics.maxMoveSpeed) || !reader.ReadFloat(message.playerKinematics.jumpSpeed) ||
        !reader.ReadFloat(message.playerKinematics.gravity) || !reader.ReadFloat(message.playerKinematics.maxFallSpeed) ||
        !reader.ReadFloat(message.playerKinematics.groundY) || !reader.ReadFloat(message.playerKinematics.minX) ||
        !reader.ReadFloat(message.playerKinematics.maxX)) {
        error = "invalid ServerWelcome";
        return false;
    }

    message.playerId = game::PlayerId{playerIdRaw};
    return true;
}

inline std::vector<uint8_t> Serialize(const SpawnPlayerMessage& message) {
    ByteWriter writer;
    writer.WriteU32(message.playerId.Value());
    writer.WriteU32(message.entityId.Value());
    writer.WriteFloat(message.spawnPosition.x);
    writer.WriteFloat(message.spawnPosition.y);
    writer.WriteString(message.displayName);
    return std::move(writer).Take();
}

inline bool Deserialize(std::span<const uint8_t> payload, SpawnPlayerMessage& message, std::string& error) {
    ByteReader reader(payload);
    uint32_t playerIdRaw = 0;
    uint32_t entityIdRaw = 0;
    if (!reader.ReadU32(playerIdRaw) || !reader.ReadU32(entityIdRaw) || !reader.ReadFloat(message.spawnPosition.x) ||
        !reader.ReadFloat(message.spawnPosition.y) || !reader.ReadString(message.displayName, 64)) {
        error = "invalid SpawnPlayer";
        return false;
    }

    message.playerId = game::PlayerId{playerIdRaw};
    message.entityId = game::EntityId{entityIdRaw};
    return true;
}

inline std::vector<uint8_t> Serialize(const DespawnEntityMessage& message) {
    ByteWriter writer;
    writer.WriteU32(message.entityId.Value());
    return std::move(writer).Take();
}

inline bool Deserialize(std::span<const uint8_t> payload, DespawnEntityMessage& message, std::string& error) {
    ByteReader reader(payload);
    uint32_t raw = 0;
    if (!reader.ReadU32(raw)) {
        error = "invalid DespawnEntity";
        return false;
    }

    message.entityId = game::EntityId{raw};
    return true;
}

inline std::vector<uint8_t> Serialize(const InputFrameMessage& message) {
    ByteWriter writer;
    writer.WriteU32(message.clientTick);
    writer.WriteU32(message.sequence);
    writer.WriteFloat(message.moveX);
    writer.WriteBool(message.jumpPressed);
    return std::move(writer).Take();
}

inline bool Deserialize(std::span<const uint8_t> payload, InputFrameMessage& message, std::string& error) {
    ByteReader reader(payload);
    if (!reader.ReadU32(message.clientTick) || !reader.ReadU32(message.sequence) || !reader.ReadFloat(message.moveX) ||
        !reader.ReadBool(message.jumpPressed)) {
        error = "invalid InputFrame";
        return false;
    }

    return true;
}

inline std::vector<uint8_t> Serialize(const ChunkInterestHintMessage& message) {
    ByteWriter writer;
    writer.WriteI32(message.centerChunkX);
    writer.WriteI32(message.centerChunkY);
    writer.WriteU16(message.radiusChunks);
    return std::move(writer).Take();
}

inline bool Deserialize(std::span<const uint8_t> payload, ChunkInterestHintMessage& message, std::string& error) {
    ByteReader reader(payload);
    if (!reader.ReadI32(message.centerChunkX) || !reader.ReadI32(message.centerChunkY) ||
        !reader.ReadU16(message.radiusChunks)) {
        error = "invalid ChunkInterestHint";
        return false;
    }

    return true;
}

inline std::vector<uint8_t> Serialize(const ChunkResyncRequestMessage& message) {
    ByteWriter writer;
    writer.WriteI32(message.chunkX);
    writer.WriteI32(message.chunkY);
    writer.WriteU32(message.clientVersion);
    return std::move(writer).Take();
}

inline bool Deserialize(std::span<const uint8_t> payload, ChunkResyncRequestMessage& message, std::string& error) {
    ByteReader reader(payload);
    if (!reader.ReadI32(message.chunkX) || !reader.ReadI32(message.chunkY) || !reader.ReadU32(message.clientVersion)) {
        error = "invalid ChunkResyncRequest";
        return false;
    }

    return true;
}

inline std::vector<uint8_t> Serialize(const ChunkBaselineMessage& message) {
    ByteWriter writer;
    writer.WriteI32(message.chunk.coord.x);
    writer.WriteI32(message.chunk.coord.y);
    writer.WriteU32(message.chunk.version.value);
    writer.WriteU16(message.chunk.width);
    writer.WriteU16(message.chunk.height);
    writer.WriteBytes(message.chunk.tiles);
    return std::move(writer).Take();
}

inline bool Deserialize(std::span<const uint8_t> payload, ChunkBaselineMessage& message, std::string& error) {
    ByteReader reader(payload);
    std::span<const uint8_t> tiles;
    if (!reader.ReadI32(message.chunk.coord.x) || !reader.ReadI32(message.chunk.coord.y) ||
        !reader.ReadU32(message.chunk.version.value) || !reader.ReadU16(message.chunk.width) ||
        !reader.ReadU16(message.chunk.height) || !reader.ReadBytes(tiles, 65536)) {
        error = "invalid ChunkBaseline";
        return false;
    }

    message.chunk.tiles.assign(tiles.begin(), tiles.end());
    if (!message.chunk.IsValid()) {
        error = "invalid ChunkBaseline dimensions";
        return false;
    }

    return true;
}

inline std::vector<uint8_t> Serialize(const ChunkDeltaMessage& message) {
    ByteWriter writer;
    writer.WriteI32(message.delta.coord.x);
    writer.WriteI32(message.delta.coord.y);
    writer.WriteU32(message.delta.baseVersion.value);
    writer.WriteU32(message.delta.newVersion.value);
    writer.WriteU32(static_cast<uint32_t>(message.delta.operations.size()));
    for (const game::ChunkTileDeltaOp& operation : message.delta.operations) {
        writer.WriteU16(operation.tileIndex);
        writer.WriteU8(operation.value);
    }
    return std::move(writer).Take();
}

inline bool Deserialize(std::span<const uint8_t> payload, ChunkDeltaMessage& message, std::string& error) {
    ByteReader reader(payload);
    uint32_t operationCount = 0;
    if (!reader.ReadI32(message.delta.coord.x) || !reader.ReadI32(message.delta.coord.y) ||
        !reader.ReadU32(message.delta.baseVersion.value) || !reader.ReadU32(message.delta.newVersion.value) ||
        !reader.ReadU32(operationCount)) {
        error = "invalid ChunkDelta";
        return false;
    }

    message.delta.operations.clear();
    message.delta.operations.reserve(operationCount);

    for (uint32_t index = 0; index < operationCount; ++index) {
        game::ChunkTileDeltaOp operation;
        if (!reader.ReadU16(operation.tileIndex) || !reader.ReadU8(operation.value)) {
            error = "invalid ChunkDelta operations";
            return false;
        }

        message.delta.operations.push_back(operation);
    }

    return true;
}

inline std::vector<uint8_t> Serialize(const WorldMetadataMessage& message) {
    ByteWriter writer;
    writer.WriteU16(message.chunkWidthTiles);
    writer.WriteU16(message.chunkHeightTiles);
    writer.WriteU16(message.tileSize);
    writer.WriteU16(message.defaultInterestRadiusChunks);
    return std::move(writer).Take();
}

inline bool Deserialize(std::span<const uint8_t> payload, WorldMetadataMessage& message, std::string& error) {
    ByteReader reader(payload);
    if (!reader.ReadU16(message.chunkWidthTiles) || !reader.ReadU16(message.chunkHeightTiles) ||
        !reader.ReadU16(message.tileSize) || !reader.ReadU16(message.defaultInterestRadiusChunks)) {
        error = "invalid WorldMetadata";
        return false;
    }

    return true;
}

inline std::vector<uint8_t> Serialize(const ChunkUnsubscribeMessage& message) {
    ByteWriter writer;
    writer.WriteI32(message.chunkX);
    writer.WriteI32(message.chunkY);
    return std::move(writer).Take();
}

inline bool Deserialize(std::span<const uint8_t> payload, ChunkUnsubscribeMessage& message, std::string& error) {
    ByteReader reader(payload);
    if (!reader.ReadI32(message.chunkX) || !reader.ReadI32(message.chunkY)) {
        error = "invalid ChunkUnsubscribe";
        return false;
    }

    return true;
}

inline std::vector<uint8_t> Serialize(const ResyncRequiredMessage& message) {
    ByteWriter writer;
    writer.WriteI32(message.reasonCode);
    writer.WriteString(message.reason);
    return std::move(writer).Take();
}

inline bool Deserialize(std::span<const uint8_t> payload, ResyncRequiredMessage& message, std::string& error) {
    ByteReader reader(payload);
    if (!reader.ReadI32(message.reasonCode) || !reader.ReadString(message.reason, 256)) {
        error = "invalid ResyncRequired";
        return false;
    }

    return true;
}

inline std::vector<uint8_t> Serialize(const DisconnectReasonMessage& message) {
    ByteWriter writer;
    writer.WriteI32(message.reasonCode);
    writer.WriteString(message.reason);
    return std::move(writer).Take();
}

inline bool Deserialize(std::span<const uint8_t> payload, DisconnectReasonMessage& message, std::string& error) {
    ByteReader reader(payload);
    if (!reader.ReadI32(message.reasonCode) || !reader.ReadString(message.reason, 256)) {
        error = "invalid DisconnectReason";
        return false;
    }

    return true;
}

inline std::vector<uint8_t> Serialize(const PingMessage& message) {
    ByteWriter writer;
    writer.WriteU32(message.sequence);
    return std::move(writer).Take();
}

inline bool Deserialize(std::span<const uint8_t> payload, PingMessage& message, std::string& error) {
    ByteReader reader(payload);
    if (!reader.ReadU32(message.sequence)) {
        error = "invalid Ping";
        return false;
    }

    return true;
}

inline std::vector<uint8_t> Serialize(const PongMessage& message) {
    ByteWriter writer;
    writer.WriteU32(message.sequence);
    return std::move(writer).Take();
}

inline bool Deserialize(std::span<const uint8_t> payload, PongMessage& message, std::string& error) {
    ByteReader reader(payload);
    if (!reader.ReadU32(message.sequence)) {
        error = "invalid Pong";
        return false;
    }

    return true;
}

}  // namespace shared::net
