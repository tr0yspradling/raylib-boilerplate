#include <cassert>
#include <chrono>

#include "shared/net/net_policy.hpp"

int main() {
    using namespace shared::net;

    assert(kProtocolMagic == 0x52424e54U);
    assert(kProtocolVersion == 2U);
    assert(policy::protocol_limits::kMaxPlayerNameLength == 32U);
    assert(policy::protocol_limits::kMaxAuthTokenLength == 2048U);
    assert(policy::chunk_interest::kDefaultMaxRequestedRadius == 16U);
    assert(policy::chunk_interest::kExpandedMaxRequestedRadius == 24U);
    assert(policy::ToInt(policy::DisconnectCode::ClientProtocolVersionMismatch) == 4002);
    assert(policy::ToInt(policy::DisconnectCode::MalformedChunkHint) == 4305);
    assert(policy::ToInt(policy::ResyncReasonCode::InvalidChunkInterestHint) == 5101);
    assert(policy::client::kPingInterval == std::chrono::milliseconds{1000});
    assert(policy::client::kChunkHintInterval == std::chrono::milliseconds{500});
    assert(policy::client::kChunkResyncCooldown == std::chrono::milliseconds{250});

    return 0;
}
