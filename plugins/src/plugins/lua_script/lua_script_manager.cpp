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
	self.id = id
	return self
end

function GameObject:OnUpdate(timestep)
	print("hello from GameObject")
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
    lua_State* L = m_state; // alias

    const size_t scene_ptr = (size_t)&p_scene;
    if (auto res = luabridge::push(m_state, scene_ptr); !res) {
        LOG_ERROR("failed to push scene, error: {}", res.message());
        return;
    }
    lua_setglobal(L, LUA_GLOBAL_SCENE);

    for (auto [entity, script] : p_scene.m_LuaScriptComponents) {
        if (script.path.empty()) {
            continue;
        }

        const auto& meta = FindOrAdd(script.path);
        if (script.instance == 0) {
            if (meta.funcNew) {
                lua_rawgeti(L, LUA_REGISTRYINDEX, meta.funcNew);
                // @TODO: check if function
                lua_pushinteger(L, entity.GetId());
                if (auto ret = CheckError(lua_pcall(L, 1, 1, 0)); ret == LUA_OK) {
                    int instance = luaL_ref(L, LUA_REGISTRYINDEX);
                    script.instance = instance;
                    LOG_VERBOSE("instance created for entity {}", entity.GetId());
                }
                // @TODO: create instance
            }
        }
        DEV_ASSERT(script.instance);

        // push the instance to stack
        lua_rawgeti(L, LUA_REGISTRYINDEX, script.instance);
        lua_getfield(L, -1, "OnUpdate");
        if (lua_isfunction(L, -1)) {
            lua_rawgeti(L, LUA_REGISTRYINDEX, script.instance);
            lua_pushinteger(L, 1);
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

Result<void> LuaScriptManager::LoadMetaTable(const std::string& p_path, GameObjectMetatable& p_meta) {
    auto res = AssetRegistry::GetSingleton().RequestAssetSync(p_path);
    if (!res) {
        return HBN_ERROR(res.error());
    }

    auto source = dynamic_cast<const TextAsset*>(*res);

    if (auto err = CheckError(luaL_dostring(m_state, source->source.c_str())); err != LUA_OK) {
        return HBN_ERROR(ErrorCode::ERR_SCRIPT_FAILED, "failed to execute script '{}'", p_path);
    }

    // @TODO: refactor
    auto RemoveExtension = [](std::string_view p_file, std::string_view p_extension) {
        const size_t pos = p_file.rfind(p_extension);
        if (pos != std::string_view::npos && pos + p_extension.size() == p_file.size()) {
            // Remove the ".lua" extension
            return p_file.substr(0, pos);
        }

        return std::string_view();
    };

    auto file_name_with_ext = StringUtils::FileName(p_path, '/');
    auto file_name = RemoveExtension(file_name_with_ext, ".lua");
    LOG("{}", file_name);
    if (file_name.empty()) {
        return HBN_ERROR(ErrorCode::FAILURE, "file '{}' is not a valid script", p_path);
    }

    auto L = m_state;
    // check if function exists
    std::string class_name(file_name);
    lua_getglobal(L, class_name.c_str());
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

LuaScriptManager::GameObjectMetatable LuaScriptManager::FindOrAdd(const std::string& p_path) {
    auto it = m_objectsMeta.find(p_path);
    if (it != m_objectsMeta.end()) {
        return it->second;
    }

    GameObjectMetatable meta;
    auto res = LoadMetaTable(p_path, meta);
    if (!res) {
        CRASH_NOW();
    } else {
        m_objectsMeta[p_path] = meta;
    }

    return meta;
}

}  // namespace my
