#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace shared::game {

struct ChunkCoord {
    int32_t x = 0;
    int32_t y = 0;

    friend bool operator==(const ChunkCoord&, const ChunkCoord&) = default;
};

struct ChunkCoordHash {
    std::size_t operator()(const ChunkCoord& coord) const noexcept {
        const uint64_t packed = (static_cast<uint64_t>(static_cast<uint32_t>(coord.x)) << 32U) |
            static_cast<uint32_t>(coord.y);
        return std::hash<uint64_t>{}(packed);
    }
};

struct ChunkVersion {
    uint32_t value = 0;
};

struct ChunkTileDeltaOp {
    uint16_t tileIndex = 0;
    uint8_t value = 0;
};

struct ChunkData {
    ChunkCoord coord{};
    ChunkVersion version{1};
    uint16_t width = 0;
    uint16_t height = 0;
    std::vector<uint8_t> tiles;

    [[nodiscard]] size_t TileCount() const { return static_cast<size_t>(width) * static_cast<size_t>(height); }
    [[nodiscard]] bool IsValid() const { return width > 0 && height > 0 && tiles.size() == TileCount(); }
};

struct ChunkDelta {
    ChunkCoord coord{};
    ChunkVersion baseVersion{};
    ChunkVersion newVersion{};
    std::vector<ChunkTileDeltaOp> operations;
};

}  // namespace shared::game
