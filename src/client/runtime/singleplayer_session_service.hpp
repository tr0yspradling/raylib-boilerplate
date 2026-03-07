#pragma once

#include <string_view>

#include "client/core/singleplayer_runtime.hpp"
#include "client/runtime/runtime_resources.hpp"

namespace client::runtime {

class SingleplayerSessionService {
    public:
        void Start(std::string_view playerName, ClientSessionState& session);
        void Stop(ClientSessionState& session);

        [[nodiscard]] bool IsActive() const;
        void Step(const game::PlayerInputFrame& inputFrame, float fixedDeltaSeconds, ClientSessionState& session);

    private:
        void PublishSessionState(ClientSessionState& session) const;

        core::SingleplayerRuntime runtime_{};
};

}  // namespace client::runtime
