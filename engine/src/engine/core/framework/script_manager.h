#pragma once
#include "engine/core/framework/module.h"
#include "engine/systems/ecs/entity.h"

namespace my {

class Scene;

class ScriptManager : public Module {
public:
    ScriptManager() : Module("ScriptManager") {}
    ScriptManager(std::string_view name) : Module(name) {}

    virtual void OnSimBegin(Scene&) {}
    virtual void OnSimEnd(Scene&) {}

    virtual void Update(Scene& p_scene);
    virtual void OnCollision(Scene& p_scene, ecs::Entity p_entity_1, ecs::Entity p_entity_2);

    static Result<ScriptManager*> Create();

protected:
    virtual auto InitializeImpl() -> Result<void> override;
    virtual void FinalizeImpl() override;
};

}  // namespace my
