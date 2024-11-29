#ifdef OPENGL_PREREQUISITES_INCLUDED
#error DO NOT INCLUDE THIS FILE IN HEADER
#endif
#define OPENGL_PREREQUISITES_INCLUDED

#pragma once
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <glad/glad.h>

#include "engine/renderer/gpu_resource.h"
#include "engine/renderer/pipeline_state.h"
#include "engine/renderer/sampler.h"

// @TODO: make this included by cpp only

namespace my::gl {

// @TODO: wrap all enums
enum COMPARISON_FUNC {
    COMPARISON_FUNC_NEVER = GL_NEVER,
    COMPARISON_FUNC_LESS = GL_LESS,
    COMPARISON_FUNC_EQUAL = GL_EQUAL,
    COMPARISON_FUNC_LESS_EQUAL = GL_LEQUAL,
    COMPARISON_FUNC_GREATER = GL_GREATER,
    COMPARISON_FUNC_NOT_EQUAL = GL_NOTEQUAL,
    COMPARISON_FUNC_GREATER_EQUAL = GL_GEQUAL,
    COMPARISON_FUNC_ALWAYS = GL_ALWAYS,
};

enum STENCIL_OP {
    STENCIL_OP_KEEP = GL_KEEP,
    STENCIL_OP_ZERO = GL_ZERO,
    STENCIL_OP_REPLACE = GL_REPLACE,
    STENCIL_OP_INCR_SAT = GL_INCR_WRAP,
    STENCIL_OP_DECR_SAT = GL_DECR_WRAP,
    STENCIL_OP_INVERT = GL_INVERT,
    STENCIL_OP_INCR = GL_INCR,
    STENCIL_OP_DECR = GL_DECR,
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
            return GL_DEPTH_STENCIL;
        default:
            CRASH_NOW();
            return 0;
    }
}

inline GLuint ConvertInternalFormat(PixelFormat p_format) {
    switch (p_format) {
        case PixelFormat::R8_UINT:
            return GL_RED;
        case PixelFormat::R8G8_UINT:
            return GL_RG;
        case PixelFormat::R8G8B8_UINT:
            return GL_RGB;
        case PixelFormat::R8G8B8A8_UINT:
            return GL_RGBA;
        case PixelFormat::R16_FLOAT:
            return GL_R16F;
        case PixelFormat::R16G16_FLOAT:
            return GL_RG16F;
        case PixelFormat::R16G16B16_FLOAT:
            return GL_RGB16F;
        case PixelFormat::R16G16B16A16_FLOAT:
            return GL_RGBA16F;
        case PixelFormat::R32_FLOAT:
            return GL_R32F;
        case PixelFormat::R32G32_FLOAT:
            return GL_RG32F;
        case PixelFormat::R32G32B32_FLOAT:
            return GL_RGB32F;
        case PixelFormat::R32G32B32A32_FLOAT:
            return GL_RGBA32F;
        case PixelFormat::R11G11B10_FLOAT:
            return GL_R11F_G11F_B10F;
        case PixelFormat::R10G10B10A2_UINT:
            return GL_RGB10_A2;
        case PixelFormat::D32_FLOAT:
            return GL_DEPTH_COMPONENT32F;
        case PixelFormat::R24G8_TYPELESS:
        case PixelFormat::R24_UNORM_X8_TYPELESS:
        case PixelFormat::D24_UNORM_S8_UINT:
        case PixelFormat::X24_TYPELESS_G8_UINT:
            return GL_DEPTH24_STENCIL8;
        default:
            CRASH_NOW();
            return 0;
    }
}

inline GLuint ConvertDataType(PixelFormat p_format) {
    switch (p_format) {
        case PixelFormat::R8_UINT:
        case PixelFormat::R8G8_UINT:
        case PixelFormat::R8G8B8_UINT:
        case PixelFormat::R8G8B8A8_UINT:
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
        default:
            CRASH_NOW();
            return 0;
    }
}

inline GLenum ConvertFilter(FilterMode p_mode) {
    switch (p_mode) {
        case FilterMode::POINT:
            return GL_NEAREST;
        case FilterMode::LINEAR:
            return GL_LINEAR;
        case FilterMode::MIPMAP_LINEAR:
            return GL_LINEAR_MIPMAP_LINEAR;
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
        case AddressMode::BORDER:
            return GL_CLAMP_TO_BORDER;
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
        glTexParameterfv(p_texture_type, GL_TEXTURE_BORDER_COLOR, p_desc.border);
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
        case Dimension::TEXTURE_CUBE_ARRAY:
            return GL_TEXTURE_CUBE_MAP_ARRAY;
        default:
            CRASH_NOW();
            return GL_TEXTURE_2D;
    }
}

}  // namespace my::gl
