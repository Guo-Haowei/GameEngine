#include "engine/scene/scene.h"
#include "lua_binding.h"
#include "lua_bridge_include.h"

namespace my::lua {

static void BindTransformComponent(lua_State* L) {
    luabridge::getGlobalNamespace(L)
        .beginClass<TransformComponent>("TransformComponent")
        .addFunction("rotate", &TransformComponent::Rotate)
        .endClass();
}

template<Serializable T>
TransformComponent* lua_SceneGetComponent(lua_State* L) {
    ecs::Entity id = lua_HelperGetEntity(L);
    Scene* scene = lua_HelperGetScene(L);

        //auto tmp = luabridge::getGlobal<uint32_t>(m_state, LUA_GLOBAL_ENTITY).value();
        //DEV_ASSERT(tmp != id);

    auto component = scene->GetComponent<T>(id);
    return component;
}

bool OpenSceneLib(lua_State* L) {
    BindTransformComponent(L);

    luabridge::getGlobalNamespace(L)
        .beginNamespace("scene")
        .addFunction(
            "GetTransformComponent", &lua_SceneGetComponent<TransformComponent>)
        .endNamespace();
    return true;
}

}  // namespace my::lua
