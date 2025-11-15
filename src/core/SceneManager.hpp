#pragma once

#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>

#include "Scene.hpp"

class SceneManager {
public:
    SceneManager(raylib::Window& window, InputManager& input)
        : context_{window, input, *this} {}

    template <typename TScene, typename... TArgs>
    TScene& RegisterScene(const std::string& id, TArgs&&... args) {
        auto scene = std::make_unique<TScene>(std::forward<TArgs>(args)...);
        auto* scenePtr = scene.get();
        scenes_.emplace(id, std::move(scene));
        return *scenePtr;
    }

    void SwitchTo(const std::string& id) {
        auto it = scenes_.find(id);
        if (it == scenes_.end()) {
            throw std::runtime_error("Scene '" + id + "' is not registered");
        }

        if (active_ != nullptr) {
            active_->OnExit(context_);
        }

        active_ = it->second.get();
        active_->OnEnter(context_);
    }

    void Update(float dt) {
        if (active_ != nullptr) {
            active_->Update(context_, dt);
        }
    }

    void Draw() {
        if (active_ != nullptr) {
            active_->Draw(context_);
        }
    }

    [[nodiscard]] Scene* ActiveScene() const { return active_; }

    [[nodiscard]] SceneContext& Context() { return context_; }

private:
    std::unordered_map<std::string, std::unique_ptr<Scene>> scenes_;
    Scene* active_ = nullptr;
    SceneContext context_;
};
