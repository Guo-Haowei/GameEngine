#include "script_manager.h"

#include "engine/core/framework/scene_manager.h"
#include "engine/scene/scene.h"
#include "engine/lua_binding/prerequisites.h"

namespace my {

#define LUA_GLOBAL_SCENE  "g_scene"
#define LUA_GLOBAL_ENTITY "g_entity"

#define SCRIPT_CHECK_PARAMS(EXPR, USAGE) lua_CheckParams(EXPR, USAGE)

static bool lua_CheckParams(bool p_result, const char* p_usage) {
    if (p_result == false) {
        LOG_ERROR("script error: incorrect usage '{}'", p_usage);
    }
    return p_result;
}

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

static int lua_SceneEntityRotateX(lua_State* L) {
    if (SCRIPT_CHECK_PARAMS(lua_isnumber(L, 1), "EntityRotateX(float): void result")) {
        auto entity = lua_HelperGetEntity(L);
        auto scene = lua_HelperGetScene(L);

        TransformComponent* transform = scene->GetComponent<TransformComponent>(entity);
        if (transform) {
            double angle = lua_tonumber(L, 1);
            transform->RotateX(Degree(angle));
        }
    }

    return 0;
}

static int lua_SceneEntityRotateY(lua_State* L) {
    if (SCRIPT_CHECK_PARAMS(lua_isnumber(L, 1), "EntityRotateY(float): void result")) {
        auto entity = lua_HelperGetEntity(L);
        auto scene = lua_HelperGetScene(L);

        TransformComponent* transform = scene->GetComponent<TransformComponent>(entity);
        if (transform) {
            double angle = lua_tonumber(L, 1);
            transform->RotateY(Degree(angle));
        }
    }

    return 0;
}

static int lua_SceneEntityRotateZ(lua_State* L) {
    if (SCRIPT_CHECK_PARAMS(lua_isnumber(L, 1), "EntityRotateZ(float): void result")) {
        auto entity = lua_HelperGetEntity(L);
        auto scene = lua_HelperGetScene(L);

        TransformComponent* transform = scene->GetComponent<TransformComponent>(entity);
        if (transform) {
            double angle = lua_tonumber(L, 1);
            transform->RotateZ(Degree(angle));
        }
    }

    return 0;
}

static int lua_SceneEntityRotate(lua_State* L) {
    if (SCRIPT_CHECK_PARAMS((lua_isnumber(L, 1) && lua_isnumber(L, 2) && lua_isnumber(L, 3)),
                            "EntityRotate(float, float, float): void result")) {
        auto entity = lua_HelperGetEntity(L);
        auto scene = lua_HelperGetScene(L);

        TransformComponent* transform = scene->GetComponent<TransformComponent>(entity);
        if (transform) {
            Degree x(lua_tonumber(L, 1));
            Degree y(lua_tonumber(L, 2));
            Degree z(lua_tonumber(L, 3));
            transform->Rotate(Vector3f(x.GetRadians(), y.GetRadians(), z.GetRadians()));
        }
    }

    return 0;
}

int lua_OpenSceneModule(lua_State* L) {
    lua_newtable(L);
    const struct luaL_Reg funcs[] = {
        { "EntityRotateX", lua_SceneEntityRotateX },
        { "EntityRotateY", lua_SceneEntityRotateY },
        { "EntityRotateZ", lua_SceneEntityRotateZ },
        { "EntityRotate", lua_SceneEntityRotate },
        { nullptr, nullptr }
    };
    luaL_setfuncs(L, funcs, 0);
    return 1;
}

auto ScriptManager::InitializeImpl() -> Result<void> {
    sol::state* lua = new sol::state;
    lua->open_libraries(sol::lib::base);

    //luaL_requiref(L, "Scene", lua_OpenSceneModule, 1);

    const char* script = R"(
    print(1111)
    func()
)";
    auto res = lua->script(script);
    if (!res.valid()) {
        sol::error err = res;
        LOG_ERROR("script error: {}", err.what());
    }

    m_state = lua;
    return Result<void>();
}

void ScriptManager::FinalizeImpl() {
    if (m_state) {
        delete m_state;
    }
}

void ScriptManager::Update(Scene& p_scene) {
    // alias
    lua_State* L = nullptr;
    lua_pushinteger(L, (size_t)(&p_scene));
    lua_setglobal(L, LUA_GLOBAL_SCENE);
    for (auto [entity, script] : p_scene.m_ScriptComponents) {
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
