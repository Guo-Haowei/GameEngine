#include "opengl_subpass.h"

#include "rendering/opengl/opengl_prerequisites.h"

namespace my {

void OpenGLSubpass::set_render_target(int p_index) const {
    unused(p_index);
    if (!m_handle) {
        return;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, m_handle);

    const int num_color_attachment = (int)color_attachments.size();

    // glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
    //                    GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, envCubemap, 0);

    if (num_color_attachment) {
        for (int idx = 0; idx < num_color_attachment; ++idx) {
            const auto& color_attachment = color_attachments[idx];
            GLenum attachment = GL_COLOR_ATTACHMENT0 + idx;
            switch (color_attachment->get_desc().type) {
                case RT_COLOR_ATTACHMENT_2D: {
                    glFramebufferTexture2D(
                        GL_FRAMEBUFFER,                  // target
                        attachment,                      // attachment
                        GL_TEXTURE_2D,                   // texture target
                        color_attachment->get_handle(),  // texture
                        0                                // level
                    );
                } break;
                default:
                    CRASH_NOW();
                    break;
            }
        }
    }

    // @TODO: move it to bind and unbind
    if (depth_attachment) {
        switch (depth_attachment->get_desc().type) {
            case RT_SHADOW_2D:
            case RT_DEPTH_ATTACHMENT_2D: {
                glFramebufferTexture2D(GL_FRAMEBUFFER,                  // target
                                       GL_DEPTH_ATTACHMENT,             // attachment
                                       GL_TEXTURE_2D,                   // texture target
                                       depth_attachment->get_handle(),  // texture
                                       0                                // level
                );
            } break;
            case RT_SHADOW_CUBE_MAP: {
                glFramebufferTexture(GL_FRAMEBUFFER,
                                     GL_DEPTH_ATTACHMENT,
                                     depth_attachment->get_handle(),
                                     0);
            } break;
            default:
                CRASH_NOW();
                break;
        }
    }

    return;
}

}  // namespace my
