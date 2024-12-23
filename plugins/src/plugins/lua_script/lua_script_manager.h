#pragma once
#include "engine/core/framework/script_manager.h"

struct lua_State;

namespace my {

class Scene;

class LuaScriptManager : public ScriptManager {

public:
    LuaScriptManager() : ScriptManager("LuaScriptManager") {}

    void Update(Scene& p_scene) override;
    void OnCollision(Scene& p_scene, ecs::Entity p_entity_1, ecs::Entity p_entity_2) override;

    void OnSimBegin(Scene& p_scene) override;
    void OnSimEnd(Scene& p_scene) override;

protected:
    struct GameObjectMetatable {
        int funcNew{ 0 };
    };

    auto InitializeImpl() -> Result<void> final;
    void FinalizeImpl() final;

    GameObjectMetatable FindOrAdd(lua_State* L, const std::string& p_path, const char* p_class_name);
    Result<void> LoadMetaTable(lua_State* L, const std::string& p_path, const char* p_class_name, GameObjectMetatable& p_meta);

    std::map<std::string, GameObjectMetatable> m_objectsMeta;
};

}  // namespace my
