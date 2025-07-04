#pragma once

#define MATH_ENABLE_SIMD_SSE  NOT_IN_USE
#define MATH_ENABLE_SIMD_NEON NOT_IN_USE

#if USING(ARCH_X64)
#undef MATH_ENABLE_SIMD_SSE
#define MATH_ENABLE_SIMD_SSE IN_USE
#include <xmmintrin.h>
#elif USING(ARCH_ARM64)  // #if USING(ARCH_X64)
#undef MATH_ENABLE_SIMD_NEON
#define MATH_ENABLE_SIMD_NEON IN_USE
#include <arm_neon.h>
#endif  // #else // #if USING(MATH_ENABLE_SIMD)
