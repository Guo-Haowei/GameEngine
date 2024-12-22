#include "lua_script_manager.h"

#include "engine/core/framework/application.h"
#include "engine/core/framework/asset_registry.h"
#include "engine/core/framework/input_manager.h"
#include "engine/core/framework/scene_manager.h"
#include "engine/scene/scene.h"
#include "engine/scene/scriptable_entity.h"

// lua include
#include "lua_binding.h"
#include "lua_bridge_include.h"

namespace my {

auto LuaScriptManager::InitializeImpl() -> Result<void> {
    m_state = luaL_newstate();

    // @TODO: bind
    luaL_openlibs(m_state);

    lua::OpenMathLib(m_state);
    lua::OpenSceneLib(m_state);

    return Result<void>();
}

void LuaScriptManager::FinalizeImpl() {
    if (m_state) {
        lua_close(m_state);
        m_state = nullptr;
    }
}

void LuaScriptManager::Update(Scene& p_scene) {
    const size_t scene_ptr = (size_t)&p_scene;
    if (auto res = luabridge::push(m_state, scene_ptr); !res) {
        LOG_ERROR("failed to push scene, error: {}", res.message());
        return;
    }
    lua_setglobal(m_state, LUA_GLOBAL_SCENE);

    if (auto res = luabridge::push(m_state, p_scene.m_timestep); !res) {
        LOG_ERROR("failed to push {}, error: {}", LUA_GLOBAL_TIMESTEP, res.message());
        return;
    }
    lua_setglobal(m_state, LUA_GLOBAL_TIMESTEP);

    for (auto [entity, script] : p_scene.m_LuaScriptComponents) {
        const char* source = script.GetSource();
        if (!source) {
            auto asset = m_app->GetAssetRegistry()->GetAssetByHandle<TextAsset>(AssetHandle{ script.GetScriptRef() });
            if (asset) {
                script.SetAsset(asset);
                source = asset->source.c_str();
            }
        }

        if (!source) {
            continue;
        }

        const uint32_t id = entity.GetId();
        if (auto res = luabridge::push(m_state, id); !res) {
            LOG_ERROR("failed to push id, error: {}", res.message());
            continue;
        }

        lua_setglobal(m_state, LUA_GLOBAL_ENTITY);
        if (auto code = luaL_dostring(m_state, source); code != 0) {
            const char* err = lua_tostring(m_state, -1);
            LOG_ERROR("script error: {}", err);
        }
    }

    ScriptManager::Update(p_scene);
}

}  // namespace my
