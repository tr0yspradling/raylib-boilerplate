#include "client/core/server_launcher_process.hpp"

#include <cerrno>
#include <chrono>
#include <cstring>
#include <filesystem>
#include <string>
#include <system_error>
#include <thread>
#include <vector>

#if defined(_WIN32)
#include <windows.h>
#else
#include <csignal>
#include <spawn.h>
#include <sys/wait.h>
#include <unistd.h>

extern char** environ;
#endif

namespace client::core {

namespace {

#if defined(_WIN32)
constexpr char kServerExecutableName[] = "game_server.exe";
#else
constexpr char kServerExecutableName[] = "game_server";
#endif

[[nodiscard]] std::filesystem::path NormalizeExecutablePath(const std::filesystem::path& executablePath) {
    if (executablePath.empty()) {
        return {};
    }

    std::error_code error;
    const std::filesystem::path absolutePath = executablePath.is_absolute()
        ? executablePath
        : std::filesystem::absolute(executablePath, error);
    if (error) {
        return executablePath.lexically_normal();
    }

    return absolutePath.lexically_normal();
}

#if defined(_WIN32)
[[nodiscard]] std::string QuoteWindowsArg(const std::string& value) {
    if (value.find_first_of(" \t\"") == std::string::npos) {
        return value;
    }

    std::string quoted;
    quoted.reserve(value.size() + 2);
    quoted.push_back('"');
    for (const char ch : value) {
        if (ch == '"') {
            quoted += "\\\"";
        } else {
            quoted.push_back(ch);
        }
    }
    quoted.push_back('"');
    return quoted;
}

[[nodiscard]] std::string BuildWindowsCommandLine(const ServerLaunchCommand& command) {
    std::string commandLine = QuoteWindowsArg(command.executablePath.string());
    for (const std::string& argument : command.arguments) {
        commandLine.push_back(' ');
        commandLine += QuoteWindowsArg(argument);
    }
    return commandLine;
}
#else
[[nodiscard]] int DecodeExitStatus(int rawStatus) {
    if (WIFEXITED(rawStatus)) {
        return WEXITSTATUS(rawStatus);
    }
    if (WIFSIGNALED(rawStatus)) {
        return 128 + WTERMSIG(rawStatus);
    }
    return -1;
}
#endif

}  // namespace

std::filesystem::path ResolveSiblingServerExecutable(const std::filesystem::path& clientExecutablePath) {
    std::filesystem::path serverExecutable = NormalizeExecutablePath(clientExecutablePath);
    if (serverExecutable.empty()) {
        return {};
    }

    serverExecutable.replace_filename(kServerExecutableName);
    return serverExecutable.lexically_normal();
}

bool BuildLocalServerLaunchCommand(const LocalServerLaunchRequest& request, ServerLaunchCommand& command, std::string& error) {
    command = {};

    if (request.clientExecutablePath.empty()) {
        error = "client executable path is empty";
        return false;
    }

    const std::filesystem::path executablePath = ResolveSiblingServerExecutable(request.clientExecutablePath);
    if (executablePath.empty()) {
        error = "unable to resolve sibling game_server path";
        return false;
    }

    std::error_code existsError;
    if (!std::filesystem::exists(executablePath, existsError) || existsError) {
        error = "game_server executable not found at " + executablePath.string();
        return false;
    }

    command.executablePath = executablePath;
    command.arguments = {
        "--port",
        std::to_string(request.serverPort),
        "--tick-rate",
        std::to_string(request.simulationTickHz),
        "--snapshot-rate",
        std::to_string(request.snapshotRateHz),
    };
    error.clear();
    return true;
}

ServerLauncherProcess::~ServerLauncherProcess() {
    Stop();
}

bool ServerLauncherProcess::Launch(const LocalServerLaunchRequest& request, std::string& error) {
    Stop();

    ServerLaunchCommand command;
    if (!BuildLocalServerLaunchCommand(request, command, error)) {
        return false;
    }

#if defined(_WIN32)
    STARTUPINFOA startupInfo{};
    startupInfo.cb = sizeof(startupInfo);

    PROCESS_INFORMATION processInfo{};
    std::string commandLine = BuildWindowsCommandLine(command);
    if (!CreateProcessA(command.executablePath.string().c_str(), commandLine.data(), nullptr, nullptr, FALSE, 0, nullptr,
                        nullptr, &startupInfo, &processInfo)) {
        error = "CreateProcess failed with error " + std::to_string(static_cast<int>(GetLastError()));
        return false;
    }

    processHandle_ = processInfo.hProcess;
    threadHandle_ = processInfo.hThread;
    exitCode_.reset();
#else
    std::vector<std::string> argvStorage;
    argvStorage.reserve(command.arguments.size() + 1);
    argvStorage.push_back(command.executablePath.string());
    argvStorage.insert(argvStorage.end(), command.arguments.begin(), command.arguments.end());

    std::vector<char*> argvPointers;
    argvPointers.reserve(argvStorage.size() + 1);
    for (std::string& argument : argvStorage) {
        argvPointers.push_back(argument.data());
    }
    argvPointers.push_back(nullptr);

    pid_t childPid = -1;
    const int spawnResult = posix_spawn(&childPid, command.executablePath.c_str(), nullptr, nullptr, argvPointers.data(),
                                        environ);
    if (spawnResult != 0) {
        error = "posix_spawn failed: " + std::string{std::strerror(spawnResult)};
        return false;
    }

    processId_ = static_cast<int>(childPid);
    exitCode_.reset();
#endif

    error.clear();
    return true;
}

ServerProcessStatus ServerLauncherProcess::PollStatus() {
#if defined(_WIN32)
    if (processHandle_ == nullptr) {
        return {.state = exitCode_.has_value() ? ServerProcessState::Exited : ServerProcessState::NotRunning,
                .exitCode = exitCode_};
    }

    const DWORD waitResult = WaitForSingleObject(static_cast<HANDLE>(processHandle_), 0);
    if (waitResult == WAIT_TIMEOUT) {
        return {.state = ServerProcessState::Running, .exitCode = std::nullopt};
    }

    DWORD exitCode = STILL_ACTIVE;
    GetExitCodeProcess(static_cast<HANDLE>(processHandle_), &exitCode);
    exitCode_ = static_cast<int>(exitCode);
    CloseHandle(static_cast<HANDLE>(processHandle_));
    processHandle_ = nullptr;
    if (threadHandle_ != nullptr) {
        CloseHandle(static_cast<HANDLE>(threadHandle_));
        threadHandle_ = nullptr;
    }
    return {.state = ServerProcessState::Exited, .exitCode = exitCode_};
#else
    if (processId_ < 0) {
        return {.state = exitCode_.has_value() ? ServerProcessState::Exited : ServerProcessState::NotRunning,
                .exitCode = exitCode_};
    }

    int status = 0;
    const pid_t result = waitpid(static_cast<pid_t>(processId_), &status, WNOHANG);
    if (result == 0) {
        return {.state = ServerProcessState::Running, .exitCode = std::nullopt};
    }
    if (result == static_cast<pid_t>(processId_)) {
        exitCode_ = DecodeExitStatus(status);
        processId_ = -1;
        return {.state = ServerProcessState::Exited, .exitCode = exitCode_};
    }

    if (errno == ECHILD) {
        processId_ = -1;
        exitCode_ = -1;
        return {.state = ServerProcessState::Exited, .exitCode = exitCode_};
    }

    return {.state = ServerProcessState::Running, .exitCode = std::nullopt};
#endif
}

void ServerLauncherProcess::Stop() {
#if defined(_WIN32)
    if (processHandle_ == nullptr) {
        if (threadHandle_ != nullptr) {
            CloseHandle(static_cast<HANDLE>(threadHandle_));
            threadHandle_ = nullptr;
        }
        return;
    }

    const ServerProcessStatus status = PollStatus();
    if (status.state == ServerProcessState::Running) {
        TerminateProcess(static_cast<HANDLE>(processHandle_), 1);
        WaitForSingleObject(static_cast<HANDLE>(processHandle_), 2000);
        DWORD exitCode = 1;
        GetExitCodeProcess(static_cast<HANDLE>(processHandle_), &exitCode);
        exitCode_ = static_cast<int>(exitCode);
    }

    if (processHandle_ != nullptr) {
        CloseHandle(static_cast<HANDLE>(processHandle_));
        processHandle_ = nullptr;
    }
    if (threadHandle_ != nullptr) {
        CloseHandle(static_cast<HANDLE>(threadHandle_));
        threadHandle_ = nullptr;
    }
#else
    if (processId_ < 0) {
        return;
    }

    const ServerProcessStatus status = PollStatus();
    if (status.state == ServerProcessState::Running) {
        kill(static_cast<pid_t>(processId_), SIGTERM);

        for (int attempt = 0; attempt < 100; ++attempt) {
            std::this_thread::sleep_for(std::chrono::milliseconds{20});
            const ServerProcessStatus currentStatus = PollStatus();
            if (currentStatus.state == ServerProcessState::Exited) {
                return;
            }
        }

        kill(static_cast<pid_t>(processId_), SIGKILL);
        int rawStatus = 0;
        waitpid(static_cast<pid_t>(processId_), &rawStatus, 0);
        exitCode_ = DecodeExitStatus(rawStatus);
        processId_ = -1;
    }
#endif
}

}  // namespace client::core
