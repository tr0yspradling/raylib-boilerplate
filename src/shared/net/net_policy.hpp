#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace shared::net {

inline constexpr uint32_t kProtocolMagic = 0x52424e54;  // RBNT
inline constexpr uint16_t kProtocolVersion = 2;

namespace policy {

inline constexpr int kTransportDebugVerbosity = 4;
inline constexpr bool kAllowUnencryptedDevTransport = true;
inline constexpr std::string_view kDevAuthToken = "dev";

namespace protocol_limits {
inline constexpr std::size_t kMaxPlayerNameLength = 32U;
inline constexpr std::size_t kMaxAuthTokenLength = 2048U;
inline constexpr std::size_t kMaxSpawnDisplayNameLength = 64U;
inline constexpr uint16_t kMinWorldMetadataDimension = 1;
inline constexpr uint16_t kMaxWorldMetadataDimension = 65535;
}  // namespace protocol_limits

namespace chunk_interest {
inline constexpr uint16_t kMinRequestedRadius = 1;
inline constexpr uint16_t kDefaultMaxRequestedRadius = 16;
inline constexpr uint16_t kExpandedMaxRequestedRadius = 24;
}  // namespace chunk_interest

enum class DisconnectCode : int32_t {
    ClientShutdown = 0,
    ClientHelloSendFailed = 4001,
    ClientProtocolVersionMismatch = 4002,
    ClientWelcomeProtocolMismatch = 4003,
    ClientInvalidServerKinematics = 4004,
    ServerFull = 4100,
    InvalidClientHello = 4201,
    ProtocolMismatch = 4202,
    BuildHashMismatch = 4203,
    AuthRejected = 4204,
    SessionNotAuthenticated = 4301,
    MalformedInputFrame = 4302,
    InputFloodDetected = 4303,
    InvalidInputFrame = 4304,
    MalformedChunkHint = 4305,
    MalformedChunkResyncRequest = 4306,
    ChunkHintFloodDetected = 4307,
    ChunkResyncFloodDetected = 4308,
};

enum class ResyncReasonCode : int32_t {
    InvalidChunkInterestHint = 5101,
    ChunkOutsideInterestSet = 5102,
    ChunkUnavailable = 5103,
};

[[nodiscard]] constexpr int32_t ToInt(DisconnectCode code) {
    return static_cast<int32_t>(code);
}

[[nodiscard]] constexpr int32_t ToInt(ResyncReasonCode code) {
    return static_cast<int32_t>(code);
}

namespace client {
inline constexpr auto kPingInterval = std::chrono::milliseconds{1000};
inline constexpr auto kChunkHintInterval = std::chrono::milliseconds{500};
inline constexpr auto kChunkResyncCooldown = std::chrono::milliseconds{250};
}  // namespace client

}  // namespace policy

}  // namespace shared::net
