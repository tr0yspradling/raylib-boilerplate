#pragma once

#include <raylib-cpp.hpp>
#include <string>
#include <utility>

#include "../ui/DebugOverlay.hpp"
#include "Config.hpp"
#include "SceneManager.hpp"

class Application {
public:
    explicit Application(AppConfig config) :
        config_(std::move(config)), window_(config_.width, config_.height, config_.title, config_.windowFlags),
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
            if (input_.QuitRequested()) {
                break;
            }

            if (input_.DebugToggled()) {
                debugOverlayEnabled_ = !debugOverlayEnabled_;
            }
            if (input_.FullscreenToggled()) {
                window_.ToggleFullscreen();
                paused_ = true;
            }

            Scene* scene = sceneManager_.ActiveScene();
            const bool pauseAllowed = scene == nullptr || scene->AllowsPause();
            if (!pauseAllowed) {
                paused_ = false;
            } else if (input_.PauseToggled()) {
                paused_ = !paused_;
            }

            if (pauseAllowed && (raylib::Window::IsResized() || !raylib::Window::IsFocused())) {
                paused_ = true;
            }

            const float dtRaw = raylib::Window::GetFrameTime();
            const float dtSim = paused_ ? 0.0f : dtRaw;
            sceneManager_.Update(dtSim);

            window_.BeginDrawing();
            window_.ClearBackground(config_.clearColor);
            sceneManager_.Draw();

            if (paused_) {
                const raylib::Vector2 size = window_.GetSize();
                DrawRectangleRounded({size.x * 0.5f - 115.0f, 18.0f, 230.0f, 48.0f}, 0.2f, 10,
                                     raylib::Color{0, 0, 0, 160});
                raylib::Color{255, 235, 125, 255}.DrawText("PAUSED", static_cast<int>(size.x) / 2 - 64, 28, 32);
            }

            if (debugOverlayEnabled_) {
                const ui::DebugOverlayState overlay{.paused = paused_,
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
    bool paused_ = false;
    bool debugOverlayEnabled_ = true;
};
