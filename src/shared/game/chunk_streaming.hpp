#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>

#include "shared/game/chunk.hpp"
#include "shared/game/world.hpp"

namespace shared::game {

[[nodiscard]] inline uint32_t HashChunkCoord(const ChunkCoord& coord) {
    const uint32_t x = static_cast<uint32_t>(coord.x);
    const uint32_t y = static_cast<uint32_t>(coord.y);
    uint32_t value = x * 0x45d9f3bU ^ (y + 0x9e3779b9U);
    value ^= value >> 16U;
    value *= 0x7feb352dU;
    value ^= value >> 15U;
    return value;
}

[[nodiscard]] inline ChunkData BuildProceduralChunk(const WorldConfig& worldConfig, const ChunkCoord& coord) {
    const int width = std::clamp(worldConfig.chunkWidthTiles, 1, 255);
    const int height = std::clamp(worldConfig.chunkHeightTiles, 1, 255);

    ChunkData chunk;
    chunk.coord = coord;
    chunk.version = ChunkVersion{1};
    chunk.width = static_cast<uint16_t>(width);
    chunk.height = static_cast<uint16_t>(height);
    chunk.tiles.resize(static_cast<size_t>(width * height), 0U);

    const uint32_t coordHash = HashChunkCoord(coord);
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            const int64_t worldY = static_cast<int64_t>(coord.y) * static_cast<int64_t>(height) + static_cast<int64_t>(y);
            uint8_t material = worldY < 0 ? 1U : 0U;
            if (material != 0U) {
                const uint32_t salt = static_cast<uint32_t>((x * 31) ^ (y * 17));
                if (((coordHash ^ salt) & 0x0fU) == 0U) {
                    material = 2U;
                }
            }

            const size_t index = static_cast<size_t>(y * width + x);
            chunk.tiles[index] = material;
        }
    }

    return chunk;
}

[[nodiscard]] inline ChunkDelta BuildFullChunkDelta(const ChunkData& chunk, ChunkVersion baseVersion,
                                                    size_t maxOperations = 65535U) {
    ChunkDelta delta;
    delta.coord = chunk.coord;
    delta.baseVersion = baseVersion;
    delta.newVersion = chunk.version;
    const size_t opLimit = std::min(chunk.tiles.size(), maxOperations);
    delta.operations.reserve(opLimit);

    for (size_t index = 0; index < opLimit; ++index) {
        delta.operations.push_back(ChunkTileDeltaOp{
            .tileIndex = static_cast<uint16_t>(index),
            .value = chunk.tiles[index],
        });
    }

    return delta;
}

[[nodiscard]] inline bool CanApplyChunkDelta(const ChunkData& chunk, const ChunkDelta& delta) {
    if (!chunk.IsValid()) {
        return false;
    }

    if (chunk.coord != delta.coord) {
        return false;
    }

    if (chunk.version.value != delta.baseVersion.value) {
        return false;
    }

    for (const ChunkTileDeltaOp& operation : delta.operations) {
        if (operation.tileIndex >= chunk.tiles.size()) {
            return false;
        }
    }

    return true;
}

[[nodiscard]] inline bool ApplyChunkDelta(ChunkData& chunk, const ChunkDelta& delta) {
    if (!CanApplyChunkDelta(chunk, delta)) {
        return false;
    }

    for (const ChunkTileDeltaOp& operation : delta.operations) {
        chunk.tiles[operation.tileIndex] = operation.value;
    }
    chunk.version = delta.newVersion;
    return true;
}

}  // namespace shared::game
