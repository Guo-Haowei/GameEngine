#include "lua_binding.h"

#include "engine/core/framework/input_manager.h"
#include "engine/math/vector.h"
#include "engine/scene/scene.h"
#include "lua_bridge_include.h"

namespace my::lua {

Scene* lua_HelperGetScene(lua_State* L) {
    lua_getglobal(L, LUA_GLOBAL_SCENE);
    Scene* scene = (Scene*)lua_tointeger(L, -1);
    lua_pop(L, 1);
    return scene;
}

bool OpenMathLib(lua_State* L) {
    luabridge::getGlobalNamespace(L)
        .beginClass<math::Vector2f>("Vector2")
        .addConstructor<void (*)(float, float)>()
        .addProperty("x", &Vector2f::x)
        .addProperty("y", &Vector2f::y)
        .addFunction("__add", [](const Vector2f& p_lhs, const Vector2f& p_rhs) {
            return p_lhs + p_rhs;
        })
        .endClass();

    luabridge::getGlobalNamespace(L)
        .beginClass<math::Vector3f>("Vector3")
        .addConstructor<void (*)(float, float, float)>()
        .addProperty("x", &Vector3f::x)
        .addProperty("y", &Vector3f::y)
        .addProperty("z", &Vector3f::z)
        .addFunction("__add", [](const Vector3f& p_lhs, const Vector3f& p_rhs) {
            return p_lhs + p_rhs;
        })
        .endClass();
    return true;
}

bool OpenInputLib(lua_State* L) {
    luabridge::getGlobalNamespace(L)
        .beginNamespace("input")
        .addFunction("GetMouseMove", []() {
            return InputManager::GetSingleton().MouseMove();
        })
        .endNamespace();
    return true;
}

template<Serializable T>
T* lua_SceneGetComponent(lua_State* L) {
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
    // TransformComponent
    luabridge::getGlobalNamespace(L)
        .beginClass<TransformComponent>("TransformComponent")
        .addFunction("Translate", &TransformComponent::Translate)
        .addFunction("Rotate", &TransformComponent::Rotate)
        .addFunction("GetScale", [](TransformComponent* p_transform) -> Vector3f {
            return p_transform->GetScale();
        })
        .addFunction("SetScale", &TransformComponent::SetScale)
        .endClass();

    // PerspectiveCameraComponent
    luabridge::getGlobalNamespace(L)
        .beginClass<PerspectiveCameraComponent>("PerspectiveCameraComponent")
        .addFunction("GetFovy", [](PerspectiveCameraComponent* p_camera) -> float {
            return p_camera->GetFovy().GetDegree();
        })
        .addFunction("SetFovy", [](PerspectiveCameraComponent* p_camera, float p_degree) {
            p_camera->SetFovy(Degree(p_degree));
        })
        .endClass();

    luabridge::getGlobalNamespace(L)
        .beginNamespace("scene")
        .addFunction(
            "GetTransform", &lua_SceneGetComponent<TransformComponent>)
        .addFunction(
            "GetPerspectiveCamera", &lua_SceneGetComponent<PerspectiveCameraComponent>)
        .endNamespace();
    return true;
}

}  // namespace my::lua
