#pragma once

#include <optional>

#include "client/core/server_launcher.hpp"

namespace client::core {

class ServerLauncherProcess final : public IServerLauncher {
public:
    ServerLauncherProcess() = default;
    ~ServerLauncherProcess() override;

    [[nodiscard]] bool Launch(const LocalServerLaunchRequest& request, std::string& error) override;
    [[nodiscard]] ServerProcessStatus PollStatus() override;
    void Stop() override;

private:
#if defined(_WIN32)
    void* processHandle_ = nullptr;
    void* threadHandle_ = nullptr;
#else
    int processId_ = -1;
#endif
    std::optional<int> exitCode_;
};

}  // namespace client::core
