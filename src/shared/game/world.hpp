#pragma once

#include <algorithm>
#include <cmath>
#include <vector>

#include "shared/game/chunk.hpp"
#include "shared/game/ids.hpp"
#include "shared/game/math_types.hpp"

namespace shared::game {

struct WorldConfig {
    int chunkWidthTiles = 64;
    int chunkHeightTiles = 64;
    int tileSize = 1;
    int interestRadiusChunks = 4;
};

struct ChunkSubscription {
    PlayerId playerId{};
    std::vector<ChunkCoord> visibleChunks;
};

[[nodiscard]] inline ChunkCoord WorldToChunkCoord(const Vec2f& position, const WorldConfig& config) {
    const float chunkWidthWorldUnits =
        static_cast<float>(std::max(1, config.chunkWidthTiles) * std::max(1, config.tileSize));
    const float chunkHeightWorldUnits =
        static_cast<float>(std::max(1, config.chunkHeightTiles) * std::max(1, config.tileSize));

    return {
        .x = static_cast<int32_t>(std::floor(position.x / chunkWidthWorldUnits)),
        .y = static_cast<int32_t>(std::floor(position.y / chunkHeightWorldUnits)),
    };
}

[[nodiscard]] inline std::vector<ChunkCoord> BuildChunkInterestArea(const ChunkCoord& center, int radiusChunks) {
    const int radius = std::max(0, radiusChunks);
    std::vector<ChunkCoord> result;
    const int side = radius * 2 + 1;
    result.reserve(static_cast<size_t>(side * side));

    for (int32_t y = center.y - radius; y <= center.y + radius; ++y) {
        for (int32_t x = center.x - radius; x <= center.x + radius; ++x) {
            result.push_back(ChunkCoord{.x = x, .y = y});
        }
    }

    return result;
}

}  // namespace shared::game
