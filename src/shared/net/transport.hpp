
#pragma once

#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <vector>

#include "shared/net/lanes.hpp"

namespace shared::net {

using ConnectionHandle = uint32_t;
constexpr ConnectionHandle kInvalidConnectionHandle = 0;

enum class ConnectionEventType : uint8_t {
    Connecting,
    Connected,
    ClosedByPeer,
    ProblemDetectedLocally,
    ListenSocketFailure,
};

struct ConnectionEvent {
    ConnectionEventType type = ConnectionEventType::Connecting;
    ConnectionHandle connection = kInvalidConnectionHandle;
    int32_t reasonCode = 0;
    std::string reason;
    std::string remoteAddress;
};

struct ReceivedPacket {
    ConnectionHandle connection = kInvalidConnectionHandle;
    Lane lane = Lane::Control;
    bool reliable = true;
    std::vector<uint8_t> bytes;
};

struct ConnectionMetrics {
    int pingMs = 0;
    int pendingReliableBytes = 0;
    int pendingUnreliableBytes = 0;
    int queuedUsec = 0;
    int jitterUsec = 0;
    float outBytesPerSec = 0.0f;
    float inBytesPerSec = 0.0f;
    float qualityLocal = 0.0f;
    float qualityRemote = 0.0f;
};

struct TransportConfig {
    bool isServer = false;
    int debugVerbosity = 4;
    bool allowUnencryptedDev = true;
};

struct SendOptions {
    Lane lane = Lane::Control;
    Reliability reliability = Reliability::ReliableOrdered;
    bool noNagle = false;
};

struct NetSimConfig {
    float fakeLagMs = 0.0f;
    float fakeJitterMs = 0.0f;
    float fakeLossSendPct = 0.0f;
    float fakeLossRecvPct = 0.0f;
};

class ITransport {
public:
    virtual ~ITransport() = default;

    virtual bool Initialize(const TransportConfig& config, std::string& error) = 0;
    virtual void Shutdown() = 0;

    [[nodiscard]] virtual bool IsInitialized() const = 0;

    virtual bool StartServer(uint16_t listenPort, std::string& error) = 0;
    virtual ConnectionHandle Connect(const std::string& host, uint16_t port, std::string& error) = 0;

    virtual void Poll() = 0;

    virtual bool Send(ConnectionHandle connection, std::span<const uint8_t> payload, const SendOptions& options,
                      std::string& error) = 0;

    virtual bool Close(ConnectionHandle connection, int32_t reasonCode, const std::string& reason, bool linger) = 0;

    [[nodiscard]] virtual std::vector<ConnectionEvent> DrainConnectionEvents() = 0;
    [[nodiscard]] virtual std::vector<ReceivedPacket> DrainReceivedPackets() = 0;

    virtual bool SetConnectionUserData(ConnectionHandle connection, uint64_t userData) = 0;
    [[nodiscard]] virtual uint64_t GetConnectionUserData(ConnectionHandle connection) const = 0;

    [[nodiscard]] virtual std::optional<ConnectionMetrics> GetConnectionMetrics(ConnectionHandle connection) const = 0;

    virtual bool ApplyConnectionNetworkSimulation(ConnectionHandle connection, const NetSimConfig& simulation) = 0;
};

}  // namespace shared::net
