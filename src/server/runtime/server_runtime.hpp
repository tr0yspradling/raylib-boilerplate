#pragma once

#include <chrono>
#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include <flecs.h>

#include "server/config/server_config.hpp"
#include "shared/game/game_state.hpp"
#include "shared/game/ids.hpp"
#include "shared/net/auth.hpp"
#include "shared/game/fixed_step.hpp"
#include "shared/net/protocol.hpp"
#include "shared/net/transport_gns.hpp"

namespace server {

namespace runtime {

class ServerRuntime {
public:
    explicit ServerRuntime(ServerConfig config);

    [[nodiscard]] bool Initialize();
    void Start();
    void Shutdown();
    void RequestStop();
    void PollTransportPump();
    void DecodeTransportMessages();
    void RunAuthAndSessionPhase();
    void RunInputApplyPhase();
    void AdvanceSimulation(float frameSeconds);
    void RunReplicationPhase();
    void RunPersistencePhase();
    void RunMetricsPhase();

    [[nodiscard]] bool IsRunning() const;

private:
    struct ChunkRecord {
        shared::game::ChunkData data{};
    };

    struct ClientSession {
        shared::net::ConnectionHandle connection = shared::net::kInvalidConnectionHandle;
        shared::game::PlayerId playerId{};
        std::string playerName;
        uint32_t badInputCount = 0;
        uint32_t receivedInputThisWindow = 0;
        uint32_t receivedChunkHintsThisWindow = 0;
        uint32_t receivedChunkResyncThisWindow = 0;
        std::chrono::steady_clock::time_point rateWindowStart{};
        uint16_t requestedInterestRadius = 0;
        std::unordered_set<shared::game::ChunkCoord, shared::game::ChunkCoordHash> subscribedChunks;
        std::unordered_map<shared::game::ChunkCoord, uint32_t, shared::game::ChunkCoordHash> sentChunkVersions;
    };

    bool LoadPersistence();
    void SavePersistence();

    void HandleConnectionEvents();
    void HandleIncomingPackets();

    void HandleClientHello(shared::net::ConnectionHandle connection, std::span<const uint8_t> payload);
    void HandleInputFrame(shared::net::ConnectionHandle connection, std::span<const uint8_t> payload);
    void HandleChunkInterestHint(shared::net::ConnectionHandle connection, std::span<const uint8_t> payload);
    void HandleChunkResyncRequest(shared::net::ConnectionHandle connection, std::span<const uint8_t> payload);
    void HandlePing(shared::net::ConnectionHandle connection, std::span<const uint8_t> payload);

    [[nodiscard]] shared::game::PlayerId AllocatePlayerId();

    void SpawnSessionPlayer(ClientSession& session);
    void RemoveSessionByConnection(shared::net::ConnectionHandle connection, const std::string& reason);

    void SendWelcome(ClientSession& session);
    void SendSpawnForPlayerToConnection(const shared::game::SnapshotPlayerView& player,
                                        shared::net::ConnectionHandle connection);
    void BroadcastSpawn(const shared::game::SnapshotPlayerView& player);
    void BroadcastDespawn(shared::game::EntityId entityId);
    void BroadcastSnapshot();
    void SendWorldMetadata(ClientSession& session);
    void SyncChunkSubscriptions(ClientSession& session);
    [[nodiscard]] ChunkRecord& GetOrCreateChunk(const shared::game::ChunkCoord& coord);
    void SendChunkBaseline(shared::net::ConnectionHandle connection, const shared::game::ChunkData& chunk);
    void SendChunkDelta(shared::net::ConnectionHandle connection, const shared::game::ChunkDelta& delta);
    void SendChunkUnsubscribe(shared::net::ConnectionHandle connection, const shared::game::ChunkCoord& coord);
    void SendResyncRequired(shared::net::ConnectionHandle connection, int32_t reasonCode, const std::string& reason);
    [[nodiscard]] bool IsChunkVisibleForSession(const ClientSession& session, const shared::game::ChunkCoord& chunk) const;
    void ResetRateWindowIfNeeded(ClientSession& session, std::chrono::steady_clock::time_point now) const;
    void LogConnectionMetricsIfDue(std::chrono::steady_clock::time_point now);

    void SendDisconnect(shared::net::ConnectionHandle connection, int32_t code, const std::string& reason);

    bool SendMessage(shared::net::ConnectionHandle connection, shared::net::MessageId messageId,
                     const std::vector<uint8_t>& payload);
    bool SendMessage(shared::net::ConnectionHandle connection, shared::net::MessageId messageId,
                     const std::vector<uint8_t>& payload, const shared::net::SendOptions& options);

    [[nodiscard]] int SnapshotIntervalTicks() const;

    ServerConfig config_;
    shared::net::TransportGns transport_;
    shared::game::GameState gameState_;
    shared::game::FixedStep fixedStep_;

    std::unique_ptr<shared::net::IAuthProvider> authProvider_;

    std::unordered_map<shared::net::ConnectionHandle, ClientSession> sessionsByConnection_;
    std::unordered_map<shared::game::PlayerId, shared::net::ConnectionHandle, shared::game::IdHash<shared::game::PlayerIdTag>>
        connectionByPlayer_;
    std::unordered_map<shared::game::ChunkCoord, ChunkRecord, shared::game::ChunkCoordHash> chunksByCoord_;

    shared::game::PlayerId nextPlayerId_{1};

    bool initialized_ = false;
    bool running_ = false;
    std::string persistenceWarning_;
    std::chrono::steady_clock::time_point lastMetricsLogAt_{};
    size_t pendingReplicationSteps_ = 0U;
};

}  // namespace runtime

}  // namespace server
