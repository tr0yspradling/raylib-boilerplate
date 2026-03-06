#include <cassert>

#include "shared/game/world.hpp"

int main() {
    using namespace shared::game;

    const WorldConfig config{
        .chunkWidthTiles = 64,
        .chunkHeightTiles = 64,
        .tileSize = 1,
        .interestRadiusChunks = 2,
    };

    const ChunkCoord origin = WorldToChunkCoord(Vec2f{0.0f, 0.0f}, config);
    assert(origin.x == 0);
    assert(origin.y == 0);

    const ChunkCoord positive = WorldToChunkCoord(Vec2f{130.0f, 70.0f}, config);
    assert(positive.x == 2);
    assert(positive.y == 1);

    const ChunkCoord negative = WorldToChunkCoord(Vec2f{-0.1f, -65.0f}, config);
    assert(negative.x == -1);
    assert(negative.y == -2);

    const std::vector<ChunkCoord> interest = BuildChunkInterestArea(ChunkCoord{.x = 1, .y = -1}, 2);
    assert(interest.size() == 25);
    const ChunkCoord expectedFirst{.x = -1, .y = -3};
    const ChunkCoord expectedLast{.x = 3, .y = 1};
    assert(interest.front() == expectedFirst);
    assert(interest.back() == expectedLast);

    return 0;
}
