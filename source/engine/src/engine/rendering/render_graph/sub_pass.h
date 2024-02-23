#pragma once
#include "rendering/render_target.h"

namespace my::rg {

using SubPassFunc = void (*)(int width, int height);

struct SubpassDesc {
    std::vector<std::shared_ptr<RenderTarget>> color_attachments;
    std::shared_ptr<RenderTarget> depth_attachment;
    SubPassFunc func = nullptr;
};

struct Subpass {
    // handle shouldn't be here
    uint32_t handle;
    int width;
    int height;
    SubPassFunc func;

    // what resource does it have
    // how does it draw to resources
    // etc

    std::vector<std::shared_ptr<RenderTarget>> color_attachments;
    std::shared_ptr<RenderTarget> depth_attachment;

    virtual void set_render_target() = 0;
    virtual void set_render_target_level(int idx) = 0;
};

}  // namespace my::rg
