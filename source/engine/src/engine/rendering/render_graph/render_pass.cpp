#include "render_pass.h"

#include "rendering/GLPrerequisites.h"

namespace my::rg {

void RenderPass::execute() {
    bind();

    if (m_func) {
        m_func();
    }

    unbind();
}

void RenderPass::create_internal(RenderPassDesc& desc) {
    m_name = std::move(desc.name);
    m_inputs = std::move(desc.dependencies);
    for (const auto& output : desc.color_attachments) {
        m_outputs.emplace_back(output->get_desc().name);
    }
    if (desc.depth_attachment) {
        m_outputs.emplace_back(desc.depth_attachment->get_desc().name);
    }
    m_func = desc.func;
}

void RenderPassGL::create_internal(RenderPassDesc& desc) {
    RenderPass::create_internal(desc);

    if (desc.type == RENDER_PASS_COMPUTE) {
        return;
    }

    glGenFramebuffers(1, &m_handle);
    bind();

    // create color attachments
    uint32_t color_attachment_count = static_cast<uint32_t>(desc.color_attachments.size());
    std::vector<GLuint> attachments;
    attachments.reserve(color_attachment_count);
    for (uint32_t i = 0; i < color_attachment_count; ++i) {
        const auto& color_attachment = desc.color_attachments[i];

        GLuint attachment = GL_COLOR_ATTACHMENT0 + i;
        switch (color_attachment->get_desc().type) {
            case RT_COLOR_ATTACHMENT: {
                // bind to frame buffer
                glFramebufferTexture2D(
                    GL_FRAMEBUFFER,                  // target
                    attachment,                      // attachment
                    GL_TEXTURE_2D,                   // texture target
                    color_attachment->get_handle(),  // texture
                    0                                // level
                );
            } break;
            default:
                break;
        }
        attachments.push_back(attachment);
    }

    if (attachments.empty()) {
        glDrawBuffer(GL_NONE);
        glReadBuffer(GL_NONE);
    } else {
        glDrawBuffers(static_cast<uint32_t>(attachments.size()), attachments.data());
    }

    if (desc.depth_attachment) {
        glFramebufferTexture2D(
            GL_FRAMEBUFFER,                       // target
            GL_DEPTH_ATTACHMENT,                  // attachment
            GL_TEXTURE_2D,                        // texture target
            desc.depth_attachment->get_handle(),  // texture
            0                                     // level
        );
    }

    DEV_ASSERT(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
    unbind();
}

void RenderPassGL::bind() {
    glBindFramebuffer(GL_FRAMEBUFFER, m_handle);
}

void RenderPassGL::unbind() {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

}  // namespace my::rg
