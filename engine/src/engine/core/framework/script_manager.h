#pragma once
#include "engine/core/framework/module.h"

namespace my {

class Scene;

class ScriptManager : public Module {
public:
    ScriptManager() : Module("ScriptManager") {}
    ScriptManager(std::string_view name) : Module(name) {}

    virtual void Update(Scene& p_scene);

    static Result<ScriptManager*> Create();
};

}  // namespace my
