#include <cassert>
#include <chrono>
#include <filesystem>

#include "client/core/client_config_policy.hpp"
#include "client/runtime/client_runtime_policy.hpp"
#include "server/config/server_config_policy.hpp"
#include "server/runtime/server_runtime_policy.hpp"
#include "shared/game/game_policy.hpp"

int main() {
    namespace fs = std::filesystem;

    assert(client::core::policy::kDefaultServerHost == "127.0.0.1");
    assert(client::core::policy::kDefaultServerPort == 27020);
    assert(client::core::policy::DefaultClientConfigPath() == (fs::path{"client_data"} / "client.cfg"));
    assert(client::runtime::policy::kSplashDuration == std::chrono::milliseconds{1400});
    assert(client::runtime::policy::kLocalServerConnectRetryInterval == std::chrono::milliseconds{250});
    assert(client::runtime::policy::kLocalServerStartupTimeout == std::chrono::seconds{10});
    assert(shared::game::policy::world::kDefaultChunkWidthTiles == 64);
    assert(shared::game::policy::world::kDefaultChunkHeightTiles == 64);
    assert(shared::game::policy::player::kDefaultGravity == -24.0f);
    assert(shared::game::policy::fixed_step::kDefaultMaxCatchupSteps == 8);
    assert(shared::game::policy::validation::kDefaultMaxSequenceBacktrack == 2048U);
    assert(server::config::policy::kDefaultListenPort == 27020);
    assert(server::config::policy::kDefaultSnapshotRateHz == 15);
    assert(server::runtime::policy::kMalformedInputThreshold == 5);
    assert(server::runtime::policy::kRateWindow == std::chrono::seconds{1});
    assert(server::runtime::policy::kExpandedMaxRequestedInterestRadius >=
        server::runtime::policy::kDefaultMaxRequestedInterestRadius);

    return 0;
}
