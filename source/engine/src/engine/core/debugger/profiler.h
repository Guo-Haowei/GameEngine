#pragma once

#define USE_PROFILER IN_USE

#if USING(USE_PROFILER)
#include "optick/optick.h"
#else
#define OPTICK_FRAME(...) (void)0
#define OPTICK_EVENT(...) (void)0
#endif
