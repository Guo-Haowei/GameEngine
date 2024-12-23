#include "lua_binding.h"

#include "engine/core/framework/display_manager.h"
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

// @TODO
struct Quat {
    Quat(const Vector3f& p_euler) {
        value = Quaternion(glm::vec3(p_euler.x, p_euler.y, p_euler.z));
    }

    Quaternion value;
};

bool OpenMathLib(lua_State* L) {
    luabridge::getGlobalNamespace(L)
        .beginClass<math::Vector2f>("Vector2")
        .addConstructor<void (*)(float, float)>()
        .addProperty("x", &Vector2f::x)
        .addProperty("y", &Vector2f::y)
        .addFunction("__add", [](const Vector2f& p_lhs, const Vector2f& p_rhs) {
            return p_lhs + p_rhs;
        })
        .addFunction("__sub", [](const Vector2f& p_lhs, const Vector2f& p_rhs) {
            return p_lhs - p_rhs;
        })
        .addFunction("__mul", [](const Vector2f& p_lhs, const Vector2f& p_rhs) {
            return p_lhs * p_rhs;
        })
        .addFunction("__div", [](const Vector2f& p_lhs, const Vector2f& p_rhs) {
            return p_lhs / p_rhs;
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
        .addFunction("__sub", [](const Vector3f& p_lhs, const Vector3f& p_rhs) {
            return p_lhs - p_rhs;
        })
        .addFunction("__mul", [](const Vector3f& p_lhs, const Vector3f& p_rhs) {
            return p_lhs * p_rhs;
        })
        .addFunction("__div", [](const Vector3f& p_lhs, const Vector3f& p_rhs) {
            return p_lhs / p_rhs;
        })
        .addFunction("normalize", [](Vector3f& p_self) {
            p_self = math::normalize(p_self);
        })
        .endClass();

    luabridge::getGlobalNamespace(L)
        .beginClass<Quat>("Quaternion")
        .addConstructor<void (*)(const Vector3f)>()
        .endClass();
    return true;
}

bool OpenInputLib(lua_State* L) {
    luabridge::getGlobalNamespace(L)
        .beginNamespace("input")
        .addFunction("GetMouseMove", []() {
            return InputManager::GetSingleton().MouseMove();
        })
        .addFunction("GetCursor", []() -> Vector2f {
            return InputManager::GetSingleton().GetCursor();
        })
        .endNamespace();
    return true;
}

bool OpenDisplayLib(lua_State* L) {
    luabridge::getGlobalNamespace(L)
        .beginNamespace("display")
        .addFunction("GetWindowSize", []() -> Vector2f {
            auto [width, height] = DisplayManager::GetSingleton().GetWindowSize();
            return Vector2f(width, height);
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
        .addFunction("GetTranslation", [](TransformComponent& p_transform) -> Vector3f {
            return p_transform.GetTranslation();
        })
        .addFunction("SetTranslation", [](TransformComponent& p_transform, const Vector3f& p_translation) {
            p_transform.SetTranslation(p_translation);
        })
        .addFunction("GetWorldTranslation", [](const TransformComponent& p_transform) {
            glm::vec3 v = p_transform.GetWorldMatrix()[3];
            return Vector3f(v.x, v.y, v.z);
        })
        .addFunction("Rotate", &TransformComponent::Rotate)
        .addFunction("SetRotation", [](TransformComponent& p_transform, const Quat& p_quat) {
            Vector4f rotation(p_quat.value.x, p_quat.value.y, p_quat.value.z, p_quat.value.w);
            p_transform.SetRotation(rotation);
        })
        .addFunction("GetScale", [](const TransformComponent& p_transform) -> Vector3f {
            return p_transform.GetScale();
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

    // RigidBodyComponent
    luabridge::getGlobalNamespace(L)
        .beginClass<RigidBodyComponent>("RigidBodyComponent")
        .addProperty("collision_type", &RigidBodyComponent::collisionType)
        .endClass();

    luabridge::getGlobalNamespace(L)
        .beginNamespace("scene")
        .addFunction(
            "GetTransform", &lua_SceneGetComponent<TransformComponent>)
        .addFunction(
            "GetPerspectiveCamera", &lua_SceneGetComponent<PerspectiveCameraComponent>)
        .addFunction(
            "GetRigidBody", &lua_SceneGetComponent<RigidBodyComponent>)
        .endNamespace();
    return true;
}

}  // namespace my::lua
