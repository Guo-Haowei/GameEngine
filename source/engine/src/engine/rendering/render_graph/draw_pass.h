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
    DrawPass(const DrawPassDesc& p_desc) : desc(p_desc) {}

    std::tuple<uint32_t, uint32_t> GetBufferSize() const {
        if (desc.depthAttachment) {
            return std::make_tuple(desc.depthAttachment->desc.width, desc.depthAttachment->desc.height);
        }

        DEV_ASSERT(!desc.colorAttachments.empty());
        return std::make_tuple(desc.colorAttachments[0]->desc.width, desc.colorAttachments[0]->desc.height);
    }

    DrawPassDesc desc;
};

}  // namespace my
