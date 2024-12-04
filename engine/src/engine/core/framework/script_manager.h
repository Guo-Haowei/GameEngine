#pragma once
#include "engine/core/framework/module.h"

namespace sol {
class state;
}

namespace my {

class Scene;

class ScriptManager : public Module {

public:
    ScriptManager() : Module("ScriptManager") {}

    void Update(Scene& p_scene);

protected:
    auto InitializeImpl() -> Result<void> final;
    void FinalizeImpl() final;

    sol::state* m_state;
};

}  // namespace my
