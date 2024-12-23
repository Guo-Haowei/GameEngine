#pragma once
#include <lua.hpp>

#include "engine/systems/ecs/entity.h"

namespace my {
class Scene;
}

namespace my::lua {

// @TODO: create a namespace
#define LUA_GLOBAL_SCENE  "g_scene"

Scene* lua_HelperGetScene(lua_State* L);

bool OpenMathLib(lua_State* L);

bool OpenSceneLib(lua_State* L);

bool OpenInputLib(lua_State* L);

bool OpenDisplayLib(lua_State* L);

}  // namespace my::lua
