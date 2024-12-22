#include "lua_math_binding.h"

#include <lua.hpp>

WARNING_PUSH()
WARNING_DISABLE(4702, "-Wunused-parameter") // unreachable code
#include <LuaBridge/LuaBridge.h>
WARNING_POP()

#include "engine/math/vector.h"

namespace my::lua {

struct lua_Vector3 {
    lua_Vector3(float x, float y, float z) : vec(x, y, z) {}


    Vector3f vec;
};

bool BindMathLib(lua_State*) {

    lua_State* L = luaL_newstate();
    luaL_openlibs(L);

    // Register MyClass
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

    // Run Lua script
    const char* source =
    R"(
        local vec1 = Vector3(1, 2, 3)
        local vec2 = Vector3(2, 3, 4)
        local vec3 = vec1 + vec2
        print(vec1.x, vec1.y, vec1.z)
        print(vec2.x, vec2.y, vec2.z)
        print(vec3.x, vec3.y, vec3.z)
    )";

    if (auto code = luaL_dostring(L, source); code != 0) {
        LOG_ERROR("Script error: {}", lua_tostring(L, -1));
    }

    lua_close(L);
    return 0;
}

}  // namespace my::lua
