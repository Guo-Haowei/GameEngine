#include "render_pass_builder.h"

namespace my::renderer {

RenderPassBuilder& RenderPassBuilder::Create(ResourceAccess p_access, std::string_view p_name, const RenderGraphResourceCreateInfo& p_desc) {
    unused(p_access);
    // Write(p_name);
    m_creates.push_back({ std::string(p_name), p_desc });
    return *this;
}

RenderPassBuilder& RenderPassBuilder::Read(ResourceAccess p_access, std::string_view p_name) {
    unused(p_access);
    m_reads.emplace_back(p_name);
    return *this;
}

RenderPassBuilder& RenderPassBuilder::Write(ResourceAccess p_access, std::string_view p_name) {
    // ignore DSV write?
    if (p_access != ResourceAccess::DSV) {
        m_writes.emplace_back(p_name);
    }
    return *this;
}

RenderPassBuilder& RenderPassBuilder::SetExecuteFunc(ExecuteFunc p_func) {
    m_func = p_func;
    return *this;
}

}  // namespace my::renderer
