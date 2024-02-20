#pragma once
#include "resource.h"

namespace my::rg {

using RenderPassFunc = void (*)(int width, int height);

enum RenderPassType {
    RENDER_PASS_SHADING,
    RENDER_PASS_COMPUTE,
};

struct SubPassDesc {
    std::vector<std::shared_ptr<Resource>> color_attachments;
    std::shared_ptr<Resource> depth_attachment;
    RenderPassFunc func = nullptr;
};

struct RenderPassDesc {
    RenderPassType type = RENDER_PASS_SHADING;
    std::string name;
    std::vector<std::string> dependencies;
    std::vector<SubPassDesc> subpasses;
};

class RenderPass {
public:
    virtual ~RenderPass() = default;

    virtual void execute() = 0;

    const std::string& get_name() const { return m_name; }

protected:
    virtual void create_internal(RenderPassDesc& pass_desc);

    std::string m_name;
    std::vector<std::string> m_inputs;
    RenderPassFunc m_func;
    int m_layer;
    int m_width;
    int m_height;

    friend class RenderGraph;
};

class RenderPassGL : public RenderPass {
public:
    void execute() override;

protected:
    void create_internal(RenderPassDesc& pass_desc) override;
    void create_subpass(const SubPassDesc& subpass_desc);

    struct SubpassData {
        uint32_t handle;
        int width;
        int height;
        RenderPassFunc func;
    };

    std::vector<SubpassData> m_subpasses;
};

}  // namespace my::rg
