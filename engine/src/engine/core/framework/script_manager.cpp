#include "script_manager.h"

#include "engine/scene/scene.h"
#include "engine/scene/scriptable_entity.h"
#include "plugins/lua_script/lua_script_manager.h"

namespace my {

void ScriptManager::Update(Scene& p_scene) {
    for (auto [entity, script] : p_scene.View<NativeScriptComponent>()) {
        // @HACK: if OnCreate() creates new NativeScriptComponents
        // the component array will be resized and invalidated
        // so save the instance pointer before hand
        // what really should do is to improve ComponentManager container to not resize,
        // but append
        // @TODO: [SCRUM-134] better ECS
        ScriptableEntity* instance = nullptr;
        if (!script.instance) {
            instance = script.instantiateFunc();
            script.instance = instance;

            instance->m_id = entity;
            instance->m_scene = &p_scene;
            instance->OnCreate();
        } else {
            instance = script.instance;
        }

        instance->OnUpdate(p_scene.m_timestep);
    }
}

Result<ScriptManager*> ScriptManager::Create() {
    return new LuaScriptManager();
}

}  // namespace my
