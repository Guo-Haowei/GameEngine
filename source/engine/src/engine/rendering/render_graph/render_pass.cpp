#include "render_pass.h"

#include "rendering/GLPrerequisites.h"

namespace my::rg {

void RenderPass::execute() {
    bind();

    if (m_func) {
        m_func(m_width, m_height, m_layer);
    }

    unbind();
}

void RenderPass::create_internal(RenderPassDesc& desc) {
    m_name = std::move(desc.name);
    m_inputs = std::move(desc.dependencies);
    m_func = desc.func;
    m_layer = desc.layer;
}

void RenderPassGL::create_internal(RenderPassDesc& desc) {
    RenderPass::create_internal(desc);

    if (desc.type == RENDER_PASS_COMPUTE) {
        return;
    }

    glGenFramebuffers(1, &m_handle);
    bind();

    // @TODO: allow resizing
    const ResourceDesc* resource_desc = nullptr;
    if (desc.depth_attachment) {
        resource_desc = &(desc.depth_attachment->get_desc());
    } else if (!desc.color_attachments.empty()) {
        resource_desc = &(desc.color_attachments[0]->get_desc());
    }
    if (resource_desc) {
        m_width = resource_desc->width;
        m_height = resource_desc->height;
    }

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
                CRASH_NOW();
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
        const auto& depth_desc = desc.depth_attachment->get_desc();
        switch (depth_desc.type) {
            case RT_SHADOW_MAP:
            case RT_DEPTH_ATTACHMENT: {
                glFramebufferTexture2D(
                    GL_FRAMEBUFFER,                       // target
                    GL_DEPTH_ATTACHMENT,                  // attachment
                    GL_TEXTURE_2D,                        // texture target
                    desc.depth_attachment->get_handle(),  // texture
                    0                                     // level
                );
            } break;
            default:
                CRASH_NOW();
                break;
        }
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
