#include "shared/net/transport_gns.hpp"

#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <deque>
#include <string>
#include <unordered_map>
#include <utility>

#include <steam/isteamnetworkingsockets.h>
#include <steam/isteamnetworkingutils.h>
#include <steam/steamnetworkingtypes.h>
#include <steam/steamnetworkingsockets.h>

namespace shared::net {

namespace {

[[nodiscard]] ConnectionHandle ToConnectionHandle(HSteamNetConnection connection) {
    return connection == k_HSteamNetConnection_Invalid ? kInvalidConnectionHandle
                                                        : static_cast<ConnectionHandle>(connection);
}

[[nodiscard]] HSteamNetConnection ToSteamConnection(ConnectionHandle connection) {
    return connection == kInvalidConnectionHandle ? k_HSteamNetConnection_Invalid
                                                  : static_cast<HSteamNetConnection>(connection);
}

[[nodiscard]] ConnectionEventType ToConnectionEventType(ESteamNetworkingConnectionState state) {
    switch (state) {
    case k_ESteamNetworkingConnectionState_Connecting:
        return ConnectionEventType::Connecting;
    case k_ESteamNetworkingConnectionState_Connected:
        return ConnectionEventType::Connected;
    case k_ESteamNetworkingConnectionState_ClosedByPeer:
        return ConnectionEventType::ClosedByPeer;
    case k_ESteamNetworkingConnectionState_ProblemDetectedLocally:
        return ConnectionEventType::ProblemDetectedLocally;
    default:
        return ConnectionEventType::ProblemDetectedLocally;
    }
}

void DebugSpew(ESteamNetworkingSocketsDebugOutputType type, const char* message) {
    std::fprintf(stderr, "[net.transport][gns][%d] %s\n", static_cast<int>(type), message);
}

}  // namespace

class TransportGns::Impl {
public:
    bool Initialize(const TransportConfig& config, std::string& error) {
        if (initialized_) {
            return true;
        }

        if (callbackOwner_ != nullptr && callbackOwner_ != this) {
            error = "only one TransportGns instance can be initialized per process";
            return false;
        }

        SteamDatagramErrMsg errorMessage{};
        if (!GameNetworkingSockets_Init(nullptr, errorMessage)) {
            error = std::string{"GameNetworkingSockets_Init failed: "} + errorMessage;
            return false;
        }

        sockets_ = SteamNetworkingSockets();
        utils_ = SteamNetworkingUtils();
        if (sockets_ == nullptr || utils_ == nullptr) {
            error = "failed to acquire SteamNetworkingSockets interfaces";
            GameNetworkingSockets_Kill();
            return false;
        }

        config_ = config;
        callbackOwner_ = this;

        const int clampedVerbosity = std::clamp(config.debugVerbosity, 0, 8);
        utils_->SetDebugOutputFunction(static_cast<ESteamNetworkingSocketsDebugOutputType>(clampedVerbosity), &DebugSpew);
        if (!utils_->SetGlobalCallback_SteamNetConnectionStatusChanged(&OnConnectionStatusChangedStatic)) {
            error = "failed to register connection status callback";
            callbackOwner_ = nullptr;
            sockets_ = nullptr;
            utils_ = nullptr;
            GameNetworkingSockets_Kill();
            return false;
        }

        if (config.allowUnencryptedDev) {
            utils_->SetGlobalConfigValueInt32(k_ESteamNetworkingConfig_IP_AllowWithoutAuth, 1);
        }

        initialized_ = true;
        return true;
    }

    void Shutdown() {
        if (!initialized_) {
            return;
        }

        for (const auto& [connectionHandle, _] : connections_) {
            const HSteamNetConnection connection = ToSteamConnection(connectionHandle);
            sockets_->CloseConnection(connection, 0, "transport shutdown", false);
        }
        connections_.clear();
        connectionUserData_.clear();

        if (pollGroup_ != k_HSteamNetPollGroup_Invalid) {
            sockets_->DestroyPollGroup(pollGroup_);
            pollGroup_ = k_HSteamNetPollGroup_Invalid;
        }

        if (listenSocket_ != k_HSteamListenSocket_Invalid) {
            sockets_->CloseListenSocket(listenSocket_);
            listenSocket_ = k_HSteamListenSocket_Invalid;
        }

        if (callbackOwner_ == this) {
            callbackOwner_ = nullptr;
            if (utils_ != nullptr) {
                utils_->SetGlobalCallback_SteamNetConnectionStatusChanged(nullptr);
            }
        }

        sockets_ = nullptr;
        utils_ = nullptr;
        initialized_ = false;

        GameNetworkingSockets_Kill();
    }

    [[nodiscard]] bool IsInitialized() const { return initialized_; }

    bool StartServer(uint16_t listenPort, std::string& error) {
        if (!initialized_) {
            error = "transport not initialized";
            return false;
        }

        if (listenSocket_ != k_HSteamListenSocket_Invalid) {
            error = "listen socket already created";
            return false;
        }

        SteamNetworkingIPAddr serverAddress;
        serverAddress.Clear();
        serverAddress.m_port = listenPort;

        listenSocket_ = sockets_->CreateListenSocketIP(serverAddress, 0, nullptr);
        if (listenSocket_ == k_HSteamListenSocket_Invalid) {
            error = "CreateListenSocketIP failed";
            events_.push_back({
                .type = ConnectionEventType::ListenSocketFailure,
                .connection = kInvalidConnectionHandle,
                .reasonCode = -1,
                .reason = "CreateListenSocketIP failed",
                .remoteAddress = {},
            });
            return false;
        }

        pollGroup_ = sockets_->CreatePollGroup();
        if (pollGroup_ == k_HSteamNetPollGroup_Invalid) {
            error = "CreatePollGroup failed";
            sockets_->CloseListenSocket(listenSocket_);
            listenSocket_ = k_HSteamListenSocket_Invalid;
            events_.push_back({
                .type = ConnectionEventType::ListenSocketFailure,
                .connection = kInvalidConnectionHandle,
                .reasonCode = -2,
                .reason = "CreatePollGroup failed",
                .remoteAddress = {},
            });
            return false;
        }

        return true;
    }

    ConnectionHandle Connect(const std::string& host, uint16_t port, std::string& error) {
        if (!initialized_) {
            error = "transport not initialized";
            return kInvalidConnectionHandle;
        }

        SteamNetworkingIPAddr remoteAddress;
        remoteAddress.Clear();
        if (!remoteAddress.ParseString(host.c_str())) {
            error = "invalid remote host address: " + host;
            return kInvalidConnectionHandle;
        }
        remoteAddress.m_port = port;

        const HSteamNetConnection connection = sockets_->ConnectByIPAddress(remoteAddress, 0, nullptr);
        if (connection == k_HSteamNetConnection_Invalid) {
            error = "ConnectByIPAddress failed";
            return kInvalidConnectionHandle;
        }

        const ConnectionHandle handle = ToConnectionHandle(connection);
        connections_.emplace(handle, connection);
        return handle;
    }

    void Poll() {
        if (!initialized_) {
            return;
        }

        sockets_->RunCallbacks();

        if (config_.isServer && pollGroup_ != k_HSteamNetPollGroup_Invalid) {
            ReceiveFromPollGroup();
            return;
        }

        ReceiveFromConnections();
    }

    bool Send(ConnectionHandle connectionHandle, std::span<const uint8_t> payload, const SendOptions& options,
              std::string& error) {
        if (!initialized_) {
            error = "transport not initialized";
            return false;
        }

        const auto it = connections_.find(connectionHandle);
        if (it == connections_.end()) {
            error = "unknown connection handle";
            return false;
        }

        if (payload.empty()) {
            return true;
        }

        int flags = 0;
        if (options.reliability == Reliability::ReliableOrdered) {
            flags |= k_nSteamNetworkingSend_Reliable;
        } else {
            flags |= k_nSteamNetworkingSend_Unreliable;
        }
        if (options.noNagle) {
            flags |= k_nSteamNetworkingSend_NoNagle;
        }

        SteamNetworkingMessage_t* message = utils_->AllocateMessage(static_cast<int>(payload.size()));
        if (message == nullptr) {
            error = "AllocateMessage failed";
            return false;
        }

        std::memcpy(message->m_pData, payload.data(), payload.size());
        message->m_conn = it->second;
        message->m_nFlags = flags;
        message->m_cbSize = static_cast<int>(payload.size());
        message->m_idxLane = static_cast<uint16>(options.lane);

        SteamNetworkingMessage_t* messages[1] = {message};
        int64 result = 0;
        sockets_->SendMessages(1, messages, &result);
        if (result < 0) {
            error = "SendMessages failed with result " + std::to_string(result);
            return false;
        }

        return true;
    }

    bool Close(ConnectionHandle connectionHandle, int32_t reasonCode, const std::string& reason, bool linger) {
        const auto it = connections_.find(connectionHandle);
        if (it == connections_.end()) {
            return false;
        }

        const bool closed = sockets_->CloseConnection(it->second, reasonCode, reason.c_str(), linger);
        connections_.erase(it);
        connectionUserData_.erase(connectionHandle);
        return closed;
    }

    [[nodiscard]] std::vector<ConnectionEvent> DrainConnectionEvents() {
        std::vector<ConnectionEvent> out;
        out.reserve(events_.size());
        while (!events_.empty()) {
            out.push_back(std::move(events_.front()));
            events_.pop_front();
        }
        return out;
    }

    [[nodiscard]] std::vector<ReceivedPacket> DrainReceivedPackets() {
        std::vector<ReceivedPacket> out;
        out.reserve(packets_.size());
        while (!packets_.empty()) {
            out.push_back(std::move(packets_.front()));
            packets_.pop_front();
        }
        return out;
    }

    bool SetConnectionUserData(ConnectionHandle connectionHandle, uint64_t userData) {
        const auto it = connections_.find(connectionHandle);
        if (it == connections_.end()) {
            return false;
        }

        sockets_->SetConnectionUserData(it->second, static_cast<int64>(userData));
        connectionUserData_[connectionHandle] = userData;
        return true;
    }

    [[nodiscard]] uint64_t GetConnectionUserData(ConnectionHandle connectionHandle) const {
        const auto it = connections_.find(connectionHandle);
        if (it == connections_.end()) {
            return 0;
        }

        auto userDataIt = connectionUserData_.find(connectionHandle);
        if (userDataIt != connectionUserData_.end()) {
            return userDataIt->second;
        }

        return static_cast<uint64_t>(sockets_->GetConnectionUserData(it->second));
    }

    [[nodiscard]] std::optional<ConnectionMetrics> GetConnectionMetrics(ConnectionHandle connectionHandle) const {
        const auto it = connections_.find(connectionHandle);
        if (it == connections_.end()) {
            return std::nullopt;
        }

        SteamNetConnectionRealTimeStatus_t status{};
        const EResult result = sockets_->GetConnectionRealTimeStatus(it->second, &status, 0, nullptr);
        if (result != k_EResultOK) {
            return std::nullopt;
        }

        ConnectionMetrics metrics;
        metrics.pingMs = status.m_nPing;
        metrics.pendingReliableBytes = status.m_cbPendingReliable;
        metrics.pendingUnreliableBytes = status.m_cbPendingUnreliable;
        metrics.queuedUsec = static_cast<int>(status.m_usecQueueTime);
        metrics.jitterUsec = status.m_usecMaxJitter;
        metrics.outBytesPerSec = status.m_flOutBytesPerSec;
        metrics.inBytesPerSec = status.m_flInBytesPerSec;
        metrics.qualityLocal = status.m_flConnectionQualityLocal;
        metrics.qualityRemote = status.m_flConnectionQualityRemote;
        return metrics;
    }

    bool ApplyConnectionNetworkSimulation(ConnectionHandle connectionHandle, const NetSimConfig& simulation) {
        const auto it = connections_.find(connectionHandle);
        if (it == connections_.end()) {
            return false;
        }

        bool success = true;
        success &= utils_->SetConnectionConfigValueFloat(
            it->second, k_ESteamNetworkingConfig_FakePacketLag_Send, simulation.fakeLagMs);
        success &= utils_->SetConnectionConfigValueFloat(
            it->second, k_ESteamNetworkingConfig_FakePacketLag_Recv, simulation.fakeLagMs);
        success &= utils_->SetConnectionConfigValueFloat(
            it->second, k_ESteamNetworkingConfig_FakePacketLoss_Send, simulation.fakeLossSendPct);
        success &= utils_->SetConnectionConfigValueFloat(
            it->second, k_ESteamNetworkingConfig_FakePacketLoss_Recv, simulation.fakeLossRecvPct);

        success &= utils_->SetConnectionConfigValueFloat(
            it->second, k_ESteamNetworkingConfig_FakePacketJitter_Send_Avg, simulation.fakeJitterMs);
        success &= utils_->SetConnectionConfigValueFloat(
            it->second, k_ESteamNetworkingConfig_FakePacketJitter_Recv_Avg, simulation.fakeJitterMs);

        const float jitterMax = simulation.fakeJitterMs * 2.0f;
        success &=
            utils_->SetConnectionConfigValueFloat(it->second, k_ESteamNetworkingConfig_FakePacketJitter_Send_Max, jitterMax);
        success &=
            utils_->SetConnectionConfigValueFloat(it->second, k_ESteamNetworkingConfig_FakePacketJitter_Recv_Max, jitterMax);

        success &= utils_->SetConnectionConfigValueFloat(it->second, k_ESteamNetworkingConfig_FakePacketJitter_Send_Pct, 100.0f);
        success &= utils_->SetConnectionConfigValueFloat(it->second, k_ESteamNetworkingConfig_FakePacketJitter_Recv_Pct, 100.0f);

        return success;
    }

private:
    void ReceiveFromPollGroup() {
        SteamNetworkingMessage_t* incomingMessages[128];

        while (true) {
            const int count = sockets_->ReceiveMessagesOnPollGroup(pollGroup_, incomingMessages, 128);
            if (count <= 0) {
                break;
            }

            for (int index = 0; index < count; ++index) {
                HandleIncomingMessage(*incomingMessages[index]);
                incomingMessages[index]->Release();
            }
        }
    }

    void ReceiveFromConnections() {
        SteamNetworkingMessage_t* incomingMessages[128];

        for (const auto& [connectionHandle, connection] : connections_) {
            (void)connectionHandle;
            while (true) {
                const int count = sockets_->ReceiveMessagesOnConnection(connection, incomingMessages, 128);
                if (count <= 0) {
                    break;
                }

                for (int index = 0; index < count; ++index) {
                    HandleIncomingMessage(*incomingMessages[index]);
                    incomingMessages[index]->Release();
                }
            }
        }
    }

    void HandleIncomingMessage(const SteamNetworkingMessage_t& message) {
        ReceivedPacket packet;
        packet.connection = ToConnectionHandle(message.m_conn);

        const int laneIndex =
            std::clamp(static_cast<int>(message.m_idxLane), 0, static_cast<int>(kLaneCount - 1));
        packet.lane = static_cast<Lane>(laneIndex);
        packet.reliable = (message.m_nFlags & k_nSteamNetworkingSend_Reliable) != 0;

        packet.bytes.resize(static_cast<size_t>(message.m_cbSize));
        if (!packet.bytes.empty()) {
            std::memcpy(packet.bytes.data(), message.m_pData, packet.bytes.size());
        }

        packets_.push_back(std::move(packet));
    }

    static void OnConnectionStatusChangedStatic(SteamNetConnectionStatusChangedCallback_t* info) {
        if (callbackOwner_ != nullptr) {
            callbackOwner_->OnConnectionStatusChanged(*info);
        }
    }

    void OnConnectionStatusChanged(const SteamNetConnectionStatusChangedCallback_t& info) {
        const ConnectionHandle connectionHandle = ToConnectionHandle(info.m_hConn);

        char addressBuffer[SteamNetworkingIPAddr::k_cchMaxString];
        info.m_info.m_addrRemote.ToString(addressBuffer, sizeof(addressBuffer), true);

        ConnectionEvent event;
        event.type = ToConnectionEventType(info.m_info.m_eState);
        event.connection = connectionHandle;
        event.reasonCode = info.m_info.m_eEndReason;
        event.reason = info.m_info.m_szEndDebug;
        event.remoteAddress = addressBuffer;

        switch (info.m_info.m_eState) {
        case k_ESteamNetworkingConnectionState_Connecting:
            connections_[connectionHandle] = info.m_hConn;
            if (config_.isServer) {
                const EResult accepted = sockets_->AcceptConnection(info.m_hConn);
                if (accepted != k_EResultOK) {
                    sockets_->CloseConnection(info.m_hConn, 4001, "accept failed", false);
                    event.type = ConnectionEventType::ProblemDetectedLocally;
                    event.reason = "AcceptConnection failed";
                    events_.push_back(std::move(event));
                    return;
                }

                if (pollGroup_ != k_HSteamNetPollGroup_Invalid) {
                    sockets_->SetConnectionPollGroup(info.m_hConn, pollGroup_);
                }
            }
            events_.push_back(std::move(event));
            break;

        case k_ESteamNetworkingConnectionState_Connected:
            connections_[connectionHandle] = info.m_hConn;
            ConfigureConnectionLanes(info.m_hConn);
            events_.push_back(std::move(event));
            break;

        case k_ESteamNetworkingConnectionState_ClosedByPeer:
        case k_ESteamNetworkingConnectionState_ProblemDetectedLocally:
            events_.push_back(std::move(event));
            sockets_->CloseConnection(info.m_hConn, 0, nullptr, false);
            connections_.erase(connectionHandle);
            connectionUserData_.erase(connectionHandle);
            break;

        default:
            break;
        }
    }

    void ConfigureConnectionLanes(HSteamNetConnection connection) {
        std::array<int, kLaneCount> lanePriorities{};
        std::array<uint16, kLaneCount> laneWeights{};

        for (const LanePolicy& policy : DefaultLanePolicies()) {
            const size_t index = static_cast<size_t>(policy.lane);
            lanePriorities[index] = policy.priority;
            laneWeights[index] = policy.weight;
        }

        sockets_->ConfigureConnectionLanes(connection, static_cast<int>(kLaneCount), lanePriorities.data(),
                                           laneWeights.data());
    }

    static inline Impl* callbackOwner_ = nullptr;

    TransportConfig config_{};
    ISteamNetworkingSockets* sockets_ = nullptr;
    ISteamNetworkingUtils* utils_ = nullptr;
    bool initialized_ = false;

    HSteamListenSocket listenSocket_ = k_HSteamListenSocket_Invalid;
    HSteamNetPollGroup pollGroup_ = k_HSteamNetPollGroup_Invalid;

    std::unordered_map<ConnectionHandle, HSteamNetConnection> connections_;
    std::unordered_map<ConnectionHandle, uint64_t> connectionUserData_;
    std::deque<ConnectionEvent> events_;
    std::deque<ReceivedPacket> packets_;
};

TransportGns::TransportGns() : impl_(std::make_unique<Impl>()) {}
TransportGns::~TransportGns() { impl_->Shutdown(); }

bool TransportGns::Initialize(const TransportConfig& config, std::string& error) {
    return impl_->Initialize(config, error);
}

void TransportGns::Shutdown() { impl_->Shutdown(); }

bool TransportGns::IsInitialized() const { return impl_->IsInitialized(); }

bool TransportGns::StartServer(uint16_t listenPort, std::string& error) {
    return impl_->StartServer(listenPort, error);
}

ConnectionHandle TransportGns::Connect(const std::string& host, uint16_t port, std::string& error) {
    return impl_->Connect(host, port, error);
}

void TransportGns::Poll() { impl_->Poll(); }

bool TransportGns::Send(ConnectionHandle connection, std::span<const uint8_t> payload, const SendOptions& options,
                        std::string& error) {
    return impl_->Send(connection, payload, options, error);
}

bool TransportGns::Close(ConnectionHandle connection, int32_t reasonCode, const std::string& reason, bool linger) {
    return impl_->Close(connection, reasonCode, reason, linger);
}

std::vector<ConnectionEvent> TransportGns::DrainConnectionEvents() { return impl_->DrainConnectionEvents(); }

std::vector<ReceivedPacket> TransportGns::DrainReceivedPackets() { return impl_->DrainReceivedPackets(); }

bool TransportGns::SetConnectionUserData(ConnectionHandle connection, uint64_t userData) {
    return impl_->SetConnectionUserData(connection, userData);
}

uint64_t TransportGns::GetConnectionUserData(ConnectionHandle connection) const {
    return impl_->GetConnectionUserData(connection);
}

std::optional<ConnectionMetrics> TransportGns::GetConnectionMetrics(ConnectionHandle connection) const {
    return impl_->GetConnectionMetrics(connection);
}

bool TransportGns::ApplyConnectionNetworkSimulation(ConnectionHandle connection, const NetSimConfig& simulation) {
    return impl_->ApplyConnectionNetworkSimulation(connection, simulation);
}

}  // namespace shared::net
