#pragma once

// #define USE_PROFILER NOT_IN_USE
#define USE_PROFILER USE_IF(USING(PLATFORM_WINDOWS))

#if USING(USE_PROFILER)
#include "optick/optick.h"
#else
#define OPTICK_FRAME(...) (void)0
#define OPTICK_EVENT(...) (void)0
#endif
