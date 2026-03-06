#pragma once

#include <memory>

#include "shared/net/transport.hpp"

namespace shared::net {

class TransportGns final : public ITransport {
public:
    TransportGns();
    ~TransportGns() override;

    TransportGns(const TransportGns&) = delete;
    TransportGns& operator=(const TransportGns&) = delete;

    bool Initialize(const TransportConfig& config, std::string& error) override;
    void Shutdown() override;

    [[nodiscard]] bool IsInitialized() const override;

    bool StartServer(uint16_t listenPort, std::string& error) override;
    ConnectionHandle Connect(const std::string& host, uint16_t port, std::string& error) override;

    void Poll() override;

    bool Send(ConnectionHandle connection, std::span<const uint8_t> payload, const SendOptions& options,
              std::string& error) override;

    bool Close(ConnectionHandle connection, int32_t reasonCode, const std::string& reason, bool linger) override;

    [[nodiscard]] std::vector<ConnectionEvent> DrainConnectionEvents() override;
    [[nodiscard]] std::vector<ReceivedPacket> DrainReceivedPackets() override;

    bool SetConnectionUserData(ConnectionHandle connection, uint64_t userData) override;
    [[nodiscard]] uint64_t GetConnectionUserData(ConnectionHandle connection) const override;

    [[nodiscard]] std::optional<ConnectionMetrics> GetConnectionMetrics(ConnectionHandle connection) const override;

    bool ApplyConnectionNetworkSimulation(ConnectionHandle connection, const NetSimConfig& simulation) override;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace shared::net
