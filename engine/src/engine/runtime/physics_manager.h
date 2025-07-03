#pragma once
#include "engine/runtime/module.h"

namespace my {

class Scene;

class IPhysicsManager : public Module, public ModuleCreateRegistry<IPhysicsManager> {
public:
    using CreateFunc = IPhysicsManager* (*)();

    IPhysicsManager(std::string_view p_name) : Module(p_name) {}

    virtual void Update(Scene& p_scene) = 0;

    virtual void OnSimBegin(Scene& p_scene) = 0;
    virtual void OnSimEnd(Scene& p_scene) = 0;
};

}  // namespace my
