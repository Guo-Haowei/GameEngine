#pragma once

#define MATH_ENABLE_SIMD IN_USE

#if defined(__i386__) && defined(__x86_64__)
// @TODO: arch
#endif

#if USING(MATH_ENABLE_SIMD)
#if USING(PLATFORM_WINDOWS)
#define MATH_ENABLE_SIMD_SSE  IN_USE
#define MATH_ENABLE_SIMD_NEON NOT_IN_USE
#include <xmmintrin.h>
#elif USING(PLATFORM_APPLE)
#define MATH_ENABLE_SIMD_SSE  NOT_IN_USE
#define MATH_ENABLE_SIMD_NEON IN_USE
#include <arm_neon.h>
#else
#error Platform not supported
#endif
#else
#define MATH_ENABLE_SIMD_SSE NOT_IN_USE
#define MATH_ENABLE_SIMD_AVX NOT_IN_USE
#endif
