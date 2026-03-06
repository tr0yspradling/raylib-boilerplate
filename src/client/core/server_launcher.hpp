#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace client::core {

struct LocalServerLaunchRequest {
    std::string clientExecutablePath;
    uint16_t serverPort = 27020;
    int simulationTickHz = 30;
    int snapshotRateHz = 15;
};

struct ServerLaunchCommand {
    std::filesystem::path executablePath;
    std::vector<std::string> arguments;
};

enum class ServerProcessState : uint8_t {
    NotRunning = 0,
    Running,
    Exited,
};

struct ServerProcessStatus {
    ServerProcessState state = ServerProcessState::NotRunning;
    std::optional<int> exitCode;
};

[[nodiscard]] std::filesystem::path ResolveSiblingServerExecutable(const std::filesystem::path& clientExecutablePath);
[[nodiscard]] bool BuildLocalServerLaunchCommand(const LocalServerLaunchRequest& request,
                                                 ServerLaunchCommand& command, std::string& error);

class IServerLauncher {
public:
    virtual ~IServerLauncher() = default;

    [[nodiscard]] virtual bool Launch(const LocalServerLaunchRequest& request, std::string& error) = 0;
    [[nodiscard]] virtual ServerProcessStatus PollStatus() = 0;
    virtual void Stop() = 0;
};

}  // namespace client::core
