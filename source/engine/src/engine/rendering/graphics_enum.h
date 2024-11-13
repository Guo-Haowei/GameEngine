#pragma once

namespace my {

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

}  // namespace my
