#pragma once

#include <chrono>
#include <cstdint>

namespace server::runtime::policy {

inline constexpr int kMalformedInputThreshold = 5;
inline constexpr auto kRateWindow = std::chrono::seconds{1};
inline constexpr uint16_t kMinRequestedInterestRadius = 1;
inline constexpr uint16_t kDefaultMaxRequestedInterestRadius = 16;
inline constexpr uint16_t kExpandedMaxRequestedInterestRadius = 24;
inline constexpr int kSpawnColumnCount = 10;
inline constexpr int kSpawnSpacingUnits = 6;
inline constexpr int kSpawnHorizontalOffset = 30;
inline constexpr auto kServerLoopSleep = std::chrono::milliseconds{1};

}  // namespace server::runtime::policy
