#pragma once

#define MATH_ENABLE_SIMD IN_USE

#if USING(MATH_ENABLE_SIMD)
#if USING(PLATFORM_WINDOWS)
#define MATH_ENABLE_SIMD_SSE IN_USE
#define MATH_ENABLE_SIMD_AVX NOT_IN_USE
#include <xmmintrin.h>
#elif USING(PLATFORM_APPLE)
#define MATH_ENABLE_SIMD_SSE NOT_IN_USE
#define MATH_ENABLE_SIMD_AVX IN_USE
#error "TODO"
#else
#error Platform not supported
#endif
#else
#define MATH_ENABLE_SIMD_SSE NOT_IN_USE
#define MATH_ENABLE_SIMD_AVX NOT_IN_USE
#endif
