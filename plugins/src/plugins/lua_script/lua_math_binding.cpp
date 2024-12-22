#include "engine/math/vector.h"
#include "lua_binding.h"
#include "lua_bridge_include.h"

namespace my::lua {

bool OpenMathLib(lua_State* L) {
    luabridge::getGlobalNamespace(L)
        .beginClass<math::Vector2f>("Vector2")
        .addConstructor<void (*)(float, float)>()
        .addProperty("x", &Vector2f::x)
        .addProperty("y", &Vector2f::y)
        .addFunction("__add", [](const Vector2f& p_lhs, const Vector2f& p_rhs) {
            return p_lhs + p_rhs;
        })
        .endClass();

    luabridge::getGlobalNamespace(L)
        .beginClass<math::Vector3f>("Vector3")
        .addConstructor<void (*)(float, float, float)>()
        .addProperty("x", &Vector3f::x)
        .addProperty("y", &Vector3f::y)
        .addProperty("z", &Vector3f::z)
        .addFunction("__add", [](const Vector3f& p_lhs, const Vector3f& p_rhs) {
            return p_lhs + p_rhs;
        })
        .endClass();
    return true;
}

}  // namespace my::lua
