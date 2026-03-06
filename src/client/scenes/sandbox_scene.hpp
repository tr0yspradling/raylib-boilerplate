#pragma once

#include <string_view>

namespace client::scenes {

[[nodiscard]] inline std::string_view GameplayCaption() {
    return "Authoritative movement + prediction/reconciliation";
}

}  // namespace client::scenes
