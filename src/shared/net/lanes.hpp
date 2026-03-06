#pragma once

#include <array>
#include <cstdint>

namespace shared::net {

enum class Reliability : uint8_t {
    ReliableOrdered,
    Unreliable,
};

enum class Lane : uint8_t {
    Control = 0,
    Input = 1,
    Snapshot = 2,
    World = 3,
    Gameplay = 4,
};

struct LanePolicy {
    Lane lane = Lane::Control;
    int priority = 0;
    uint16_t weight = 1;
    Reliability reliability = Reliability::ReliableOrdered;
    const char* rationale = "";
};

constexpr size_t kLaneCount = 5;

[[nodiscard]] inline const std::array<LanePolicy, kLaneCount>& DefaultLanePolicies() {
    static const std::array<LanePolicy, kLaneCount> kPolicies = {
        LanePolicy{Lane::Control,
                   100,
                   1,
                   Reliability::ReliableOrdered,
                   "Handshake/session-critical control traffic must arrive reliably and first."},
        LanePolicy{Lane::Input,
                   90,
                   4,
                   Reliability::Unreliable,
                   "Input frames are high-frequency and can drop; newer input supersedes old input."},
        LanePolicy{Lane::Snapshot,
                   70,
                   4,
                   Reliability::Unreliable,
                   "State snapshots are transient and should not back up behind reliable traffic."},
        LanePolicy{Lane::World,
                   60,
                   2,
                   Reliability::ReliableOrdered,
                   "Chunk baselines/deltas and terrain authority are mostly reliable world traffic."},
        LanePolicy{Lane::Gameplay,
                   50,
                   1,
                   Reliability::ReliableOrdered,
                   "Inventory/crafting/chat and transactional gameplay changes are reliable."},
    };
    return kPolicies;
}

[[nodiscard]] inline const LanePolicy& PolicyForLane(Lane lane) {
    return DefaultLanePolicies().at(static_cast<size_t>(lane));
}

}  // namespace shared::net
