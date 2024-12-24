#pragma once
#include <lua.hpp>

#include "engine/systems/ecs/entity.h"

namespace my {
class Scene;
}

namespace my::lua {

#define LUA_GLOBAL_SCENE  "g_scene"

void SetPreloadFunc(lua_State* L);

bool OpenMathLib(lua_State* L);

bool OpenInputLib(lua_State* L);

bool OpenDisplayLib(lua_State* L);

bool OpenEngineLib(lua_State* L);

bool OpenSceneLib(lua_State* L);

}  // namespace my::lua
