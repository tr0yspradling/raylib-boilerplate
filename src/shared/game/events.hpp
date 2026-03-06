#pragma once

#include "shared/game/ids.hpp"
#include "shared/game/math_types.hpp"

namespace shared::game {

enum class EventType : uint8_t {
    SpawnPlayer,
    DespawnEntity,
    TerrainEdited,
    InventoryChanged,
    CraftCompleted,
    CombatResolved,
};

struct GameEvent {
    EventType type = EventType::SpawnPlayer;
    PlayerId playerId{};
    EntityId entityId{};
    Vec2f worldPosition{};
};

}  // namespace shared::game
