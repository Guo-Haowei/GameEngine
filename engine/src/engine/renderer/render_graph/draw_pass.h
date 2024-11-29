#pragma once
#include "engine/renderer/gpu_resource.h"
#include "engine/renderer/render_graph/render_graph_defines.h"

namespace my {

struct DrawPass;

class GraphicsManager;

using DrawPassExecuteFunc = void (*)(const DrawPass*);

struct ResourceTransition {
    std::shared_ptr<GpuTexture> resource;
    int slot;
    std::function<void(GraphicsManager*, GpuTexture*, int)> beginPassFunc;
    std::function<void(GraphicsManager*, GpuTexture*, int)> endPassFunc;
};

struct DrawPassDesc {
    std::vector<std::shared_ptr<GpuTexture>> colorAttachments;
    std::shared_ptr<GpuTexture> depthAttachment;

    std::vector<ResourceTransition> transitions;

    DrawPassExecuteFunc execFunc{ nullptr };
};

struct DrawPass {
    DrawPass(const DrawPassDesc& p_desc) : desc(p_desc) {
        // @TODO: better way
        for (auto it : desc.colorAttachments) {
            outSrvs.emplace_back(it);
        }
        if (desc.depthAttachment) {
            outSrvs.emplace_back(desc.depthAttachment);
        }
    }

    std::tuple<uint32_t, uint32_t> GetBufferSize() const {
        if (desc.depthAttachment) {
            return std::make_tuple(desc.depthAttachment->desc.width, desc.depthAttachment->desc.height);
        }

        DEV_ASSERT(!desc.colorAttachments.empty());
        return std::make_tuple(desc.colorAttachments[0]->desc.width, desc.colorAttachments[0]->desc.height);
    }

    DrawPassDesc desc;
    uint32_t id{ 0 };

    std::vector<std::shared_ptr<GpuTexture>> outSrvs;
};

}  // namespace my
