#ifdef OPENGL_RESOURCES_INCLUDED
#error DO NOT INCLUDE THIS FILE IN HEADER
#endif
#define OPENGL_RESOURCES_INCLUDED

#include "drivers/opengl/opengl_prerequisites.h"
#include "rendering/render_graph/subpass.h"
#include "rendering/texture.h"

namespace my {

struct OpenGLTexture : public Texture {
    using Texture::Texture;

    ~OpenGLTexture() {
        if (handle) {
            // glMakeTextureHandleNonResidentARB();

            glDeleteTextures(1, &handle);
            handle = 0;
            resident_handle = 0;
        }
    }

    uint32_t get_handle() const final { return handle; }
    uint64_t get_resident_handle() const final { return resident_handle; }
    uint64_t get_imgui_handle() const final { return handle; }

    uint32_t handle;
    uint64_t resident_handle;
};

struct OpenGLSubpass : public Subpass {
    uint32_t m_handle;
};

}  // namespace my
