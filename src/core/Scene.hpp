#pragma once

#include <string_view>

#include <raylib-cpp.hpp>

#include "../input/InputManager.hpp"

class SceneManager;

struct SceneContext {
    raylib::Window& window;
    InputManager& input;
    SceneManager& manager;
};

class Scene {
public:
    virtual ~Scene() = default;

    virtual void OnEnter(SceneContext&) {}
    virtual void OnExit(SceneContext&) {}

    virtual void Update(SceneContext&, float dt) = 0;
    virtual void Draw(SceneContext&) = 0;
    virtual std::string_view Name() const = 0;
};
