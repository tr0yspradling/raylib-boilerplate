#pragma once

#include "shared/net/snapshot.hpp"

namespace shared::net {

// Snapshot delta compression is scaffolded. The first online slice sends full
// snapshot payloads as SnapshotDelta messages. A future phase can replace this
// with baseline+delta encoding without changing gameplay call sites.
struct DeltaCodec {
    [[nodiscard]] SnapshotPayload Encode(const SnapshotPayload& current) const { return current; }
    [[nodiscard]] SnapshotPayload Decode(const SnapshotPayload& incoming) const { return incoming; }
};

}  // namespace shared::net
