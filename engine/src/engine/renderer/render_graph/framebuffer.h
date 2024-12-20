#pragma once
#include "engine/renderer/gpu_resource.h"
#include "engine/renderer/render_graph/render_graph_defines.h"

namespace my::renderer {
struct DrawData;
}

namespace my {

struct Framebuffer;

class GraphicsManager;

struct ResourceTransition {
    std::shared_ptr<GpuTexture> resource;
    int slot;
    std::function<void(GraphicsManager*, GpuTexture*, int)> beginPassFunc;
    std::function<void(GraphicsManager*, GpuTexture*, int)> endPassFunc;
};

struct FramebufferDesc {
    enum Type : uint32_t {
        TEXTURE = 0,
        SCREEN,
    } type{ TEXTURE };
    std::vector<std::shared_ptr<GpuTexture>> colorAttachments;
    std::shared_ptr<GpuTexture> depthAttachment;
    std::vector<ResourceTransition> transitions;
};

struct Framebuffer {
    Framebuffer(const FramebufferDesc& p_desc) : desc(p_desc) {
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

    FramebufferDesc desc;
    uint32_t id{ 0 };

    std::vector<std::shared_ptr<GpuTexture>> outSrvs;
};

}  // namespace my
