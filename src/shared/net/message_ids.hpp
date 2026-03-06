#pragma once

#include <cstdint>

namespace shared::net {

enum class MessageId : uint16_t {
    Invalid = 0,

    // Client -> Server
    ClientHello = 1,
    AuthResponse = 2,
    SessionResumeRequest = 3,
    InputFrame = 4,
    AimInput = 5,
    UseItemIntent = 6,
    DigIntent = 7,
    PlaceIntent = 8,
    InventoryActionIntent = 9,
    CraftIntent = 10,
    ChunkInterestHint = 11,
    Ping = 12,
    ChatMessage = 13,
    ChunkResyncRequest = 14,

    // Server -> Client
    ServerWelcome = 100,
    AuthChallenge = 101,
    AuthResult = 102,
    SpawnPlayer = 103,
    DespawnEntity = 104,
    SnapshotBaseline = 105,
    SnapshotDelta = 106,
    ChunkBaseline = 107,
    ChunkDelta = 108,
    TerrainEditResult = 109,
    InventoryState = 110,
    CraftResult = 111,
    EventBatch = 112,
    Pong = 113,
    DisconnectReason = 114,
    ResyncRequired = 115,
    WorldMetadata = 116,
    ChunkUnsubscribe = 117,
};

}  // namespace shared::net
