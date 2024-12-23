#pragma once

// #define USE_PROFILER NOT_IN_USE
#define USE_PROFILER USE_IF(USING(PLATFORM_WINDOWS))

#if USING(USE_PROFILER)
#include "optick/optick.h"
#define HBN_PROFILE_FRAME(...)  OPTICK_FRAME(__VA_ARGS__)
#define HBN_PROFILE_EVENT(...)  OPTICK_EVENT(__VA_ARGS__)
#define HBN_PROFILE_THREAD(...) OPTICK_THREAD(__VA_ARGS__)
#else
#define HBN_PROFILE_FRAME(...)  (void)0
#define HBN_PROFILE_EVENT(...)  (void)0
#define HBN_PROFILE_THREAD(...) (void)0
#endif
