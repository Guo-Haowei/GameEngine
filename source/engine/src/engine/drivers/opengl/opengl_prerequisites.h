#pragma once
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <glad/glad.h>

#include "rendering/pixel_format.h"
#include "rendering/sampler.h"

namespace my::gl {

inline GLuint convert_format(PixelFormat format) {
    switch (format) {
        case FORMAT_R8_UINT:
        case FORMAT_R16_FLOAT:
        case FORMAT_R32_FLOAT:
            return GL_RED;
        case FORMAT_R8G8_UINT:
        case FORMAT_R16G16_FLOAT:
        case FORMAT_R32G32_FLOAT:
            return GL_RG;
        case FORMAT_R8G8B8_UINT:
        case FORMAT_R16G16B16_FLOAT:
        case FORMAT_R32G32B32_FLOAT:
            return GL_RGB;
        case FORMAT_R8G8B8A8_UINT:
        case FORMAT_R16G16B16A16_FLOAT:
        case FORMAT_R32G32B32A32_FLOAT:
            return GL_RGBA;
        case FORMAT_D32_FLOAT:
            return GL_DEPTH_COMPONENT;
        default:
            CRASH_NOW();
            return 0;
    }
}

inline GLuint convert_internal_format(PixelFormat format) {
    switch (format) {
        case FORMAT_R8_UINT:
            return GL_RED;
        case FORMAT_R8G8_UINT:
            return GL_RG;
        case FORMAT_R8G8B8_UINT:
            return GL_RGB;
        case FORMAT_R8G8B8A8_UINT:
            return GL_RGBA;
        case FORMAT_R16_FLOAT:
            return GL_R16F;
        case FORMAT_R16G16_FLOAT:
            return GL_RG16F;
        case FORMAT_R16G16B16_FLOAT:
            return GL_RGB16F;
        case FORMAT_R16G16B16A16_FLOAT:
            return GL_RGBA16F;
        case FORMAT_R32_FLOAT:
            return GL_R32F;
        case FORMAT_R32G32_FLOAT:
            return GL_RG32F;
        case FORMAT_R32G32B32_FLOAT:
            return GL_RGB32F;
        case FORMAT_R32G32B32A32_FLOAT:
            return GL_RGBA32F;
        case FORMAT_D32_FLOAT:
            // return GL_DEPTH_COMPONENT;
            return GL_DEPTH_COMPONENT32F;
        default:
            CRASH_NOW();
            return 0;
    }
}

inline GLuint convert_data_type(PixelFormat format) {
    switch (format) {
        case FORMAT_R8_UINT:
        case FORMAT_R8G8_UINT:
        case FORMAT_R8G8B8_UINT:
        case FORMAT_R8G8B8A8_UINT:
            return GL_UNSIGNED_BYTE;
        case FORMAT_R16_FLOAT:
        case FORMAT_R16G16_FLOAT:
        case FORMAT_R16G16B16_FLOAT:
        case FORMAT_R16G16B16A16_FLOAT:
        case FORMAT_R32_FLOAT:
        case FORMAT_R32G32_FLOAT:
        case FORMAT_R32G32B32_FLOAT:
        case FORMAT_R32G32B32A32_FLOAT:
        case FORMAT_D32_FLOAT:
            return GL_FLOAT;
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

    switch (p_texture_type) {
        case GL_TEXTURE_2D:
            break;
        case GL_TEXTURE_CUBE_MAP:
            glTexParameteri(p_texture_type, GL_TEXTURE_WRAP_R, convert_address_mode(p_desc.mode_w));
            break;
        default:
            CRASH_NOW();
            break;
    }

    // @TODO: border
    if (p_desc.mode_u == AddressMode::BORDER ||
        p_desc.mode_v == AddressMode::BORDER ||
        p_desc.mode_w == AddressMode::BORDER) {
        glTexParameterfv(p_texture_type, GL_TEXTURE_BORDER_COLOR, p_desc.border);
    }
}

}  // namespace my::gl
