#pragma once
#include "engine/math/vector.h"

namespace my {

enum ColorCode : uint32_t {
    COLOR_GREEN = 0x80E080,
    COLOR_YELLOW = 0xE0E080,
    COLOR_RED = 0xE08080,
    COLOR_PALEGREEN = 0x98FB98,
    COLOR_SILVER = 0xA0A0A0,
    COLOR_WHITE = 0xE0E0E0,
};

class Color : public Vector<float, 4> {
    using Base = Vector<float, 4>;

public:
    constexpr Color()
        : Base(0, 0, 0, 1) {}
    constexpr Color(float p_r, float p_g, float p_b, float p_a)
        : Base(p_r, p_g, p_b, p_a) {}
    constexpr Color(float p_r, float p_g, float p_b)
        : Base(p_r, p_g, p_b, 1.0f) {}

    uint32_t ToRgb() const;
    uint32_t ToRgba() const;

    Vector4f ToVector4f() const {
        return Vector4f(r, g, b, a);
    }

    static constexpr Color Hex(uint32_t p_hex) {
        float b = (p_hex & 0xFF) / 255.0f;
        p_hex >>= 8;
        float g = (p_hex & 0xFF) / 255.0f;
        p_hex >>= 8;
        float r = (p_hex & 0xFF) / 255.0f;
        return Color(r, g, b, 1.0f);
    }

    static Color HexRgba(uint32_t hex);
};

}  // namespace my
