#pragma once
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <glad/glad.h>

#include "rendering/sampler.h"
#include "rendering/texture.h"

// @TODO: make this included by cpp only

namespace my::gl {

inline GLuint convert_format(PixelFormat format) {
    switch (format) {
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
        case PixelFormat::D24_UNORM_S8_UINT:
            return GL_DEPTH_STENCIL;
        default:
            CRASH_NOW();
            return 0;
    }
}

inline GLuint convert_internal_format(PixelFormat format) {
    switch (format) {
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
        case PixelFormat::D24_UNORM_S8_UINT:
            return GL_DEPTH24_STENCIL8;
        default:
            CRASH_NOW();
            return 0;
    }
}

inline GLuint convert_data_type(PixelFormat format) {
    switch (format) {
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
        case PixelFormat::D24_UNORM_S8_UINT:
            return GL_UNSIGNED_INT_24_8;
        default:
            CRASH_NOW();
            return 0;
    }
}

inline GLenum convert_filter(FilterMode p_mode) {
    switch (p_mode) {
        case FilterMode::NEAREST:
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

inline GLenum convert_address_mode(AddressMode p_mode) {
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

inline void set_sampler(GLenum p_texture_type, const SamplerDesc& p_desc) {
    glTexParameteri(p_texture_type, GL_TEXTURE_MIN_FILTER, convert_filter(p_desc.min));
    glTexParameteri(p_texture_type, GL_TEXTURE_MAG_FILTER, convert_filter(p_desc.mag));
    glTexParameteri(p_texture_type, GL_TEXTURE_WRAP_S, convert_address_mode(p_desc.mode_u));
    glTexParameteri(p_texture_type, GL_TEXTURE_WRAP_T, convert_address_mode(p_desc.mode_v));
    glTexParameteri(p_texture_type, GL_TEXTURE_WRAP_R, convert_address_mode(p_desc.mode_w));

    // @TODO: border
    if (p_desc.mode_u == AddressMode::BORDER ||
        p_desc.mode_v == AddressMode::BORDER ||
        p_desc.mode_w == AddressMode::BORDER) {
        glTexParameterfv(p_texture_type, GL_TEXTURE_BORDER_COLOR, p_desc.border);
    }
}

inline GLenum convert_dimension(Dimension p_dimension) {
    switch (p_dimension) {
        case my::TEXTURE_2D:
            return GL_TEXTURE_2D;
        case my::TEXTURE_3D:
            return GL_TEXTURE_3D;
        case my::TEXTURE_2D_ARRAY:
            return GL_TEXTURE_2D_ARRAY;
        case my::TEXTURE_CUBE:
            return GL_TEXTURE_CUBE_MAP;
        default:
            CRASH_NOW();
            return GL_TEXTURE_2D;
    }
}

}  // namespace my::gl
