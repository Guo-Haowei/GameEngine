#pragma once
#include "rendering/render_target.h"

namespace my {

struct Subpass;

using SubPassFunc = void (*)(const Subpass*);

struct SubpassDesc {
    std::vector<std::shared_ptr<RenderTarget>> color_attachments;
    std::shared_ptr<RenderTarget> depth_attachment;
    SubPassFunc func = nullptr;
};

struct Subpass {
    SubPassFunc func;
    std::vector<std::shared_ptr<RenderTarget>> color_attachments;
    std::shared_ptr<RenderTarget> depth_attachment;

    virtual void set_render_target(int p_index) const = 0;

    void set_render_target() const {
        set_render_target(0);
    }
};

}  // namespace my
