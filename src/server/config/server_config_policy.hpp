#pragma once

#include <cstdint>
#include <string_view>

#include "shared/game/game_policy.hpp"

namespace server::config::policy {

inline constexpr std::string_view kDefaultBindAddress = "0.0.0.0";
inline constexpr uint16_t kDefaultListenPort = 27020;
inline constexpr int kDefaultSimulationTickHz = 30;
inline constexpr int kDefaultSnapshotRateHz = 15;
inline constexpr int kDefaultMaxClients = 64;
inline constexpr int kDefaultMaxInputFramesPerSecond = 120;
inline constexpr int kDefaultMaxChunkHintsPerSecond = 30;
inline constexpr int kDefaultMaxChunkResyncRequestsPerSecond = 60;
inline constexpr int kDefaultMetricsLogIntervalSeconds = 10;
inline constexpr std::string_view kDefaultPersistencePath = "server_data/world_state.txt";
inline constexpr std::string_view kDefaultConfigPath = "src/server/config/server.cfg";
inline constexpr std::string_view kUsageText =
    "Usage: game_server [--config PATH] [--port PORT] [--tick-rate HZ] [--snapshot-rate HZ]";

inline constexpr int kMinPositiveRate = 1;
inline constexpr int kMinMetricsLogIntervalSeconds = 0;
inline constexpr int kMaxMetricsLogIntervalSeconds = 600;
inline constexpr float kMinNetSimMs = 0.0f;
inline constexpr float kMinPacketLossPct = 0.0f;
inline constexpr float kMaxPacketLossPct = 100.0f;

inline constexpr int kMinWorldTileSize = shared::game::policy::world::kMinChunkDimensionTiles;
inline constexpr int kMaxWorldChunkDimensionTiles = shared::game::policy::world::kMaxChunkDimensionTiles;

}  // namespace server::config::policy
