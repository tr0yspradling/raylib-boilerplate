#pragma once

#include <string>
#include <vector>

#include "shared/game/game_state.hpp"
#include "shared/net/serializer.hpp"

namespace shared::net {

struct SnapshotEntity {
    game::PlayerId playerId{};
    game::EntityId entityId{};
    std::string displayName;
    game::Vec2f position{};
    game::Vec2f velocity{};
    bool onGround = true;
    uint32_t lastProcessedInputSequence = 0;
};

struct SnapshotPayload {
    game::TickId serverTick = 0;
    std::vector<SnapshotEntity> entities;
};

inline void SerializeSnapshotPayload(const SnapshotPayload& payload, ByteWriter& writer) {
    writer.WriteU32(payload.serverTick);
    writer.WriteU16(static_cast<uint16_t>(payload.entities.size()));
    for (const SnapshotEntity& entity : payload.entities) {
        writer.WriteU32(entity.playerId.Value());
        writer.WriteU32(entity.entityId.Value());
        writer.WriteFloat(entity.position.x);
        writer.WriteFloat(entity.position.y);
        writer.WriteFloat(entity.velocity.x);
        writer.WriteFloat(entity.velocity.y);
        writer.WriteBool(entity.onGround);
        writer.WriteU32(entity.lastProcessedInputSequence);
        writer.WriteString(entity.displayName);
    }
}

inline bool DeserializeSnapshotPayload(ByteReader& reader, SnapshotPayload& out, std::string& error) {
    uint16_t count = 0;
    if (!reader.ReadU32(out.serverTick) || !reader.ReadU16(count)) {
        error = "snapshot header underflow";
        return false;
    }

    out.entities.clear();
    out.entities.reserve(count);

    for (uint16_t index = 0; index < count; ++index) {
        SnapshotEntity entity;
        uint32_t playerIdRaw = 0;
        uint32_t entityIdRaw = 0;

        if (!reader.ReadU32(playerIdRaw) || !reader.ReadU32(entityIdRaw) || !reader.ReadFloat(entity.position.x) ||
            !reader.ReadFloat(entity.position.y) || !reader.ReadFloat(entity.velocity.x) ||
            !reader.ReadFloat(entity.velocity.y) || !reader.ReadBool(entity.onGround) ||
            !reader.ReadU32(entity.lastProcessedInputSequence) || !reader.ReadString(entity.displayName, 64)) {
            error = "snapshot payload underflow";
            return false;
        }

        entity.playerId = game::PlayerId{playerIdRaw};
        entity.entityId = game::EntityId{entityIdRaw};
        out.entities.push_back(std::move(entity));
    }

    return true;
}

inline SnapshotPayload BuildSnapshotPayload(const game::SnapshotView& view) {
    SnapshotPayload payload;
    payload.serverTick = view.tick;
    payload.entities.reserve(view.players.size());

    for (const game::SnapshotPlayerView& player : view.players) {
        payload.entities.push_back({
            .playerId = player.playerId,
            .entityId = player.entityId,
            .displayName = player.displayName,
            .position = player.position,
            .velocity = player.velocity,
            .onGround = player.onGround,
            .lastProcessedInputSequence = player.lastProcessedInputSequence,
        });
    }

    return payload;
}

}  // namespace shared::net
