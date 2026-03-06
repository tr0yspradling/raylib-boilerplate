#pragma once

#include <cstdint>
#include <vector>

namespace shared::game {

using ItemId = uint32_t;

struct ItemStack {
    ItemId itemId = 0;
    uint16_t quantity = 0;
};

struct InventoryState {
    std::vector<ItemStack> slots;
};

}  // namespace shared::game
