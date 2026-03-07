#pragma once

#include <filesystem>
#include <string>

#include "client/core/client_config.hpp"

namespace client::core {

[[nodiscard]] std::filesystem::path DefaultClientConfigPath();
[[nodiscard]] client::ClientConfig LoadClientConfigFile(const std::filesystem::path& path, std::string& warning);
[[nodiscard]] bool SaveClientConfigFile(const client::ClientConfig& config, const std::filesystem::path& path,
                                        std::string& error);

}  // namespace client::core
