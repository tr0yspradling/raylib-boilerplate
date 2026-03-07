#pragma once

#include <chrono>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>

#include "client/core/client_config.hpp"
#include "client/runtime/runtime_resources.hpp"
#include "shared/game/fixed_step.hpp"
#include "shared/net/protocol.hpp"
#include "shared/net/transport.hpp"

namespace client::runtime {

class MultiplayerSessionService {
    public:
        explicit MultiplayerSessionService(ClientConfig& config, game::FixedStep& fixedStep);
        MultiplayerSessionService(ClientConfig& config, game::FixedStep& fixedStep, std::unique_ptr<net::ITransport> transport);
        ~MultiplayerSessionService();

        void Shutdown(ClientSessionState& session);
        void CloseConnection(const ClientSessionState& session, int32_t reasonCode, const std::string& reason, bool linger);

        [[nodiscard]] bool EnsureTransportInitialized(std::string& error);
        [[nodiscard]] bool BeginConnectionAttempt(ClientSessionState& session, std::string& error);

        void Poll(ClientFlowState& flow, LocalServerStartupState& localServer, ClientSessionState& session);
        void UpdateCadence(const ClientFlowState& flow, ClientSessionState& session,
                           std::chrono::steady_clock::time_point now);
        void SendInputFrame(ClientSessionState& session, const game::PlayerInputFrame& frame);

        [[nodiscard]] std::optional<net::ConnectionMetrics> ConnectionMetrics(const ClientSessionState& session) const;

    private:
        void HandleConnectionEvents(ClientFlowState& flow, LocalServerStartupState& localServer, ClientSessionState& session);
        void HandleIncomingPackets(ClientFlowState& flow, ClientSessionState& session);

        void OnConnectedToServer(ClientFlowState& flow, ClientSessionState& session);

        void HandleServerWelcome(const net::ServerWelcomeMessage& message, ClientFlowState& flow,
                                 ClientSessionState& session);
        void HandleWorldMetadata(const net::WorldMetadataMessage& message, ClientSessionState& session);
        void HandleSpawnPlayer(const net::SpawnPlayerMessage& message, ClientSessionState& session);
        void HandleDespawnEntity(const net::DespawnEntityMessage& message, ClientSessionState& session);
        void HandleSnapshot(const net::SnapshotPayload& snapshot, ClientSessionState& session);
        void HandleChunkBaseline(const net::ChunkBaselineMessage& message, ClientSessionState& session);
        void HandleChunkDelta(const net::ChunkDeltaMessage& message, ClientSessionState& session);
        void HandleChunkUnsubscribe(const net::ChunkUnsubscribeMessage& message, ClientSessionState& session);
        void HandleResyncRequired(const net::ResyncRequiredMessage& message, ClientFlowState& flow,
                                  ClientSessionState& session);
        void HandleDisconnectReason(const net::DisconnectReasonMessage& message, ClientFlowState& flow,
                                    ClientSessionState& session);

        void SendPing(ClientSessionState& session);
        void SendChunkInterestHint(ClientSessionState& session);
        void RequestChunkResync(ClientSessionState& session, const game::ChunkCoord& coord, uint32_t clientVersion);
        void ReconcileFromSnapshot(ClientSessionState& session, const net::SnapshotEntity& localEntity);
        void ResetSession(ClientFlowState& flow, ClientSessionState& session) const;

        [[nodiscard]] bool IsLocalPlayerReady(const ClientSessionState& session) const;

        ClientConfig& config_;
        game::FixedStep& fixedStep_;
        std::unique_ptr<net::ITransport> transport_;
};

}  // namespace client::runtime
