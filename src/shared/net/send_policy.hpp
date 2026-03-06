#pragma once

#include <cstdint>

#include "shared/net/message_ids.hpp"
#include "shared/net/transport.hpp"

namespace shared::net {

enum class MessageDirection : uint8_t {
    ClientToServer = 0,
    ServerToClient = 1,
};

[[nodiscard]] inline SendOptions SendOptionsForMessage(MessageId messageId, MessageDirection direction) {
    switch (direction) {
    case MessageDirection::ClientToServer:
        switch (messageId) {
        case MessageId::ClientHello:
            return SendOptions{.lane = Lane::Control, .reliability = Reliability::ReliableOrdered, .noNagle = true};
        case MessageId::InputFrame:
            return SendOptions{.lane = Lane::Input, .reliability = Reliability::Unreliable, .noNagle = true};
        case MessageId::ChunkInterestHint:
            return SendOptions{.lane = Lane::World, .reliability = Reliability::Unreliable, .noNagle = true};
        case MessageId::ChunkResyncRequest:
            return SendOptions{.lane = Lane::World, .reliability = Reliability::ReliableOrdered, .noNagle = true};
        case MessageId::Ping:
            return SendOptions{.lane = Lane::Control, .reliability = Reliability::Unreliable, .noNagle = true};
        case MessageId::ChatMessage:
            return SendOptions{.lane = Lane::Gameplay, .reliability = Reliability::ReliableOrdered, .noNagle = true};
        default:
            break;
        }
        break;

    case MessageDirection::ServerToClient:
        switch (messageId) {
        case MessageId::ServerWelcome:
        case MessageId::SpawnPlayer:
        case MessageId::DespawnEntity:
        case MessageId::DisconnectReason:
        case MessageId::ResyncRequired:
        case MessageId::WorldMetadata:
        case MessageId::ChunkUnsubscribe:
            return SendOptions{.lane = Lane::Control, .reliability = Reliability::ReliableOrdered, .noNagle = true};
        case MessageId::SnapshotBaseline:
        case MessageId::SnapshotDelta:
            return SendOptions{.lane = Lane::Snapshot, .reliability = Reliability::Unreliable, .noNagle = true};
        case MessageId::ChunkBaseline:
        case MessageId::ChunkDelta:
        case MessageId::TerrainEditResult:
            return SendOptions{.lane = Lane::World, .reliability = Reliability::ReliableOrdered, .noNagle = true};
        case MessageId::InventoryState:
        case MessageId::CraftResult:
        case MessageId::EventBatch:
            return SendOptions{.lane = Lane::Gameplay, .reliability = Reliability::ReliableOrdered, .noNagle = true};
        case MessageId::Pong:
            return SendOptions{.lane = Lane::Control, .reliability = Reliability::Unreliable, .noNagle = true};
        default:
            break;
        }
        break;
    }

    return SendOptions{.lane = Lane::Control, .reliability = Reliability::ReliableOrdered, .noNagle = true};
}

}  // namespace shared::net
