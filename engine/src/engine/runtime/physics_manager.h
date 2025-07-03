#pragma once
#include "engine/runtime/module.h"

namespace my {

class Scene;

class PhysicsManager : public Module {
public:
    PhysicsManager(std::string_view p_name) : Module(p_name) {}

    virtual void Update(Scene& p_scene) = 0;

    virtual void OnSimBegin(Scene& p_scene) = 0;
    virtual void OnSimEnd(Scene& p_scene) = 0;

    static PhysicsManager* Create();
};

}  // namespace my

extern "C" my::PhysicsManager* CreatePhysicsManager();

