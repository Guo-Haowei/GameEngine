#pragma once
#include "resource.h"

namespace my::rg {

using RenderPassFunc = void (*)(int width, int height, int);

enum RenderPassType {
    RENDER_PASS_SHADING,
    RENDER_PASS_COMPUTE,
};

struct RenderPassDesc {
    RenderPassType type = RENDER_PASS_SHADING;
    std::string name;
    std::vector<std::string> dependencies;
    std::vector<std::shared_ptr<Resource>> color_attachments;
    std::shared_ptr<Resource> depth_attachment;
    RenderPassFunc func = nullptr;
    int layer = 0;
};

class RenderPass {
public:
    virtual ~RenderPass() = default;

    void execute();

    virtual void bind() = 0;
    virtual void unbind() = 0;

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
    void bind() override;
    void unbind() override;

protected:
    void create_internal(RenderPassDesc& pass_desc) override;
    uint32_t m_handle = 0;
};

}  // namespace my::rg
