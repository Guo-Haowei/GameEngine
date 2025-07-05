#pragma once
#include "render_pass.h"

namespace my::renderer {

class RenderPassBuilder {
public:
    RenderPassBuilder& Create(std::string_view p_name, GpuTextureDesc p_desc);
    RenderPassBuilder& Read(std::string_view p_name);
    RenderPassBuilder& Write(std::string_view p_name);
    RenderPassBuilder& SetExecuteFunc(ExecuteFunc p_func);

    std::string_view GetName() const { return m_name; }

private:
    RenderPassBuilder(std::string_view p_name) : m_name{ p_name } {}

    std::string_view m_name;
    std::vector<std::pair<std::string_view, GpuTextureDesc>> m_creates;
    std::vector<std::string_view> m_reads;
    std::vector<std::string_view> m_writes;
    ExecuteFunc m_func;

    friend class RenderGraphBuilder;
};

}  // namespace my::renderer
