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

    constexpr const char* source = R"(
GameObject = {}
GameObject.__index = GameObject

function GameObject.new(id)
	local self = setmetatable({}, GameObject)
	print("????")
	self.id = id
	return self
end

function GameObject:OnUpdate(timestep)
	print("hello from GameObject")
end

-- propeller.lua
Propeller = {}
Propeller.__index = Propeller
setmetatable(Propeller, GameObject)

function Propeller.new(id)
	local self = GameObject.new(id)
	setmetatable(self, Propeller)
	print("????")
	return self
end

function Propeller:OnUpdate(timestep)
	print("hello from Propeller")
end
)";

    CheckError(luaL_dostring(m_state, source));

    luabridge::LuaRef meta = luabridge::getGlobal(m_state, "GameObject");
    auto res = meta["new"](32);
    DEV_ASSERT(res.wasOk());
    luabridge::LuaRef instance = res[0];
    instance["OnUpdate"](0.01);

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

    for (auto [entity, script] : p_scene.m_LuaScriptComponents) {
        if (script.path.empty()) {
            continue;
        }

        const auto& meta = FindOrAdd(script.path);
        if (script.instance == 0) {
            if (meta.funcNew) {
                // @TODO: create instance
            }
        }
        DEV_ASSERT(script.instance);

        if (meta.funcUpdate) {
            // @TODO:
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

void LuaScriptManager::CheckError(int p_result) {
    if (p_result == LUA_OK) {
        return;
    }
    const char* err = lua_tostring(m_state, -1);
    LOG_ERROR("script error: {}", err);
}

const LuaScriptManager::GameObjectMetatable& LuaScriptManager::FindOrAdd(const std::string& p_path) {
    auto it = m_objectsMeta.find(p_path);
    if (it != m_objectsMeta.end()) {
        return it->second;
    }

    auto res = AssetRegistry::GetSingleton().RequestAssetSync(p_path);
    GameObjectMetatable meta;
    if (!res) {
        CRASH_NOW_MSG("error handling");
        return meta;
    }

    auto file_name = StringUtils::FileName(p_path);
    LOG("{}", file_name);

    m_objectsMeta[p_path] = meta;
    return m_objectsMeta[p_path];
}

}  // namespace my
