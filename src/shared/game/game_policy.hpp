#pragma once

#include <cstddef>
#include <cstdint>

namespace shared::game::policy {

inline constexpr float kMinNormalizedMoveAxis = -1.0f;
inline constexpr float kMaxNormalizedMoveAxis = 1.0f;

namespace world {
inline constexpr int kDefaultChunkWidthTiles = 64;
inline constexpr int kDefaultChunkHeightTiles = 64;
inline constexpr int kDefaultTileSize = 1;
inline constexpr int kDefaultInterestRadiusChunks = 4;
inline constexpr int kMinChunkDimensionTiles = 1;
inline constexpr int kMaxChunkDimensionTiles = 255;
}  // namespace world

namespace player {
inline constexpr float kDefaultMaxMoveSpeed = 8.0f;
inline constexpr float kDefaultJumpSpeed = 8.0f;
inline constexpr float kDefaultGravity = -24.0f;
inline constexpr float kDefaultMaxFallSpeed = -48.0f;
inline constexpr float kDefaultGroundY = 0.0f;
inline constexpr float kDefaultMinX = -256.0f;
inline constexpr float kDefaultMaxX = 256.0f;
}  // namespace player

namespace interpolation {
inline constexpr std::size_t kMinSamples = 2U;
inline constexpr std::size_t kDefaultMaxSamples = 32U;
}  // namespace interpolation

namespace fixed_step {
inline constexpr double kDefaultStepSeconds = 1.0 / 30.0;
inline constexpr int kDefaultMaxCatchupSteps = 8;
inline constexpr double kMinStepSeconds = 1e-6;
}  // namespace fixed_step

namespace validation {
inline constexpr float kDefaultMaxAbsMoveAxis = 1.01f;
inline constexpr uint32_t kDefaultMaxSequenceBacktrack = 2048U;
}  // namespace validation

}  // namespace shared::game::policy
