#pragma once
#include "rendering/render_target.h"

namespace my {

struct Subpass;

using SubpassExecuteFunc = void (*)(const Subpass*);

struct SubpassDesc {
    std::vector<std::shared_ptr<RenderTarget>> color_attachments;
    std::shared_ptr<RenderTarget> depth_attachment;
    SubpassExecuteFunc exec_func = nullptr;
};

struct Subpass {
    SubpassExecuteFunc exec_func;
    std::vector<std::shared_ptr<RenderTarget>> color_attachments;
    std::shared_ptr<RenderTarget> depth_attachment;
};

}  // namespace my
