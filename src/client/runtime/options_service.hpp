#pragma once

#include <functional>
#include <string>

#include "client/core/client_config.hpp"
#include "client/runtime/runtime_resources.hpp"
#include "client/ui/ui_state.hpp"

namespace client::runtime {

struct OptionsApplyResult {
    bool success = false;
    std::string statusMessage;
};

using ApplyWindowSettingsFn = std::function<void(int width, int height, int targetFps)>;

class OptionsService {
    public:
        [[nodiscard]] OptionsApplyResult Apply(const ui::OptionsScreenState& optionsScreenState, ClientConfig& config,
                                               ui::JoinServerScreenState& joinScreenState, bool& debugOverlayEnabled,
                                               ApplyWindowSettingsFn applyWindowSettings = {}) const;
};

}  // namespace client::runtime
