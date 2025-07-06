#include "render_pass_builder.h"

namespace my::renderer {

RenderPassBuilder& RenderPassBuilder::Create(std::string_view p_name, GpuTextureDesc p_desc) {
    //Write(p_name);
    m_creates.push_back({ std::string(p_name), p_desc });
    return *this;
}

RenderPassBuilder& RenderPassBuilder::Read(std::string_view p_name) {
    m_reads.emplace_back(p_name);
    return *this;
}

RenderPassBuilder& RenderPassBuilder::Write(std::string_view p_name) {
    m_writes.emplace_back(p_name);
    return *this;
}

RenderPassBuilder& RenderPassBuilder::SetExecuteFunc(ExecuteFunc p_func) {
    m_func = p_func;
    return *this;
}

}  // namespace my::renderer
