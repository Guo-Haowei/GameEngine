#pragma once

namespace my {

// clang-format off
#define BACKEND_LIST                       \
    BACKEND_DECLARE(EMPTY,  "None")        \
    BACKEND_DECLARE(OPENGL, "OpenGL")      \
    BACKEND_DECLARE(D3D11,  "Direct3D 11") \
    BACKEND_DECLARE(D3D12,  "Direct3D 12") \
    BACKEND_DECLARE(VULKAN, "Vulkan")      \
    BACKEND_DECLARE(METAL,  "Metal")
// clang-format on

enum class Backend : uint8_t {
#define BACKEND_DECLARE(ENUM, STR) ENUM,
    BACKEND_LIST
#undef BACKEND_DECLARE
};

enum ClearFlags : uint32_t {
    CLEAR_NONE = BIT(0),
    CLEAR_COLOR_BIT = BIT(1),
    CLEAR_DEPTH_BIT = BIT(2),
    CLEAR_STENCIL_BIT = BIT(3),
};
DEFINE_ENUM_BITWISE_OPERATIONS(ClearFlags);

enum StencilFlags {
    STENCIL_FLAG_SELECTED = BIT(1),
};

enum class FilterMode {
    POINT,
    LINEAR,
    MIPMAP_LINEAR,  // @TODO: change
};

enum class AddressMode {
    WRAP,
    CLAMP,
    BORDER,
};

enum class StaticBorderColor {
    TRANSPARENT_BLACK = 0,
    OPAQUE_BLACK,
    OPAQUE_WHITE,
};

enum class InputClassification : uint8_t {
    PER_VERTEX_DATA = 0,
    PER_INSTANCE_DATA,
};

enum class FillMode : uint8_t {
    WIREFRAME = 0,
    SOLID,
};

enum class CullMode : uint8_t {
    NONE = 0,
    FRONT,
    BACK,
    FRONT_AND_BACK,
};

enum class ComparisonFunc : uint8_t {
    NEVER = 0,
    LESS,
    EQUAL,
    LESS_EQUAL,
    GREATER,
    NOT_EQUAL,
    GREATER_EQUAL,
    ALWAYS,
};

enum class DepthStencilOpDesc : uint8_t {
    ALWAYS = 0,
    Z_PASS,
    EQUAL,
};

enum class PrimitiveTopology : uint8_t {
    UNDEFINED = 0,
    POINT,
    LINE,
    TRIANGLE,
    PATCH,
};

struct Viewport {
    Viewport(int p_width, int p_height) : width(p_width), height(p_height), topLeftX(0), topLeftY(0) {}

    int width;
    int height;
    int topLeftX;
    int topLeftY;
};

}  // namespace my
