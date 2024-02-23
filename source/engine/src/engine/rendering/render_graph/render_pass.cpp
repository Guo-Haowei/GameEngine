#include "render_pass.h"

#include "rendering/opengl/opengl_prerequisites.h"

namespace my::rg {

void RenderPass::add_sub_pass(std::shared_ptr<Subpass> p_subpass) {
    m_subpasses.push_back(p_subpass);
}

void RenderPass::create_internal(RenderPassDesc& desc) {
    m_name = std::move(desc.name);
    m_inputs = std::move(desc.dependencies);
}

void RenderPass::execute() {
    for (auto& subpass : m_subpasses) {
        subpass->func(subpass.get());
    }
}

#if 0
void RenderPassGL::create_subpass(const SubpassDesc& subpass_desc) {
    //auto subpass = std::make_shared<Subpass>();

    //subpass->func = subpass_desc.func;
    //subpass->handle = 0;

    //const int num_depth_attachment = subpass_desc.depth_attachment != nullptr;
    //const int num_color_attachment = (int)subpass_desc.color_attachments.size();
    //if (!num_depth_attachment && !num_color_attachment) {
    //    m_subpasses.emplace_back(subpass);
    //    return;
    //}

    //if (num_depth_attachment) {
    //    const RenderTargetDesc& desc = subpass_desc.depth_attachment->get_desc();
    //    subpass->width = desc.width;
    //    subpass->height = desc.height;
    //} else {
    //    const RenderTargetDesc& desc = subpass_desc.color_attachments[0]->get_desc();
    //    subpass->width = desc.width;
    //    subpass->height = desc.height;
    //}

    //glGenFramebuffers(1, &subpass->handle);
    //glBindFramebuffer(GL_FRAMEBUFFER, subpass->handle);

    //if (!num_color_attachment) {
    //    glDrawBuffer(GL_NONE);
    //    glReadBuffer(GL_NONE);
    //} else {
    //    // create color attachments
    //    std::vector<GLuint> attachments;
    //    attachments.reserve(num_color_attachment);
    //    for (int i = 0; i < num_color_attachment; ++i) {
    //        const auto& color_attachment = subpass_desc.color_attachments[i];

    //        GLuint attachment = GL_COLOR_ATTACHMENT0 + i;
    //        switch (color_attachment->get_desc().type) {
    //            case RT_COLOR_ATTACHMENT: {
    //                // bind to frame buffer
    //                glFramebufferTexture2D(
    //                    GL_FRAMEBUFFER,                  // target
    //                    attachment,                      // attachment
    //                    GL_TEXTURE_2D,                   // texture target
    //                    color_attachment->get_handle(),  // texture
    //                    0                                // level
    //                );
    //            } break;
    //            default:
    //                CRASH_NOW();
    //                break;
    //        }
    //        attachments.push_back(attachment);
    //    }

    //    if (attachments.empty()) {
    //        glDrawBuffer(GL_NONE);
    //        glReadBuffer(GL_NONE);
    //    } else {
    //        glDrawBuffers(num_color_attachment, attachments.data());
    //    }
    //}

    //if (auto depth_attachment = subpass_desc.depth_attachment; depth_attachment) {
    //    switch (depth_attachment->get_desc().type) {
    //        case RT_SHADOW_MAP:
    //        case RT_DEPTH_ATTACHMENT: {
    //            glFramebufferTexture2D(GL_FRAMEBUFFER,                  // target
    //                                   GL_DEPTH_ATTACHMENT,             // attachment
    //                                   GL_TEXTURE_2D,                   // texture target
    //                                   depth_attachment->get_handle(),  // texture
    //                                   0                                // level
    //            );
    //        } break;
    //        case RT_SHADOW_CUBE_MAP: {
    //            glFramebufferTexture(GL_FRAMEBUFFER,
    //                                 GL_DEPTH_ATTACHMENT,
    //                                 depth_attachment->get_handle(),
    //                                 0);
    //        } break;
    //        default:
    //            CRASH_NOW();
    //            break;
    //    }
    //}

    //DEV_ASSERT(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);

    //glBindFramebuffer(GL_FRAMEBUFFER, 0);
    //m_subpasses.emplace_back(subpass);
}

#endif

}  // namespace my::rg
