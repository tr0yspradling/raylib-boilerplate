#pragma once

#include <string_view>

namespace client::scenes {

[[nodiscard]] inline std::string_view ConnectingCaption() {
    return "Connecting to dedicated server";
}

}  // namespace client::scenes
