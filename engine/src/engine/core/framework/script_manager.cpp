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

void ScriptManager::OnCollision(Scene& p_scene, ecs::Entity p_entity_1, ecs::Entity p_entity_2) {
    NativeScriptComponent* script_1 = p_scene.GetComponent<NativeScriptComponent>(p_entity_1);
    NativeScriptComponent* script_2 = p_scene.GetComponent<NativeScriptComponent>(p_entity_2);

    if (script_1 && script_1->instance) {
        script_1->instance->OnCollision(p_entity_2);
    }

    if (script_2 && script_2->instance) {
        script_2->instance->OnCollision(p_entity_1);
    }
}

Result<ScriptManager*> ScriptManager::Create() {
    return new LuaScriptManager();
}

}  // namespace my
