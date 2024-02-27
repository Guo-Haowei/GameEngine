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

// @TODO: subpass is more like render target, rename the concept later
struct Subpass {
    SubPassFunc func;
    std::vector<std::shared_ptr<RenderTarget>> color_attachments;
    std::shared_ptr<RenderTarget> depth_attachment;
};

}  // namespace my
