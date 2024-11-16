#pragma once
#include "rendering/render_target.h"

namespace my {

struct DrawPass;

using DrawPassExecuteFunc = void (*)(const DrawPass*);

// @TODO: fix this, DrawPassDesc and DrawPass the same
struct DrawPassDesc {
    std::vector<std::shared_ptr<GpuTexture>> colorAttachments;
    std::shared_ptr<GpuTexture> depthAttachment;
    DrawPassExecuteFunc execFunc{ nullptr };
};

struct DrawPass {
    DrawPassExecuteFunc execFunc{ nullptr };
    std::vector<std::shared_ptr<GpuTexture>> colorAttachments;
    std::shared_ptr<GpuTexture> depthAttachment;
};

}  // namespace my
