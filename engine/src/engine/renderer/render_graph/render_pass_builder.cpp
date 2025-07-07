#include "render_pass_builder.h"

namespace my::renderer {

RenderPassBuilder& RenderPassBuilder::Create(std::string_view p_name, const RenderGraphResourceCreateInfo& p_desc) {
    m_creates.push_back({ std::string(p_name), p_desc });
    return *this;
}

RenderPassBuilder& RenderPassBuilder::Read(ResourceAccess p_access, std::string_view p_name) {
    m_reads.emplace_back(Resource{ std::string(p_name), p_access });
    return *this;
}

RenderPassBuilder& RenderPassBuilder::Write(ResourceAccess p_access, std::string_view p_name) {
    // ignore DSV write?
    m_writes.emplace_back(Resource{ std::string(p_name), p_access });
    return *this;
}

RenderPassBuilder& RenderPassBuilder::SetExecuteFunc(ExecuteFunc p_func) {
    m_func = p_func;
    return *this;
}

}  // namespace my::renderer
