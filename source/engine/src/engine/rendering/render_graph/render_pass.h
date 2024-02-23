#pragma once
#include "rendering/render_graph/subpass.h"

namespace my::rg {

struct RenderPassDesc {
    std::string name;
    std::vector<std::string> dependencies;
};

class RenderPass {
public:
    void add_sub_pass(std::shared_ptr<Subpass> p_subpass);

    void execute();

    const std::string& get_name() const { return m_name; }

protected:
    virtual void create_internal(RenderPassDesc& pass_desc);

    std::string m_name;
    std::vector<std::string> m_inputs;
    std::vector<std::shared_ptr<Subpass>> m_subpasses;

    friend class RenderGraph;
};

}  // namespace my::rg
