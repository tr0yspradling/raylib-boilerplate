#pragma once

#include <raylib-cpp.hpp>
#include <string>
#include <utility>

#include "ui/DebugOverlay.hpp"
#include "Config.hpp"
#include "SceneManager.hpp"

class Application {
public:
    explicit Application(AppConfig config) :
        config_(std::move(config)), window_(config_.width, config_.height, config_.title, config_.windowFlags),
        sceneManager_(window_, input_) {
        SetExitKey(KEY_NULL);
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
            if (input_.QuitRequested()) {
                break;
            }

            if (input_.DebugToggled()) {
                debugOverlayEnabled_ = !debugOverlayEnabled_;
            }
            if (input_.FullscreenToggled()) {
                window_.ToggleFullscreen();
            }

            const float dtRaw = raylib::Window::GetFrameTime();
            sceneManager_.Update(dtRaw);

            window_.BeginDrawing();
            window_.ClearBackground(config_.clearColor);
            sceneManager_.Draw();

            if (debugOverlayEnabled_) {
                const Scene* scene = sceneManager_.ActiveScene();
                const bool scenePaused = scene != nullptr && scene->IsPaused();
                const float dtSim = scenePaused ? 0.0f : dtRaw;
                const ui::DebugOverlayState overlay{.paused = scenePaused,
                                                    .rawDtMs = dtRaw * 1000.0f,
                                                    .simDtMs = dtSim * 1000.0f,
                                                    .moveAxis = input_.MoveAxis(),
                                                    .mouseWheel = input_.MouseWheel(),
                                                    .activeGamepad = input_.ActiveGamepad()};
                ui::DebugOverlay::Draw(sceneManager_, overlay);
            }
            window_.EndDrawing();
        }
    }

    [[nodiscard]] SceneManager& Scenes() { return sceneManager_; }

private:
    AppConfig config_;
    raylib::Window window_;
    InputManager input_;
    SceneManager sceneManager_;
    bool debugOverlayEnabled_ = true;
};
