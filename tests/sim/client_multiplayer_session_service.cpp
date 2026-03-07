#include <cassert>
#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <utility>
#include <vector>

#include "client/runtime/multiplayer_session_service.hpp"

namespace {

class FakeTransport final : public shared::net::ITransport {
    public:
        struct SentPacketRecord {
            shared::net::ConnectionHandle connection = shared::net::kInvalidConnectionHandle;
            std::vector<uint8_t> bytes;
            shared::net::SendOptions options;
        };

        bool Initialize(const shared::net::TransportConfig&, std::string& error) override {
            initialized = true;
            error.clear();
            return true;
        }

        void Shutdown() override { initialized = false; }

        [[nodiscard]] bool IsInitialized() const override { return initialized; }

        bool StartServer(uint16_t, std::string& error) override {
            error = "unsupported";
            return false;
        }

        shared::net::ConnectionHandle Connect(const std::string& host, uint16_t port, std::string& error) override {
            lastHost = host;
            lastPort = port;
            error.clear();
            return nextConnection;
        }

        void Poll() override { ++pollCount; }

        bool Send(shared::net::ConnectionHandle connection, std::span<const uint8_t> payload,
                  const shared::net::SendOptions& options, std::string& error) override {
            sentPackets.push_back({
                .connection = connection,
                .bytes = std::vector<uint8_t>(payload.begin(), payload.end()),
                .options = options,
            });
            error.clear();
            return true;
        }

        bool Close(shared::net::ConnectionHandle connection, int32_t reasonCode, const std::string& reason,
                   bool linger) override {
            closedConnections.push_back(connection);
            lastCloseReasonCode = reasonCode;
            lastCloseReason = reason;
            lastCloseLinger = linger;
            return true;
        }

        [[nodiscard]] std::vector<shared::net::ConnectionEvent> DrainConnectionEvents() override {
            return std::exchange(connectionEvents, {});
        }

        [[nodiscard]] std::vector<shared::net::ReceivedPacket> DrainReceivedPackets() override {
            return std::exchange(receivedPackets, {});
        }

        bool SetConnectionUserData(shared::net::ConnectionHandle, uint64_t) override { return true; }

        [[nodiscard]] uint64_t GetConnectionUserData(shared::net::ConnectionHandle) const override { return 0; }

        [[nodiscard]] std::optional<shared::net::ConnectionMetrics>
        GetConnectionMetrics(shared::net::ConnectionHandle) const override {
            return std::nullopt;
        }

        bool ApplyConnectionNetworkSimulation(shared::net::ConnectionHandle, const shared::net::NetSimConfig&) override {
            return true;
        }

        bool initialized = false;
        shared::net::ConnectionHandle nextConnection = 77;
        std::string lastHost;
        uint16_t lastPort = 0;
        int pollCount = 0;
        std::vector<shared::net::ConnectionEvent> connectionEvents;
        std::vector<shared::net::ReceivedPacket> receivedPackets;
        std::vector<SentPacketRecord> sentPackets;
        std::vector<shared::net::ConnectionHandle> closedConnections;
        int32_t lastCloseReasonCode = 0;
        std::string lastCloseReason;
        bool lastCloseLinger = false;
};

}  // namespace

int main() {
    using client::runtime::ClientFlowState;
    using client::runtime::ClientSessionState;
    using client::runtime::LocalServerStartupState;
    using client::runtime::MultiplayerSessionService;
    namespace core = client::core;
    namespace game = shared::game;
    namespace net = shared::net;

    client::ClientConfig config;
    config.serverHost = "192.168.0.10";
    config.serverPort = 28000;
    config.playerName = "alice";
    config.buildCompatibilityHash = 42;

    game::FixedStep fixedStep(1.0 / 30.0);
    auto transport = std::make_unique<FakeTransport>();
    FakeTransport* transportPtr = transport.get();
    MultiplayerSessionService service(config, fixedStep, std::move(transport));

    ClientFlowState flow;
    flow.runtime.mode = core::RuntimeMode::JoiningServer;
    flow.runtime.joiningInProgress = true;
    LocalServerStartupState localServer;
    ClientSessionState session;

    std::string error;
    assert(service.EnsureTransportInitialized(error));
    assert(service.BeginConnectionAttempt(session, error));
    assert(session.serverConnection == 77);
    assert(transportPtr->lastHost == "192.168.0.10");
    assert(transportPtr->lastPort == 28000);

    session.connecting = true;
    transportPtr->connectionEvents.push_back({
        .type = net::ConnectionEventType::Connected,
        .connection = session.serverConnection,
    });

    service.Poll(flow, localServer, session);

    assert(session.connected);
    assert(!session.connecting);
    assert(flow.statusMessage == "Connected, waiting for server welcome...");
    assert(transportPtr->pollCount == 1);
    assert(transportPtr->sentPackets.size() == 1);

    net::EnvelopeHeader header;
    std::span<const uint8_t> payload;
    assert(net::ParsePacket(transportPtr->sentPackets.front().bytes, header, payload, error));
    assert(header.messageId == net::MessageId::ClientHello);

    net::ClientHelloMessage hello;
    assert(net::Deserialize(payload, hello, error));
    assert(hello.playerName == "alice");
    assert(hello.buildCompatibilityHash == 42);

    const net::ConnectionHandle connection = session.serverConnection;
    transportPtr->connectionEvents.push_back({
        .type = net::ConnectionEventType::ProblemDetectedLocally,
        .connection = connection,
        .reason = "timeout",
    });

    service.Poll(flow, localServer, session);

    assert(!session.connected);
    assert(session.serverConnection == net::kInvalidConnectionHandle);
    assert(flow.statusMessage == "Join failed: timeout");
    assert(flow.disconnectReason.empty());

    return 0;
}
