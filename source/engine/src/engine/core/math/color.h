#pragma once

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
#pragma warning(push)
#pragma warning(disable : 4201)
    union {
        struct
        {
            float r, g, b, a;
        };
        float components[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    };
#pragma warning(pop)

    constexpr Color() = default;
    constexpr Color(float p_red, float p_green, float p_blue, float p_alpha) : r(p_red), g(p_green), b(p_blue), a(p_alpha) {}
    constexpr Color(float p_red, float p_green, float p_blue) : Color(p_red, p_green, p_blue, 1.0f) {}

    uint32_t toRgb() const;
    uint32_t toRgba() const;

    static Color hex(uint32_t hex);
    static Color hexRgba(uint32_t hex);
};

}  // namespace my
