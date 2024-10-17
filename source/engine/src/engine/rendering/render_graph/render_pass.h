#pragma once
#include "rendering/render_graph/subpass.h"

namespace my::rg {

struct RenderPassDesc {
    std::string name;
    std::vector<std::string> dependencies;
};

class RenderPass {
public:
    void addSubpass(std::shared_ptr<Subpass> p_subpass);

    void execute();

    const std::string& getName() const { return m_name; }

protected:
    virtual void createInternal(RenderPassDesc& pass_desc);

    std::string m_name;
    std::vector<std::string> m_inputs;
    std::vector<std::shared_ptr<Subpass>> m_subpasses;

    friend class RenderGraph;
};

}  // namespace my::rg
