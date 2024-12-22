#include "lua_script_manager.h"

#include "engine/core/framework/application.h"
#include "engine/core/framework/asset_registry.h"
#include "engine/core/framework/input_manager.h"
#include "engine/core/framework/scene_manager.h"
#include "engine/core/string/string_utils.h"
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

    // @TODO: refactor this
    constexpr const char* source = R"(
GameObject = {}
GameObject.__index = GameObject

function GameObject.new(id)
	local self = setmetatable({}, GameObject)
	self.id = id
	return self
end

function GameObject:OnUpdate(timestep)
	print("hello from GameObject")
end
)";
    if (auto res = CheckError(luaL_dostring(m_state, source)); res != LUA_OK) {
        return HBN_ERROR(ErrorCode::ERR_SCRIPT_FAILED, "failed to initialize GameObject");
    }

    return Result<void>();
}

void LuaScriptManager::FinalizeImpl() {
    if (m_state) {
        lua_close(m_state);
        m_state = nullptr;
    }
}

void LuaScriptManager::Update(Scene& p_scene) {
    lua_State* L = m_state; // alias
    const lua_Number timestep = p_scene.m_timestep;

    const size_t scene_ptr = (size_t)&p_scene;
    if (auto res = luabridge::push(m_state, scene_ptr); !res) {
        LOG_ERROR("failed to push scene, error: {}", res.message());
        return;
    }
    lua_setglobal(L, LUA_GLOBAL_SCENE);

    for (auto [entity, script] : p_scene.m_LuaScriptComponents) {
        if (script.m_path.empty()) {
            continue;
        }

        const auto& meta = FindOrAdd(script.m_path, script.m_className.c_str());
        if (script.m_instance == 0) {
            if (meta.funcNew) {
                lua_rawgeti(L, LUA_REGISTRYINDEX, meta.funcNew);
                // @TODO: check if function
                lua_pushinteger(L, entity.GetId());
                if (auto ret = CheckError(lua_pcall(L, 1, 1, 0)); ret == LUA_OK) {
                    int instance = luaL_ref(L, LUA_REGISTRYINDEX);
                    script.m_instance = instance;
                    LOG_VERBOSE("instance created for entity {}", entity.GetId());
                }
                // @TODO: create instance
            }
        }
        DEV_ASSERT(script.m_instance);

        // push the instance to stack
        lua_rawgeti(L, LUA_REGISTRYINDEX, script.m_instance);
        lua_getfield(L, -1, "OnUpdate");
        if (lua_isfunction(L, -1)) {
            lua_rawgeti(L, LUA_REGISTRYINDEX, script.m_instance);
            lua_pushnumber(L, timestep);
            CheckError(lua_pcall(L, 2, 0, 0));
            lua_pop(L, 1); // pop the return value
        } else {
            DEV_ASSERT(0);
            lua_pop(L, 1); // remove the value (not a function)
        }
    }
#if 0

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
        CheckError(luaL_dostring(m_state, source));
    }
    #endif

    ScriptManager::Update(p_scene);
}

int LuaScriptManager::CheckError(int p_result) {
    if (p_result != LUA_OK) {
        const char* err = lua_tostring(m_state, -1);
        LOG_ERROR("script error: {}", err);
    }
    return p_result;
}

Result<void> LuaScriptManager::LoadMetaTable(const std::string& p_path, const char* p_class_name, GameObjectMetatable& p_meta) {
    auto res = AssetRegistry::GetSingleton().RequestAssetSync(p_path);
    if (!res) {
        return HBN_ERROR(res.error());
    }

    auto source = dynamic_cast<const TextAsset*>(*res);

    if (auto err = CheckError(luaL_dostring(m_state, source->source.c_str())); err != LUA_OK) {
        return HBN_ERROR(ErrorCode::ERR_SCRIPT_FAILED, "failed to execute script '{}'", p_path);
    }

    auto L = m_state;
    // check if function exists
    lua_getglobal(L, p_class_name);
    if (!lua_istable(L, -1)) {
        CRASH_NOW();
    }

    lua_getfield(L, -1, "new");
    auto ref = luaL_ref(L, LUA_REGISTRYINDEX);
    if (ref == LUA_REFNIL) {
        CRASH_NOW();
    }
    p_meta.funcNew = ref;
    return Result<void>();
}

LuaScriptManager::GameObjectMetatable LuaScriptManager::FindOrAdd(const std::string& p_path, const char* p_class_name) {
    auto it = m_objectsMeta.find(p_path);
    if (it != m_objectsMeta.end()) {
        return it->second;
    }

    GameObjectMetatable meta;
    auto res = LoadMetaTable(p_path, p_class_name, meta);
    if (!res) {
        CRASH_NOW();
    } else {
        m_objectsMeta[p_path] = meta;
    }

    return meta;
}

}  // namespace my
