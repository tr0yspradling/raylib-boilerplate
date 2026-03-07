#include <cassert>
#include <chrono>

#include "client/runtime/runtime_resources.hpp"

int main() {
    using client::runtime::ClientSessionState;
    using client::runtime::ResetClientSessionState;
    using client::runtime::net::kInvalidConnectionHandle;
    using shared::game::ChunkCoord;
    using shared::game::PlayerId;

    ClientSessionState session;
    session.serverConnection = 42;
    session.connecting = true;
    session.connected = true;
    session.serverWelcomed = true;
    session.clientTick = 7;
    session.latestServerTick = 11;
    session.renderInterpolationTick = 5.5f;
    session.serverTickRateHz = 60;
    session.serverSnapshotRateHz = 20;
    session.localPlayerId = PlayerId{17};
    session.pendingInputs.push_back({.clientTick = 4, .sequence = 9});
    session.remotePlayers[PlayerId{9}] = client::runtime::RemotePlayerView();
    session.chunksByCoord[ChunkCoord{.x = 2, .y = 3}] = client::runtime::ClientChunkState();
    session.chunkResyncRequestedAt[ChunkCoord{.x = 4, .y = 5}] = std::chrono::steady_clock::now();
    session.chunkVersionConflictCount = 3;
    session.nextPingSequence = 8;

    ResetClientSessionState(session);

    assert(session.serverConnection == kInvalidConnectionHandle);
    assert(!session.connecting);
    assert(!session.connected);
    assert(!session.serverWelcomed);
    assert(session.clientTick == 0);
    assert(session.latestServerTick == 0);
    assert(session.renderInterpolationTick == 0.0f);
    assert(session.localPlayerId == PlayerId{});
    assert(session.pendingInputs.empty());
    assert(session.remotePlayers.empty());
    assert(session.chunksByCoord.empty());
    assert(session.chunkResyncRequestedAt.empty());
    assert(session.chunkVersionConflictCount == 0);
    assert(session.serverTickRateHz == 60);
    assert(session.serverSnapshotRateHz == 20);
    assert(session.nextPingSequence == 8);

    return 0;
}
