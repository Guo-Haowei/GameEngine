#pragma once

namespace my {

// clang-format off
#define BACKEND_LIST                                \
    BACKEND_DECLARE(EMPTY,  "None",         empty)  \
    BACKEND_DECLARE(OPENGL, "OpenGL",       opengl) \
    BACKEND_DECLARE(D3D11,  "Direct3D 11",  d3d11)  \
    BACKEND_DECLARE(D3D12,  "Direct3D 12",  d3d12)  \
    BACKEND_DECLARE(VULKAN, "Vulkan",       vulkan) \
    BACKEND_DECLARE(METAL,  "Metal",        metal)
// clang-format on

enum class Backend : uint8_t {
#define BACKEND_DECLARE(ENUM, ...) ENUM,
    BACKEND_LIST
#undef BACKEND_DECLARE
        COUNT,
};

inline const char* ToString(Backend p_backend) {
    DEV_ASSERT_INDEX(p_backend, Backend::COUNT);
    static const char* m_table[] = {
#define BACKEND_DECLARE(ENUM, STR, DVAR) #DVAR,
        BACKEND_LIST
#undef BACKEND_DECLARE
    };
    static_assert(array_length(m_table) == std::to_underlying(Backend::COUNT));
    return m_table[std::to_underlying(p_backend)];
}

enum ClearFlags : uint32_t {
    CLEAR_NONE = BIT(0),
    CLEAR_COLOR_BIT = BIT(1),
    CLEAR_DEPTH_BIT = BIT(2),
    CLEAR_STENCIL_BIT = BIT(3),
};
DEFINE_ENUM_BITWISE_OPERATIONS(ClearFlags);

enum StencilFlags : uint8_t {
    STENCIL_FLAG_SELECTED = BIT(8),
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

#define COMPARISON_FUNC_ENUM_LIST          \
    COMPARISON_FUNC_ENUM(NEVER, 1)         \
    COMPARISON_FUNC_ENUM(LESS, 2)          \
    COMPARISON_FUNC_ENUM(EQUAL, 3)         \
    COMPARISON_FUNC_ENUM(LESS_EQUAL, 4)    \
    COMPARISON_FUNC_ENUM(GREATER, 5)       \
    COMPARISON_FUNC_ENUM(NOT_EQUAL, 6)     \
    COMPARISON_FUNC_ENUM(GREATER_EQUAL, 7) \
    COMPARISON_FUNC_ENUM(ALWAYS, 8)

enum class ComparisonFunc : uint8_t {
#define COMPARISON_FUNC_ENUM(ENUM, VALUE) ENUM = VALUE,
    COMPARISON_FUNC_ENUM_LIST
#undef COMPARISON_FUNC_ENUM
};

#define STENCIL_OP_ENUM_LIST     \
    STENCIL_OP_ENUM(KEEP, 1)     \
    STENCIL_OP_ENUM(ZERO, 2)     \
    STENCIL_OP_ENUM(REPLACE, 3)  \
    STENCIL_OP_ENUM(INCR_SAT, 4) \
    STENCIL_OP_ENUM(DECR_SAT, 5) \
    STENCIL_OP_ENUM(INVERT, 6)   \
    STENCIL_OP_ENUM(INCR, 7)     \
    STENCIL_OP_ENUM(DECR, 8)

enum class StencilOp : uint8_t {
#define STENCIL_OP_ENUM(ENUM, VALUE) ENUM = VALUE,
    STENCIL_OP_ENUM_LIST
#undef STENCIL_OP_ENUM
};

enum class Blend : uint8_t {
    BLEND_ZERO = 1,
    BLEND_ONE,
    BLEND_SRC_ALPHA,
    BLEND_INV_SRC_ALPHA,
};

enum class BlendOp : uint8_t {
    BLEND_OP_ADD,
    BLEND_OP_SUB,
};

enum ColorWriteEnable : uint8_t {
    COLOR_WRITE_ENABLE_NONE = BIT(0),
    COLOR_WRITE_ENABLE_RED = BIT(1),
    COLOR_WRITE_ENABLE_GREEN = BIT(2),
    COLOR_WRITE_ENABLE_BLUE = BIT(3),
    COLOR_WRITE_ENABLE_ALPHA = BIT(4),
    COLOR_WRITE_ENABLE_ALL =
        COLOR_WRITE_ENABLE_RED | COLOR_WRITE_ENABLE_GREEN | COLOR_WRITE_ENABLE_BLUE | COLOR_WRITE_ENABLE_ALPHA,
};
DEFINE_ENUM_BITWISE_OPERATIONS(ColorWriteEnable);

struct RenderTargetBlendDesc {
    bool blendEnabled{ false };
    Blend blendSrc{ Blend::BLEND_ONE };
    Blend blendDest{ Blend::BLEND_ZERO };
    BlendOp blendOp{ BlendOp::BLEND_OP_ADD };
    ColorWriteEnable colorWriteMask{ COLOR_WRITE_ENABLE_ALL };
};

struct BlendDesc {
    bool alphaToCoverageEnable{ false };
    bool independentBlendEnable{ false };
    RenderTargetBlendDesc renderTargets[8]{};
};

// @TODO: refactor
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
