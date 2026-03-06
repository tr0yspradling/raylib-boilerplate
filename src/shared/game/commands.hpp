#pragma once

#include <cstdint>
#include <string>

#include "shared/game/chunk.hpp"
#include "shared/game/entity.hpp"
#include "shared/game/ids.hpp"

namespace shared::game {

struct AimInput {
    float worldX = 0.0f;
    float worldY = 0.0f;
};

struct UseItemIntent {
    uint32_t itemSlot = 0;
};

struct DigIntent {
    int32_t tileX = 0;
    int32_t tileY = 0;
};

struct PlaceIntent {
    int32_t tileX = 0;
    int32_t tileY = 0;
    uint32_t itemSlot = 0;
};

struct InventoryActionIntent {
    uint32_t fromSlot = 0;
    uint32_t toSlot = 0;
    uint16_t quantity = 0;
};

struct CraftIntent {
    std::string recipeId;
};

struct ChunkInterestHint {
    ChunkCoord center{};
    uint8_t radius = 0;
};

// Additional combat/building intent structs are intentionally deferred to keep
// this initial networking slice small and compilable.

}  // namespace shared::game
