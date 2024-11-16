#pragma once
#include "rendering/gpu_resource.h"
#include "rendering/render_graph/render_graph_defines.h"

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
