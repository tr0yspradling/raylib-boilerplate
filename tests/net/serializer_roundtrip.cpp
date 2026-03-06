#include <cassert>
#include <string>
#include <vector>

#include "shared/net/protocol.hpp"
#include "shared/net/snapshot.hpp"

int main() {
    using namespace shared;

    const net::ClientHelloMessage outHello{
        .requestedProtocolVersion = net::kProtocolVersion,
        .buildCompatibilityHash = 12345,
        .playerName = "alice",
        .authToken = "dev-token",
    };

    const std::vector<uint8_t> helloPayload = net::Serialize(outHello);
    const std::vector<uint8_t> helloPacket = net::BuildPacket(net::MessageId::ClientHello, helloPayload);

    net::EnvelopeHeader header;
    std::span<const uint8_t> parsedPayload;
    std::string error;
    assert(net::ParsePacket(helloPacket, header, parsedPayload, error));
    assert(header.messageId == net::MessageId::ClientHello);

    net::ClientHelloMessage inHello;
    assert(net::Deserialize(parsedPayload, inHello, error));
    assert(inHello.playerName == outHello.playerName);
    assert(inHello.authToken == outHello.authToken);
    assert(inHello.buildCompatibilityHash == outHello.buildCompatibilityHash);

    const net::ServerWelcomeMessage outWelcome{
        .protocolVersion = net::kProtocolVersion,
        .playerId = game::PlayerId{5},
        .serverTick = 42,
        .serverTickRateHz = 30,
        .snapshotRateHz = 15,
        .playerKinematics =
            game::PlayerKinematicsConfig{
                .maxMoveSpeed = 7.5f,
                .jumpSpeed = 9.25f,
                .gravity = -25.0f,
                .maxFallSpeed = -50.0f,
                .groundY = 1.0f,
                .minX = -100.0f,
                .maxX = 100.0f,
            },
    };
    const std::vector<uint8_t> welcomePayload = net::Serialize(outWelcome);
    net::ServerWelcomeMessage inWelcome;
    assert(net::Deserialize(welcomePayload, inWelcome, error));
    assert(inWelcome.protocolVersion == outWelcome.protocolVersion);
    assert(inWelcome.playerId == outWelcome.playerId);
    assert(inWelcome.playerKinematics.maxMoveSpeed == outWelcome.playerKinematics.maxMoveSpeed);
    assert(inWelcome.playerKinematics.gravity == outWelcome.playerKinematics.gravity);

    const net::ChunkBaselineMessage outChunkBaseline{
        .chunk =
            game::ChunkData{
                .coord = game::ChunkCoord{.x = -2, .y = 4},
                .version = game::ChunkVersion{7},
                .width = 4,
                .height = 2,
                .tiles = {0, 1, 2, 3, 4, 5, 6, 7},
            },
    };
    const std::vector<uint8_t> chunkBaselinePayload = net::Serialize(outChunkBaseline);
    net::ChunkBaselineMessage inChunkBaseline;
    assert(net::Deserialize(chunkBaselinePayload, inChunkBaseline, error));
    assert(inChunkBaseline.chunk.coord == outChunkBaseline.chunk.coord);
    assert(inChunkBaseline.chunk.version.value == outChunkBaseline.chunk.version.value);
    assert(inChunkBaseline.chunk.tiles == outChunkBaseline.chunk.tiles);

    const net::ChunkDeltaMessage outChunkDelta{
        .delta =
            game::ChunkDelta{
                .coord = game::ChunkCoord{.x = -2, .y = 4},
                .baseVersion = game::ChunkVersion{7},
                .newVersion = game::ChunkVersion{8},
                .operations = {
                    game::ChunkTileDeltaOp{.tileIndex = 1, .value = 9},
                    game::ChunkTileDeltaOp{.tileIndex = 6, .value = 11},
                },
            },
    };
    const std::vector<uint8_t> chunkDeltaPayload = net::Serialize(outChunkDelta);
    net::ChunkDeltaMessage inChunkDelta;
    assert(net::Deserialize(chunkDeltaPayload, inChunkDelta, error));
    assert(inChunkDelta.delta.coord == outChunkDelta.delta.coord);
    assert(inChunkDelta.delta.baseVersion.value == 7);
    assert(inChunkDelta.delta.newVersion.value == 8);
    assert(inChunkDelta.delta.operations.size() == 2);
    assert(inChunkDelta.delta.operations[1].tileIndex == 6);
    assert(inChunkDelta.delta.operations[1].value == 11);

    const net::WorldMetadataMessage outWorldMetadata{
        .chunkWidthTiles = 96,
        .chunkHeightTiles = 64,
        .tileSize = 2,
        .defaultInterestRadiusChunks = 5,
    };
    const std::vector<uint8_t> worldMetadataPayload = net::Serialize(outWorldMetadata);
    net::WorldMetadataMessage inWorldMetadata;
    assert(net::Deserialize(worldMetadataPayload, inWorldMetadata, error));
    assert(inWorldMetadata.chunkWidthTiles == outWorldMetadata.chunkWidthTiles);
    assert(inWorldMetadata.tileSize == outWorldMetadata.tileSize);

    const net::ChunkUnsubscribeMessage outChunkUnsubscribe{
        .chunkX = -3,
        .chunkY = 7,
    };
    const std::vector<uint8_t> chunkUnsubscribePayload = net::Serialize(outChunkUnsubscribe);
    net::ChunkUnsubscribeMessage inChunkUnsubscribe;
    assert(net::Deserialize(chunkUnsubscribePayload, inChunkUnsubscribe, error));
    assert(inChunkUnsubscribe.chunkX == outChunkUnsubscribe.chunkX);
    assert(inChunkUnsubscribe.chunkY == outChunkUnsubscribe.chunkY);

    const net::ChunkResyncRequestMessage outChunkResyncRequest{
        .chunkX = -3,
        .chunkY = 7,
        .clientVersion = 12,
    };
    const std::vector<uint8_t> chunkResyncPayload = net::Serialize(outChunkResyncRequest);
    net::ChunkResyncRequestMessage inChunkResyncRequest;
    assert(net::Deserialize(chunkResyncPayload, inChunkResyncRequest, error));
    assert(inChunkResyncRequest.chunkX == outChunkResyncRequest.chunkX);
    assert(inChunkResyncRequest.chunkY == outChunkResyncRequest.chunkY);
    assert(inChunkResyncRequest.clientVersion == outChunkResyncRequest.clientVersion);

    const net::ResyncRequiredMessage outResync{
        .reasonCode = 5101,
        .reason = "invalid chunk hint",
    };
    const std::vector<uint8_t> resyncPayload = net::Serialize(outResync);
    net::ResyncRequiredMessage inResync;
    assert(net::Deserialize(resyncPayload, inResync, error));
    assert(inResync.reasonCode == outResync.reasonCode);
    assert(inResync.reason == outResync.reason);

    net::SnapshotPayload outSnapshot;
    outSnapshot.serverTick = 77;
    outSnapshot.entities.push_back({
        .playerId = game::PlayerId{1},
        .entityId = game::EntityId{1},
        .displayName = "alice",
        .position = {10.0f, 5.0f},
        .velocity = {1.0f, 0.0f},
        .onGround = false,
        .lastProcessedInputSequence = 99,
    });

    net::ByteWriter writer;
    net::SerializeSnapshotPayload(outSnapshot, writer);
    net::ByteReader reader(writer.Data());

    net::SnapshotPayload inSnapshot;
    assert(net::DeserializeSnapshotPayload(reader, inSnapshot, error));
    assert(inSnapshot.serverTick == outSnapshot.serverTick);
    assert(inSnapshot.entities.size() == 1);
    assert(inSnapshot.entities[0].displayName == "alice");
    assert(inSnapshot.entities[0].lastProcessedInputSequence == 99);

    return 0;
}
