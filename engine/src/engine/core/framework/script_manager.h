#pragma once
#include "engine/core/framework/module.h"

struct lua_State;

namespace my {

class Scene;

class ScriptManager : public Module {

public:
    ScriptManager() : Module("ScriptManager") {}

    void Update(Scene& p_scene);

protected:
    auto InitializeImpl() -> Result<void> final;
    void FinalizeImpl() final;

    lua_State* m_state;
};

}  // namespace my
