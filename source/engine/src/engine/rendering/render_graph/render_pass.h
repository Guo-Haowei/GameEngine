#pragma once
#include "rendering/render_graph/sub_pass.h"

namespace my::rg {

struct RenderPassDesc {
    std::string name;
    std::vector<std::string> dependencies;
    std::vector<SubpassDesc> subpasses;
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
    int m_layer;
    int m_width;
    int m_height;

    friend class RenderGraph;
};

// @TODO: refactor
class RenderPassGL : public RenderPass {
public:
    void execute() override;

protected:
    void create_internal(RenderPassDesc& pass_desc) override;
    void create_subpass(const SubpassDesc& subpass_desc);

    std::vector<std::shared_ptr<Subpass>> m_subpasses;
};

}  // namespace my::rg
