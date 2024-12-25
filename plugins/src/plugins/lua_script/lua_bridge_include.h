#pragma once

extern "C" {
#include "lauxlib.h"
#include "lua.h"
#include "lualib.h"
}

WARNING_PUSH()
WARNING_DISABLE(4702, "-Wunused-parameter")  // unreachable code
#include <LuaBridge/LuaBridge.h>
WARNING_POP()
