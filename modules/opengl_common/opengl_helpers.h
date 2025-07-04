#ifdef OPENGL_HELPERS_INCLUDED
#error DO NOT INCLUDE THIS FILE IN HEADER
#endif
#define OPENGL_HELPERS_INCLUDED

#include "engine/renderer/gpu_resource.h"
#include "engine/renderer/pipeline_state.h"
#include "engine/renderer/sampler.h"
#include "opengl_prerequisites.h"

// @TODO: wrap all enums for better debugging information

namespace my::gl {

enum BUFFER_TYPE : uint32_t {
    ARRAY_BUFFER = GL_ARRAY_BUFFER,
    INDEX_BUFFER = GL_ELEMENT_ARRAY_BUFFER,
};

static inline BUFFER_TYPE Convert(GpuBufferType p_type) {
    switch (p_type) {
        case GpuBufferType::VERTEX:
            return ARRAY_BUFFER;
        case GpuBufferType::INDEX:
            return INDEX_BUFFER;
        case GpuBufferType::CONSTANT:
        case GpuBufferType::STRUCTURED:
        default:
            CRASH_NOW();
            return ARRAY_BUFFER;
    }
};

enum COMPARISON_FUNC : uint32_t {
    COMPARISON_FUNC_NEVER = GL_NEVER,
    COMPARISON_FUNC_LESS = GL_LESS,
    COMPARISON_FUNC_EQUAL = GL_EQUAL,
    COMPARISON_FUNC_LESS_EQUAL = GL_LEQUAL,
    COMPARISON_FUNC_GREATER = GL_GREATER,
    COMPARISON_FUNC_NOT_EQUAL = GL_NOTEQUAL,
    COMPARISON_FUNC_GREATER_EQUAL = GL_GEQUAL,
    COMPARISON_FUNC_ALWAYS = GL_ALWAYS,
};

static inline COMPARISON_FUNC Convert(ComparisonFunc p_func) {
    switch (p_func) {
#define COMPARISON_FUNC_ENUM(ENUM, VALUE) \
    case ComparisonFunc::ENUM:            \
        return COMPARISON_FUNC_##ENUM;
        COMPARISON_FUNC_ENUM_LIST
#undef COMPARISON_FUNC_ENUM
        default:
            CRASH_NOW();
            return COMPARISON_FUNC_ALWAYS;
    }
};

enum STENCIL_OP : uint32_t {
    STENCIL_OP_KEEP = GL_KEEP,
    STENCIL_OP_ZERO = GL_ZERO,
    STENCIL_OP_REPLACE = GL_REPLACE,
    STENCIL_OP_INCR_SAT = GL_INCR_WRAP,
    STENCIL_OP_DECR_SAT = GL_DECR_WRAP,
    STENCIL_OP_INVERT = GL_INVERT,
    STENCIL_OP_INCR = GL_INCR,
    STENCIL_OP_DECR = GL_DECR,
};

static inline STENCIL_OP Convert(StencilOp p_op) {
    switch (p_op) {
#define STENCIL_OP_ENUM(ENUM, VALUE) \
    case StencilOp::ENUM:            \
        return STENCIL_OP_##ENUM;
        STENCIL_OP_ENUM_LIST
#undef STENCIL_OP_ENUM
        default:
            CRASH_NOW();
            return STENCIL_OP_KEEP;
    }
}

enum BLEND : uint32_t {
    BLEND_ZERO = GL_ZERO,
    BLEND_ONE = GL_ONE,
    BLEND_SRC_ALPHA = GL_SRC_ALPHA,
    BLEND_INV_SRC_ALPHA = GL_ONE_MINUS_SRC_ALPHA,
};

static inline BLEND Convert(Blend p_blend) {
    switch (p_blend) {
        case Blend::BLEND_ZERO:
            return gl::BLEND_ZERO;
        case Blend::BLEND_ONE:
            return gl::BLEND_ONE;
        case Blend::BLEND_SRC_ALPHA:
            return gl::BLEND_SRC_ALPHA;
        case Blend::BLEND_INV_SRC_ALPHA:
            return gl::BLEND_INV_SRC_ALPHA;
        default:
            CRASH_NOW();
            return gl::BLEND_SRC_ALPHA;
    }
}

enum BLEND_OP : uint32_t {
    BLEND_OP_ADD = GL_FUNC_ADD,
    BLEND_OP_SUB = GL_FUNC_SUBTRACT,
};

static inline BLEND_OP Convert(BlendOp p_op) {
    switch (p_op) {
        case BlendOp::BLEND_OP_SUB:
            return gl::BLEND_OP_SUB;
        case BlendOp::BLEND_OP_ADD:
            return gl::BLEND_OP_ADD;
        default:
            CRASH_NOW();
            return gl::BLEND_OP_ADD;
    }
}

enum TOPOLOGY : uint32_t {
#if !USING(PLATFORM_WASM)
    TOPOLOGY_POINT = GL_POINT,
#endif
    TOPOLOGY_LINE = GL_LINES,
    TOPOLOGY_TRIANGLE = GL_TRIANGLES,
};

static inline TOPOLOGY Convert(PrimitiveTopology p_topology) {
    switch (p_topology) {
#if !USING(PLATFORM_WASM)
        case PrimitiveTopology::POINT:
            return TOPOLOGY_POINT;
#endif
        case PrimitiveTopology::LINE:
            return TOPOLOGY_LINE;
        case PrimitiveTopology::TRIANGLE:
            return TOPOLOGY_TRIANGLE;
        default:
            CRASH_NOW();
            return TOPOLOGY_TRIANGLE;
    }
}

inline GLuint ConvertFormat(PixelFormat p_format) {
    switch (p_format) {
        case PixelFormat::R8_UINT:
        case PixelFormat::R16_FLOAT:
        case PixelFormat::R32_FLOAT:
            return GL_RED;
        case PixelFormat::R8G8_UINT:
        case PixelFormat::R16G16_FLOAT:
        case PixelFormat::R32G32_FLOAT:
            return GL_RG;
        case PixelFormat::R8G8B8_UINT:
        case PixelFormat::R16G16B16_FLOAT:
        case PixelFormat::R32G32B32_FLOAT:
        case PixelFormat::R11G11B10_FLOAT:
            return GL_RGB;
        case PixelFormat::R8G8B8A8_UINT:
        case PixelFormat::R8G8B8A8_UNORM:
        case PixelFormat::R10G10B10A2_UINT:
        case PixelFormat::R16G16B16A16_FLOAT:
        case PixelFormat::R32G32B32A32_FLOAT:
            return GL_RGBA;
        case PixelFormat::D32_FLOAT:
            return GL_DEPTH_COMPONENT;
        case PixelFormat::R24G8_TYPELESS:
        case PixelFormat::R24_UNORM_X8_TYPELESS:
        case PixelFormat::D24_UNORM_S8_UINT:
        case PixelFormat::X24_TYPELESS_G8_UINT:
        case PixelFormat::R32G8X24_TYPELESS:
        case PixelFormat::D32_FLOAT_S8X24_UINT:
            return GL_DEPTH_STENCIL;
        default:
            CRASH_NOW();
            return 0;
    }
}

enum Format : uint32_t {
    INVALID = 0,

    RED = GL_RED,
    R8 = GL_R8,

    RG = GL_RG,
    RG8 = GL_RG8,

    RGB = GL_RGB,
    RGB8 = GL_RGB8,

    RGBA = GL_RGBA,
    RGBA8 = GL_RGBA8,

    R16F = GL_R16F,
    RG16F = GL_RG16F,
    RGB16F = GL_RGB16F,
    RGBA16F = GL_RGBA16F,
    R32F = GL_R32F,
    RG32F = GL_RG32F,
    RGB32F = GL_RGB32F,
    RGBA32F = GL_RGBA32F,
    R11F_G11F_B10F = GL_R11F_G11F_B10F,
    RGB10_A2 = GL_RGB10_A2,
    DEPTH_COMPONENT32F = GL_DEPTH_COMPONENT32F,
    DEPTH24_STENCIL8 = GL_DEPTH24_STENCIL8,
    DEPTH32F_STENCIL8 = GL_DEPTH32F_STENCIL8,
};

inline Format ConvertInternalFormat(PixelFormat p_format) {
    switch (p_format) {
        case PixelFormat::R8_UINT:
            return R8;
        case PixelFormat::R8G8_UINT:
            return RG8;
        case PixelFormat::R8G8B8_UINT:
            return RGB8;
        case PixelFormat::R8G8B8A8_UNORM:
        case PixelFormat::R8G8B8A8_UINT:
            return RGBA8;
        case PixelFormat::R16_FLOAT:
            return R16F;
        case PixelFormat::R16G16_FLOAT:
            return RG16F;
        case PixelFormat::R16G16B16_FLOAT:
            return RGB16F;
        case PixelFormat::R16G16B16A16_FLOAT:
            return RGBA16F;
        case PixelFormat::R32_FLOAT:
            return R32F;
        case PixelFormat::R32G32_FLOAT:
            return RG32F;
        case PixelFormat::R32G32B32_FLOAT:
            return RGB32F;
        case PixelFormat::R32G32B32A32_FLOAT:
            return RGBA32F;
        case PixelFormat::R11G11B10_FLOAT:
            return R11F_G11F_B10F;
        case PixelFormat::R10G10B10A2_UINT:
            return RGB10_A2;
        case PixelFormat::D32_FLOAT:
            return DEPTH_COMPONENT32F;
        case PixelFormat::R24G8_TYPELESS:
        case PixelFormat::R24_UNORM_X8_TYPELESS:
        case PixelFormat::D24_UNORM_S8_UINT:
        case PixelFormat::X24_TYPELESS_G8_UINT:
            return DEPTH24_STENCIL8;
        case PixelFormat::R32G8X24_TYPELESS:
        case PixelFormat::D32_FLOAT_S8X24_UINT:
            return DEPTH32F_STENCIL8;
        default:
            CRASH_NOW();
            return INVALID;
    }
}

inline GLuint ConvertDataType(PixelFormat p_format) {
    switch (p_format) {
        case PixelFormat::R8_UINT:
        case PixelFormat::R8G8_UINT:
        case PixelFormat::R8G8B8_UINT:
        case PixelFormat::R8G8B8A8_UINT:
        case PixelFormat::R8G8B8A8_UNORM:
        case PixelFormat::R10G10B10A2_UINT:
            return GL_UNSIGNED_BYTE;
        case PixelFormat::R16_FLOAT:
        case PixelFormat::R16G16_FLOAT:
        case PixelFormat::R16G16B16_FLOAT:
        case PixelFormat::R16G16B16A16_FLOAT:
        case PixelFormat::R32_FLOAT:
        case PixelFormat::R32G32_FLOAT:
        case PixelFormat::R32G32B32_FLOAT:
        case PixelFormat::R32G32B32A32_FLOAT:
        case PixelFormat::R11G11B10_FLOAT:
        case PixelFormat::D32_FLOAT:
            return GL_FLOAT;
        case PixelFormat::R24G8_TYPELESS:
        case PixelFormat::R24_UNORM_X8_TYPELESS:
        case PixelFormat::D24_UNORM_S8_UINT:
        case PixelFormat::X24_TYPELESS_G8_UINT:
            return GL_UNSIGNED_INT_24_8;
        case PixelFormat::R32G8X24_TYPELESS:
        case PixelFormat::D32_FLOAT_S8X24_UINT:
            return GL_FLOAT_32_UNSIGNED_INT_24_8_REV;
        default:
            CRASH_NOW();
            return 0;
    }
}

inline GLenum ConvertFilter(MinFilter p_mode) {
    switch (p_mode) {
        case MinFilter::POINT:
            return GL_NEAREST;
        case MinFilter::LINEAR:
            return GL_LINEAR;
        case MinFilter::LINEAR_MIPMAP_LINEAR:
            return GL_LINEAR_MIPMAP_LINEAR;
        default:
            CRASH_NOW();
            return 0;
    }
}

inline GLenum ConvertFilter(MagFilter p_mode) {
    switch (p_mode) {
        case MagFilter::POINT:
            return GL_NEAREST;
        case MagFilter::LINEAR:
            return GL_LINEAR;
        default:
            CRASH_NOW();
            return 0;
    }
}

inline GLenum ConvertAddressMode(AddressMode p_mode) {
    switch (p_mode) {
        case AddressMode::WRAP:
            return GL_REPEAT;
        case AddressMode::CLAMP:
            return GL_CLAMP_TO_EDGE;
#if !USING(PLATFORM_WASM)
        case AddressMode::BORDER:
            return GL_CLAMP_TO_BORDER;
#endif
        default:
            CRASH_NOW();
            return 0;
    }
}

inline void SetSampler(GLenum p_texture_type, const SamplerDesc& p_desc) {
    glTexParameteri(p_texture_type, GL_TEXTURE_MIN_FILTER, ConvertFilter(p_desc.minFilter));
    glTexParameteri(p_texture_type, GL_TEXTURE_MAG_FILTER, ConvertFilter(p_desc.magFilter));
    glTexParameteri(p_texture_type, GL_TEXTURE_WRAP_S, ConvertAddressMode(p_desc.addressU));
    glTexParameteri(p_texture_type, GL_TEXTURE_WRAP_T, ConvertAddressMode(p_desc.addressV));
    glTexParameteri(p_texture_type, GL_TEXTURE_WRAP_R, ConvertAddressMode(p_desc.addressW));

    // @TODO: border
    if (p_desc.addressU == AddressMode::BORDER ||
        p_desc.addressV == AddressMode::BORDER ||
        p_desc.addressW == AddressMode::BORDER) {
#if !USING(PLATFORM_WASM)
        glTexParameterfv(p_texture_type, GL_TEXTURE_BORDER_COLOR, p_desc.border);
#else
        CRASH_NOW();
#endif
    }
}

inline GLenum ConvertDimension(Dimension p_dimension) {
    switch (p_dimension) {
        case Dimension::TEXTURE_2D:
            return GL_TEXTURE_2D;
        case Dimension::TEXTURE_3D:
            return GL_TEXTURE_3D;
        case Dimension::TEXTURE_2D_ARRAY:
            return GL_TEXTURE_2D_ARRAY;
        case Dimension::TEXTURE_CUBE:
            return GL_TEXTURE_CUBE_MAP;
#if !USING(PLATFORM_WASM)
        case Dimension::TEXTURE_CUBE_ARRAY:
            return GL_TEXTURE_CUBE_MAP_ARRAY;
#endif
        default:
            CRASH_NOW();
            return GL_TEXTURE_2D;
    }
}

}  // namespace my::gl
