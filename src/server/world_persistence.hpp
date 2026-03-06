#pragma once

#include <string>

#include "shared/game/game_state.hpp"

namespace server {

bool SaveWorldState(const std::string& filePath, const shared::game::GameState& state, std::string& error);
bool LoadWorldState(const std::string& filePath, shared::game::GameState& state, std::string& warning);

}  // namespace server
