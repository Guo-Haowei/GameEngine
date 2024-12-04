#pragma once
#include "engine/core/math/geomath.h"

namespace my {

enum ColorCode : uint32_t {
    COLOR_GREEN = 0x80E080,
    COLOR_YELLOW = 0xE0E080,
    COLOR_RED = 0xE08080,
    COLOR_PALEGREEN = 0x98FB98,
    COLOR_SILVER = 0xA0A0A0,
    COLOR_WHITE = 0xE0E0E0,
};

struct Color {
    float r, g, b, a;

    constexpr Color() : r(0.0f), g(0.0f), b(0.0f), a(1.0f) {}
    constexpr Color(float p_red, float p_green, float p_blue, float p_alpha) : r(p_red), g(p_green), b(p_blue), a(p_alpha) {}
    constexpr Color(float p_red, float p_green, float p_blue) : Color(p_red, p_green, p_blue, 1.0f) {}

    uint32_t ToRgb() const;
    uint32_t ToRgba() const;

    Vector4f ToVector4f() const {
        return Vector4f(r, g, b, a);
    }

    static Color Hex(uint32_t hex);
    static Color HexRgba(uint32_t hex);
};

}  // namespace my
