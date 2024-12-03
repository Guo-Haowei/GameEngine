#include "script_manager.h"

#include "engine/core/framework/scene_manager.h"
#include "engine/scene/scene.h"

extern "C" {
#include <lua/lauxlib.h>
#include <lua/lua.h>
#include <lua/lualib.h>
}

namespace my {

#define LUA_GLOBAL_ENTITY "g_entity"

static int lua_SceneEntityRotateX(lua_State* L) {
    if (lua_isnumber(L, 1)) {
        lua_getglobal(L, LUA_GLOBAL_ENTITY);
        if (DEV_VERIFY(lua_isinteger(L, -1))) {
            ecs::Entity entity{ static_cast<uint32_t>(lua_tointeger(L, -1)) };
            Scene* scene = SceneManager::GetSingleton().GetScenePtr();
            TransformComponent* transform = scene->GetComponent<TransformComponent>(entity);
            if (transform) {
                double angle = lua_tonumber(L, 1);
                Degree degree(static_cast<float>(angle));
                transform->RotateX(degree);
            }
        }
    }

    return 0;
}

static int lua_SceneEntityRotateY(lua_State* L) {
    if (lua_isnumber(L, 1)) {
        lua_getglobal(L, LUA_GLOBAL_ENTITY);
        if (DEV_VERIFY(lua_isinteger(L, -1))) {
            ecs::Entity entity{ static_cast<uint32_t>(lua_tointeger(L, -1)) };
            Scene* scene = SceneManager::GetSingleton().GetScenePtr();
            TransformComponent* transform = scene->GetComponent<TransformComponent>(entity);
            if (transform) {
                double angle = lua_tonumber(L, 1);
                Degree degree(static_cast<float>(angle));
                transform->RotateY(degree * scene->m_elapsedTime);
            }
        }
    }

    return 0;
}

static int lua_SceneEntityRotateZ(lua_State* L) {
    if (lua_isnumber(L, 1)) {
        lua_getglobal(L, LUA_GLOBAL_ENTITY);
        if (DEV_VERIFY(lua_isinteger(L, -1))) {
            ecs::Entity entity{ static_cast<uint32_t>(lua_tointeger(L, -1)) };
            Scene* scene = SceneManager::GetSingleton().GetScenePtr();
            TransformComponent* transform = scene->GetComponent<TransformComponent>(entity);
            if (transform) {
                double angle = lua_tonumber(L, 1);
                Degree degree(static_cast<float>(angle));
                transform->RotateZ(degree);
            }
        }
    }

    return 0;
}

static int lua_OpenSceneModule(lua_State* L) {
    lua_newtable(L);
    const struct luaL_Reg funcs[] = {
        { "EntityRotateX", lua_SceneEntityRotateX },
        { "EntityRotateY", lua_SceneEntityRotateY },
        { "EntityRotateZ", lua_SceneEntityRotateZ },
        { nullptr, nullptr }
    };
    luaL_setfuncs(L, funcs, 0);
    return 1;
}

auto ScriptManager::InitializeImpl() -> Result<void> {
    lua_State* L = luaL_newstate();
    DEV_ASSERT(L);
    luaL_openlibs(L);

    luaL_requiref(L, "Scene", lua_OpenSceneModule, 1);

    m_state = L;
    return Result<void>();
}

void ScriptManager::FinalizeImpl() {
    if (m_state) {
        lua_close(m_state);
    }
}

void ScriptManager::Update(Scene& p_scene) {
    // alias
    lua_State* L = m_state;
    for (auto [entity, script] : p_scene.m_ScriptComponents) {
        unused(entity);
        if (script.source) {
            lua_pushinteger(L, entity.GetId());
            lua_setglobal(L, LUA_GLOBAL_ENTITY);

            if (luaL_dostring(L, script.source->source.c_str()) != LUA_OK) {
                const char* error = lua_tostring(L, -1);
                LOG_ERROR("script error - {}", error);
                lua_pop(L, 1);
            }
        }
    }
}

}  // namespace my
