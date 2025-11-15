#pragma once

#include <string>
#include <utility>

#include <raylib-cpp.hpp>

#include "Config.hpp"
#include "SceneManager.hpp"
#include "../ui/DebugOverlay.hpp"

class Application {
public:
    explicit Application(AppConfig config)
        : config_(std::move(config)),
          window_(config_.width, config_.height, config_.title, config_.windowFlags),
          sceneManager_(window_, input_) {
        window_.SetTargetFPS(config_.targetFPS);
    }

    template <typename TScene, typename... TArgs>
    TScene& RegisterScene(const std::string& id, TArgs&&... args) {
        return sceneManager_.RegisterScene<TScene>(id, std::forward<TArgs>(args)...);
    }

    void Run(const std::string& initialScene) {
        sceneManager_.SwitchTo(initialScene);

        while (!raylib::Window::ShouldClose()) {
            input_.Update();
            const float dt = raylib::Window::GetFrameTime();

            sceneManager_.Update(dt);

            window_.BeginDrawing();
            window_.ClearBackground(config_.clearColor);
            sceneManager_.Draw();
            ui::DebugOverlay::Draw(sceneManager_, dt);
            window_.EndDrawing();
        }
    }

    [[nodiscard]] SceneManager& Scenes() { return sceneManager_; }

private:
    AppConfig config_;
    raylib::Window window_;
    InputManager input_;
    SceneManager sceneManager_;
};
