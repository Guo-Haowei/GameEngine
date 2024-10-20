#pragma once
#include "rendering/render_target.h"

namespace my {

struct DrawPass;

using DrawPassExecuteFunc = void (*)(const DrawPass*);

struct DrawPassDesc {
    std::vector<std::shared_ptr<RenderTarget>> color_attachments;
    std::shared_ptr<RenderTarget> depth_attachment;
    DrawPassExecuteFunc exec_func = nullptr;
};

struct DrawPass {
    DrawPassExecuteFunc exec_func;
    std::vector<std::shared_ptr<RenderTarget>> color_attachments;
    std::shared_ptr<RenderTarget> depth_attachment;
};

}  // namespace my
