#include "engine/scene/scene.h"
#include "lua_binding.h"
#include "lua_bridge_include.h"

namespace my::lua {

static void BindTransformComponent(lua_State* L) {
    luabridge::getGlobalNamespace(L)
        .beginClass<TransformComponent>("TransformComponent")
        .addFunction("Rotate", &TransformComponent::Rotate)
        .addFunction("GetScale", [](TransformComponent* p_transform) -> Vector3f {
            return p_transform->GetScale();
        })
        .addFunction("SetScale", &TransformComponent::SetScale)
        .endClass();
}

template<Serializable T>
TransformComponent* lua_SceneGetComponent(lua_State* L) {
    if (lua_gettop(L) == 1) {
        if (lua_isnumber(L, 1)) {
            lua_Integer id = luaL_checkinteger(L, 1);
            Scene* scene = lua_HelperGetScene(L);
            auto component = scene->GetComponent<T>(ecs::Entity(static_cast<uint32_t>(id)));
            return component;
        }
    }

    DEV_ASSERT(0);
    return nullptr;
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
