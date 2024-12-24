#pragma once
#include "engine/core/framework/script_manager.h"

struct lua_State;

namespace my {

class Scene;

struct ObjectFunctions {
    int funcNew{ 0 };
};

class LuaScriptManager : public ScriptManager {

public:
    LuaScriptManager() : ScriptManager("LuaScriptManager") {}

    void Update(Scene& p_scene) override;
    void OnCollision(Scene& p_scene, ecs::Entity p_entity_1, ecs::Entity p_entity_2) override;

    void OnSimBegin(Scene& p_scene) override;
    void OnSimEnd(Scene& p_scene) override;

protected:

    auto InitializeImpl() -> Result<void> final;
    void FinalizeImpl() final;

    ObjectFunctions FindOrAdd(lua_State* L, const std::string& p_path, const char* p_class_name);
    Result<void> LoadMetaTable(lua_State* L, const std::string& p_path, const char* p_class_name, ObjectFunctions& p_meta);

    std::map<std::string, ObjectFunctions> m_objectsMeta;
    std::string m_includePath;
    int m_gameRef{ 0 };
};

}  // namespace my
