#pragma once
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif

#if USING(PLATFORM_WASM)
#include <GLES3/gl3.h>
#else
#include <glad/glad.h>
#endif

#define USE_GLES3 USE_IF(USING(PLATFORM_WASM))
