#include <cassert>

#include "shared/net/send_policy.hpp"

int main() {
    using namespace shared::net;

    const SendOptions clientHello = SendOptionsForMessage(MessageId::ClientHello, MessageDirection::ClientToServer);
    assert(clientHello.lane == Lane::Control);
    assert(clientHello.reliability == Reliability::ReliableOrdered);

    const SendOptions inputFrame = SendOptionsForMessage(MessageId::InputFrame, MessageDirection::ClientToServer);
    assert(inputFrame.lane == Lane::Input);
    assert(inputFrame.reliability == Reliability::Unreliable);

    const SendOptions snapshotDelta = SendOptionsForMessage(MessageId::SnapshotDelta, MessageDirection::ServerToClient);
    assert(snapshotDelta.lane == Lane::Snapshot);
    assert(snapshotDelta.reliability == Reliability::Unreliable);

    const SendOptions chunkBaseline = SendOptionsForMessage(MessageId::ChunkBaseline, MessageDirection::ServerToClient);
    assert(chunkBaseline.lane == Lane::World);
    assert(chunkBaseline.reliability == Reliability::ReliableOrdered);

    const SendOptions pong = SendOptionsForMessage(MessageId::Pong, MessageDirection::ServerToClient);
    assert(pong.lane == Lane::Control);
    assert(pong.reliability == Reliability::Unreliable);

    const SendOptions unknown = SendOptionsForMessage(MessageId::Invalid, MessageDirection::ClientToServer);
    assert(unknown.lane == Lane::Control);
    assert(unknown.reliability == Reliability::ReliableOrdered);

    return 0;
}
