#pragma once
#include "lua_binding.h"

namespace my::lua {

ecs::Entity lua_HelperGetEntity(lua_State* L) {
    lua_getglobal(L, LUA_GLOBAL_ENTITY);
    ecs::Entity entity{ static_cast<uint32_t>(lua_tointeger(L, -1)) };
    lua_pop(L, 1);
    return entity;
}

Scene* lua_HelperGetScene(lua_State* L) {
    lua_getglobal(L, LUA_GLOBAL_SCENE);
    Scene* scene = (Scene*)lua_tointeger(L, -1);
    lua_pop(L, 1);
    return scene;
}

}  // namespace my::lua
