#include <exception>
#include <raylib-cpp.hpp>

#include "core/Application.hpp"
#include "core/Config.hpp"
#include "scenes/MenuScene.hpp"
#include "scenes/SandboxScene.hpp"
#include "scenes/SplashScene.hpp"

auto main() -> int {
    try {
        AppConfig config{};
        Application app{config};
        app.RegisterScene<SplashScene>("splash");
        app.RegisterScene<MenuScene>("menu");
        app.RegisterScene<SandboxScene>("sandbox");
        app.Run("splash");
        return 0;
    } catch (const std::exception& e) {
        TraceLog(LOG_ERROR, "Fatal error: %s", e.what());
        return 1;
    }
}
