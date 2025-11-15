#include <exception>

#include <raylib-cpp.hpp>

#include "core/Application.hpp"
#include "core/Config.hpp"
#include "scenes/MenuScene.hpp"
#include "scenes/SandboxScene.hpp"

auto main() -> int {
    try {
        AppConfig config{};
        Application app{config};
        app.RegisterScene<MenuScene>("menu");
        app.RegisterScene<SandboxScene>("sandbox");
        app.Run("menu");
        return 0;
    } catch (const std::exception& e) {
        TraceLog(LOG_ERROR, "Fatal error: %s", e.what());
        return 1;
    }
}
