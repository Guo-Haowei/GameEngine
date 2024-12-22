#pragma once
#include "engine/core/framework/script_manager.h"

struct lua_State;

namespace my {

class Scene;

class LuaScriptManager : public ScriptManager {

public:
    LuaScriptManager() : ScriptManager("LuaScriptManager") {}

    void Update(Scene& p_scene);

protected:
    struct GameObjectMetatable {
        int funcNew{ 0 };
    };

    auto InitializeImpl() -> Result<void> final;
    void FinalizeImpl() final;

    int CheckError(int p_result);
    GameObjectMetatable FindOrAdd(const std::string& p_path, const char* p_class_name);
    Result<void> LoadMetaTable(const std::string& p_path, const char* p_class_name, GameObjectMetatable& p_meta);

    lua_State* m_state{ nullptr };
    std::map<std::string, GameObjectMetatable> m_objectsMeta;
};

}  // namespace my
