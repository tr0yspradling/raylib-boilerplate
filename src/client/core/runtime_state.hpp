#pragma once

#include <cstdint>
#include <string>

namespace client::core {

enum class RuntimeMode : uint8_t {
    Boot,
    Menu,
    JoiningServer,
    StartingLocalServer,
    Multiplayer,
    Singleplayer,
    Options,
    Disconnected,
};

struct RuntimeState {
    RuntimeMode mode = RuntimeMode::Boot;
    bool splashCompleted = false;
    bool requestedJoin = false;
    bool joiningInProgress = false;
    bool requestedLocalServerStart = false;
    bool requestedSingleplayer = false;
    bool requestedOptions = false;
    std::string disconnectReason;
};

}  // namespace client::core
