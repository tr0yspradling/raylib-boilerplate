#include <algorithm>
#include <cassert>

#include "shared/game/chunk_streaming.hpp"

int main() {
    using namespace shared::game;

    const WorldConfig world{
        .chunkWidthTiles = 16,
        .chunkHeightTiles = 12,
        .tileSize = 1,
        .interestRadiusChunks = 3,
    };

    ChunkData authoritative = BuildProceduralChunk(world, ChunkCoord{.x = 2, .y = -1});
    assert(authoritative.IsValid());

    authoritative.version = ChunkVersion{7};
    assert(authoritative.tiles.size() > 8);
    authoritative.tiles[3] = 5U;
    authoritative.tiles[8] = 9U;

    const ChunkDelta fullDelta = BuildFullChunkDelta(authoritative, ChunkVersion{6});
    assert(fullDelta.coord == authoritative.coord);
    assert(fullDelta.baseVersion.value == 6U);
    assert(fullDelta.newVersion.value == 7U);
    assert(fullDelta.operations.size() == authoritative.tiles.size());

    ChunkData local = authoritative;
    local.version = ChunkVersion{6};
    std::fill(local.tiles.begin(), local.tiles.end(), 0U);

    assert(ApplyChunkDelta(local, fullDelta));
    assert(local.version.value == authoritative.version.value);
    assert(local.tiles == authoritative.tiles);

    ChunkData wrongVersion = local;
    wrongVersion.version = ChunkVersion{4};
    assert(!ApplyChunkDelta(wrongVersion, fullDelta));

    ChunkDelta invalid = fullDelta;
    invalid.baseVersion = local.version;
    invalid.operations.push_back(ChunkTileDeltaOp{
        .tileIndex = static_cast<uint16_t>(local.tiles.size()),
        .value = 1U,
    });
    assert(!ApplyChunkDelta(local, invalid));

    return 0;
}
