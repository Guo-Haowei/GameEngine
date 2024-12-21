#pragma once
#include "engine/core/framework/script_manager.h"

namespace sol {
class state;
}

namespace my {

class Scene;

class LuaScriptManager : public ScriptManager {

public:
    LuaScriptManager() : ScriptManager("LuaScriptManager") {}

    void Update(Scene& p_scene);

protected:
    auto InitializeImpl() -> Result<void> final;
    void FinalizeImpl() final;

    sol::state* m_state{ nullptr };
};

}  // namespace my
