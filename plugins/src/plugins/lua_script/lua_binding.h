#pragma once
#include <lua.hpp>

#include "engine/systems/ecs/entity.h"

namespace my {
class Scene;
}

namespace my::lua {

// @TODO: create a namespace
#define LUA_GLOBAL_SCENE  "g_scene"
#define LUA_GLOBAL_ENTITY "g_entity"
#define LUA_GLOBAL_TIMESTEP "timestep"

ecs::Entity lua_HelperGetEntity(lua_State* L);

Scene* lua_HelperGetScene(lua_State* L);

bool OpenMathLib(lua_State* L);

bool OpenSceneLib(lua_State* L);

}  // namespace my::lua
