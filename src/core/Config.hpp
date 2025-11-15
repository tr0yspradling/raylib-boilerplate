#pragma once

#include <string>

#include <raylib-cpp.hpp>

// Shared application configuration that controls window setup and the default
// look of the boilerplate sample. Extend this struct instead of sprinkling
// literals throughout the codebase.
struct AppConfig {
    int width = 1600;
    int height = 900;
    int targetFPS = 120;
    std::string title = "raylib boilerplate";
    unsigned int windowFlags = FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT;
    raylib::Color clearColor{20, 24, 28, 255};
};
