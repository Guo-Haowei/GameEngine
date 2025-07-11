#pragma once
#include "engine/ecs/entity.h"
#include "engine/runtime/module.h"

namespace my {

class Scene;

// enum class ScriptType {
//     Native,
//     Lua,
//     Python,
//     // etc.
// };
//
// struct IScriptManager {
//     virtual bool CanHandle(ScriptType type) const = 0;
//     virtual void ExecuteScript(const std::string& code) = 0;
//     virtual ~IScriptManager() = default;
// };

class ScriptManager : public Module {
public:
    ScriptManager()
        : Module("ScriptManager") {}
    ScriptManager(std::string_view name)
        : Module(name) {}

    virtual void OnSimBegin(Scene&) {}
    virtual void OnSimEnd(Scene&) {}

    virtual void Update(Scene& p_scene, float p_timestep);
    virtual void OnCollision(Scene& p_scene, ecs::Entity p_entity_1, ecs::Entity p_entity_2);

    static Result<ScriptManager*> Create();

protected:
    virtual auto InitializeImpl() -> Result<void> override;
    virtual void FinalizeImpl() override;
};

}  // namespace my
