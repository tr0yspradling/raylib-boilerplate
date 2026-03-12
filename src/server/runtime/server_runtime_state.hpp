#pragma once

#include <chrono>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include "shared/game/chunk.hpp"
#include "shared/game/game_state.hpp"
#include "shared/game/ids.hpp"
#include "shared/net/transport.hpp"

namespace server::runtime {

struct ChunkRecord {
    shared::game::ChunkData data{};
};

struct ClientSession {
    shared::net::ConnectionHandle connection = shared::net::kInvalidConnectionHandle;
    shared::game::PlayerId playerId{};
    std::string playerName;
    uint32_t badInputCount = 0;
    uint32_t receivedInputThisWindow = 0;
    uint32_t receivedChunkHintsThisWindow = 0;
    uint32_t receivedChunkResyncThisWindow = 0;
    std::chrono::steady_clock::time_point rateWindowStart{};
    uint16_t requestedInterestRadius = 0;
    std::unordered_set<shared::game::ChunkCoord, shared::game::ChunkCoordHash> subscribedChunks;
    std::unordered_map<shared::game::ChunkCoord, uint32_t, shared::game::ChunkCoordHash> sentChunkVersions;
};

using SessionsByConnectionMap = std::unordered_map<shared::net::ConnectionHandle, ClientSession>;
using ConnectionByPlayerMap =
    std::unordered_map<shared::game::PlayerId, shared::net::ConnectionHandle,
                       shared::game::IdHash<shared::game::PlayerIdTag>>;
using ChunksByCoordMap = std::unordered_map<shared::game::ChunkCoord, ChunkRecord, shared::game::ChunkCoordHash>;

}  // namespace server::runtime
