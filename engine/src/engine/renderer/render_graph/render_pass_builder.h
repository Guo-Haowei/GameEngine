#pragma once
#include "engine/renderer/sampler.h"
#include "render_pass.h"

namespace my::renderer {

struct RenderGraphResourceCreateInfo {
    GpuTextureDesc resourceDesc;
    SamplerDesc samplerDesc = PointClampSampler();
};

// @TODO: move to somewhere else
enum class ResourceAccess : uint8_t {
    SRV,
    UAV,
    RTV,
    DSV,
    Present,
    CopySrc,
    CopyDst,
    DepthRead,
    DepthWrite,
};

class RenderPassBuilder {
public:
    RenderPassBuilder& Create(ResourceAccess p_access, std::string_view p_name, const RenderGraphResourceCreateInfo& p_desc);
    RenderPassBuilder& Read(ResourceAccess p_access, std::string_view p_name);
    RenderPassBuilder& Write(ResourceAccess p_access, std::string_view p_name);
    RenderPassBuilder& SetExecuteFunc(ExecuteFunc p_func);

    std::string_view GetName() const { return m_name; }

private:
    RenderPassBuilder(std::string_view p_name) : m_name{ p_name } {}

    std::string m_name;
    std::vector<std::pair<std::string, RenderGraphResourceCreateInfo>> m_creates;
    std::vector<std::string> m_reads;
    std::vector<std::string> m_writes;
    ExecuteFunc m_func;

    friend class RenderGraphBuilder;
};

}  // namespace my::renderer
