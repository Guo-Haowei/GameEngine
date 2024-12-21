#pragma once

// @TODO: add name space
namespace my::math {

template<typename T>
concept Arithmetic = std::is_arithmetic_v<T>;
template<typename T>
concept FloatingPoint = std::is_floating_point_v<T>;

template<Arithmetic T, int N>
    requires(N >= 2 && N <= 4)
struct VectorBase;

template<Arithmetic T, int N>
struct Vector;

template<Arithmetic T>
struct Vector<T, 2>;

template<Arithmetic T>
struct Vector<T, 3>;

template<Arithmetic T>
struct Vector<T, 4>;

template<typename T, int S, int N, int A, int B, int C, int D>
struct Swizzle;

template<typename T, int N, int A, int B, int C, int D>
using Swizzle2 = Swizzle<T, 2, N, A, B, C, D>;
template<typename T, int N, int A, int B, int C, int D>
using Swizzle3 = Swizzle<T, 3, N, A, B, C, D>;
template<typename T, int N, int A, int B, int C, int D>
using Swizzle4 = Swizzle<T, 4, N, A, B, C, D>;

}  // namespace my::math
