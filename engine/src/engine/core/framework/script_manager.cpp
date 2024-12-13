#include "script_manager.h"

#include "engine/core/framework/application.h"
#include "engine/core/framework/asset_registry.h"
#include "engine/core/framework/input_manager.h"
#include "engine/core/framework/scene_manager.h"
#include "engine/scene/scene.h"
#include "engine/scene/scriptable_entity.h"

WARNING_PUSH()
WARNING_DISABLE(4100, "-Wunused-parameter")
#define SOL_PRINT_ERRORS  0
#define SOL_NO_EXCEPTIONS 1
#include "sol/sol.hpp"
WARNING_POP()

namespace my {

#define LUA_GLOBAL_SCENE  "g_scene"
#define LUA_GLOBAL_ENTITY "g_entity"

static ecs::Entity lua_HelperGetEntity(lua_State* L) {
    lua_getglobal(L, LUA_GLOBAL_ENTITY);
    ecs::Entity entity{ static_cast<uint32_t>(lua_tointeger(L, -1)) };
    lua_pop(L, 1);
    return entity;
}

static Scene* lua_HelperGetScene(lua_State* L) {
    lua_getglobal(L, LUA_GLOBAL_SCENE);
    Scene* scene = (Scene*)lua_tointeger(L, -1);
    lua_pop(L, 1);
    return scene;
}

auto ScriptManager::InitializeImpl() -> Result<void> {
    m_state = new sol::state;
    sol::state& lua = *m_state;

    lua.open_libraries(sol::lib::base);

#if USING(PLATFORM_WINDOWS)
    lua.new_usertype<Vector2f>("Vector2f",
                               sol::constructors<Vector2f(float, float)>(),
                               "x", &Vector2f::x,
                               "y", &Vector2f::y);
    lua.new_usertype<Vector3f>("Vector3f",
                               sol::constructors<Vector3f(float, float, float)>(),
                               "x", &Vector3f::x,
                               "y", &Vector3f::y,
                               "z", &Vector3f::z);
#endif

    lua["scene_helper"] = lua.create_table();
    lua["scene_helper"]["entity_rotate_x"] = [](sol::this_state L, float p_degree) {
        auto entity = lua_HelperGetEntity(L);
        auto scene = lua_HelperGetScene(L);
        TransformComponent* transform = scene->GetComponent<TransformComponent>(entity);
        if (transform) {
            transform->RotateX(Degree(p_degree));
        }
    };
    lua["scene_helper"]["entity_rotate_y"] = [](sol::this_state L, float p_degree) {
        auto entity = lua_HelperGetEntity(L);
        auto scene = lua_HelperGetScene(L);
        TransformComponent* transform = scene->GetComponent<TransformComponent>(entity);
        if (transform) {
            transform->RotateY(Degree(p_degree));
        }
    };
    lua["scene_helper"]["entity_rotate_z"] = [](sol::this_state L, float p_degree) {
        auto entity = lua_HelperGetEntity(L);
        auto scene = lua_HelperGetScene(L);
        TransformComponent* transform = scene->GetComponent<TransformComponent>(entity);
        if (transform) {
            transform->RotateZ(Degree(p_degree));
        }
    };
    lua["scene_helper"]["entity_translate"] = [](sol::this_state L, const Vector3f& p_translation) {
        auto entity = lua_HelperGetEntity(L);
        auto scene = lua_HelperGetScene(L);
        TransformComponent* transform = scene->GetComponent<TransformComponent>(entity);
        if (transform) {
            transform->Translate(p_translation);
        }
    };

    lua["input"] = lua.create_table();
    lua["input"]["mouse_move"] = []() {
        return InputManager::GetSingleton().MouseMove();
    };

    return Result<void>();
}

void ScriptManager::FinalizeImpl() {
    if (m_state) {
        delete m_state;
    }
}

void ScriptManager::Update(Scene& p_scene) {
    // alias
    sol::state& lua = *m_state;
    lua[LUA_GLOBAL_SCENE] = (size_t)(&p_scene);
    for (auto [entity, script] : p_scene.m_LuaScriptComponents) {
        const char* source = script.GetSource();
        if (!source) {
            auto asset = m_app->GetAssetRegistry()->GetAssetByHandle<TextAsset>(AssetHandle{ script.GetScriptRef() });
            if (asset) {
                script.SetAsset(asset);
                source = asset->source.c_str();
            }
        }

        if (source) {
            lua[LUA_GLOBAL_ENTITY] = entity.GetId();
            auto res = lua.script(source);
            if (!res.valid()) {
                sol::error err = res;
                LOG_ERROR("script error: {}", err.what());
            }
        }
    }

    // @TODO: generalize it
    ecs::View<NativeScriptComponent> view = p_scene.View<NativeScriptComponent>();

    for (auto [entity, script] : view) {
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

}  // namespace my
