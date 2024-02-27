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

class OpenGLSubpass : public Subpass {
public:
    void set_render_target(int p_index, int p_mip_level) const override;

private:
    uint32_t m_handle;

    friend class OpenGLGraphicsManager;
};

void OpenGLSubpass::set_render_target(int p_index, int p_mip_level) const {
    if (!m_handle) {
        return;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, m_handle);

    // @TODO: bind cube map/texture 2d array
    if (!color_attachments.empty()) {
        auto resource = color_attachments[0];
        if (resource->get_desc().type == RT_COLOR_ATTACHMENT_CUBE_MAP) {
            glFramebufferTexture2D(GL_FRAMEBUFFER,
                                   GL_COLOR_ATTACHMENT0,
                                   GL_TEXTURE_CUBE_MAP_POSITIVE_X + p_index,
                                   resource->get_handle(),
                                   p_mip_level);
        }
    }

    return;
}
}  // namespace my
