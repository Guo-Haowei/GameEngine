#include "opengl_subpass.h"

#include "drivers/opengl/opengl_prerequisites.h"

namespace my {

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
