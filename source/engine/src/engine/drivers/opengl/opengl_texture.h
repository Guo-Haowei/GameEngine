#pragma once
#include "rendering/texture.h"

namespace my {

struct OpenGLTexture : public Texture {
    using Texture::Texture;

    ~OpenGLTexture();

    uint32_t get_handle() const final { return handle; }
    uint64_t get_resident_handle() const final { return resident_handle; }
    uint64_t get_imgui_handle() const final { return handle; }

    uint32_t handle;
    uint64_t resident_handle;
};

}  // namespace my
