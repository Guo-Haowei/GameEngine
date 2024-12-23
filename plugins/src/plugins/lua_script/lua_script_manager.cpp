#include "lua_script_manager.h"

#include "engine/core/framework/application.h"
#include "engine/core/framework/asset_registry.h"
#include "engine/core/framework/input_manager.h"
#include "engine/core/framework/scene_manager.h"
#include "engine/core/string/string_builder.h"
#include "engine/core/string/string_utils.h"
#include "engine/scene/scene.h"
#include "engine/scene/scriptable_entity.h"

// lua include
#include "lua_binding.h"
#include "lua_bridge_include.h"

namespace my {

auto LuaScriptManager::InitializeImpl() -> Result<void> {
    m_state = luaL_newstate();

    luaL_openlibs(m_state);

    lua::OpenMathLib(m_state);
    lua::OpenSceneLib(m_state);
    lua::OpenInputLib(m_state);
    lua::OpenDisplayLib(m_state);

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
end

function GameObject:OnCollision(other)
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

inline int PushArg(lua_State*) {
    return 0;
}

template<typename T>
    requires std::is_integral_v<T>
static int PushArg(lua_State* L, const T& p_value) {
    lua_pushinteger(L, p_value);
    return 1;
}

template<typename T>
    requires std::is_floating_point_v<T>
static int PushArg(lua_State* L, const T& p_value) {
    lua_pushnumber(L, p_value);
    return 1;
}

template<typename T, typename... Args>
static int PushArg(lua_State* L, T&& p_value, Args&&... p_args) {
    PushArg<T>(L, p_value);
    PushArg(L, std::forward<Args>(p_args)...);
    return 1 + sizeof...(p_args);
}

template<typename ...Args>
static void EntityCall(lua_State* L, int p_ref, const char* p_method, Args&&... p_args) {
    lua_rawgeti(L, LUA_REGISTRYINDEX, p_ref);
    lua_getfield(L, -1, p_method);
    if (lua_isfunction(L, -1)) {
        // push self
        lua_rawgeti(L, LUA_REGISTRYINDEX, p_ref);
        int arg_count = PushArg(L, std::forward<Args>(p_args)...);
        const int result = lua_pcall(L, 1 + arg_count, 0, 0);
        if (result != LUA_OK) {
            const char* err = lua_tostring(L, -1);
            LOG_ERROR("script error: {}", err);
        }
        lua_pop(L, 1);  // pop the return value
    } else {
        DEV_ASSERT(0);
        lua_pop(L, 1);
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
                    // LOG_VERBOSE("instance created for entity {}", entity.GetId());
                }
            }
        }

        if (DEV_VERIFY(script.m_instance)) {
            EntityCall(L, script.m_instance, "OnUpdate", timestep);
        }
    }

    ScriptManager::Update(p_scene);
}

void LuaScriptManager::OnCollision(Scene& p_scene, ecs::Entity p_entity_1, ecs::Entity p_entity_2) {
    ScriptManager::OnCollision(p_scene, p_entity_1, p_entity_2);

    lua_State* L = m_state;

    LuaScriptComponent* script_1 = p_scene.GetComponent<LuaScriptComponent>(p_entity_1);
    LuaScriptComponent* script_2 = p_scene.GetComponent<LuaScriptComponent>(p_entity_2);

    if (script_1 && script_1->m_instance) {
        EntityCall(L, script_1->m_instance, "OnCollision", p_entity_2.GetId());
    }

    if (script_2 && script_2->m_instance) {
        EntityCall(L, script_2->m_instance, "OnCollision", p_entity_1.GetId());
    }
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
    if (auto res = LoadMetaTable(p_path, p_class_name, meta); !res) {
        StringStreamBuilder builder;
        builder << res.error();
        LOG_ERROR("{}", builder.ToString());
    } else {
        m_objectsMeta[p_path] = meta;
    }

    return meta;
}

}  // namespace my
