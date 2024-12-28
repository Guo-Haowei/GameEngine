#pragma once
#include "detail/vector2.h"
#include "detail/vector3.h"
#include "detail/vector4.h"
#include "detail/vector_math.h"

namespace my {

using Vector2i = Vector<int, 2>;
using Vector3i = Vector<int, 3>;
using Vector4i = Vector<int, 4>;
using Vector2u = Vector<uint32_t, 2>;
using Vector3u = Vector<uint32_t, 3>;
using Vector4u = Vector<uint32_t, 4>;
using Vector2f = Vector<float, 2>;
using Vector3f = Vector<float, 3>;
using Vector4f = Vector<float, 4>;

static_assert(sizeof(Vector2f) == 8);
static_assert(sizeof(Vector3f) == 12);
static_assert(sizeof(Vector4f) == 16);
static_assert(sizeof(Vector2i) == 8);
static_assert(sizeof(Vector3i) == 12);
static_assert(sizeof(Vector4i) == 16);
static_assert(sizeof(Vector2u) == 8);
static_assert(sizeof(Vector3u) == 12);
static_assert(sizeof(Vector4u) == 16);

static_assert(alignof(Vector4f) == 16);

}  // namespace my
