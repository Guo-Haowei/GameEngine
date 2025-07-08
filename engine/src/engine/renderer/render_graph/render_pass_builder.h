#pragma once
#include "engine/renderer/sampler.h"
#include "render_pass.h"

// clang-format off
namespace my { struct GpuTexture; }
// clang-format on

namespace my {

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
    using ImportFunc = std::function<std::shared_ptr<GpuTexture>()>;

    struct Resource {
        std::string name;
        ResourceAccess access;
    };

    RenderPassBuilder& Create(std::string_view p_name, const RenderGraphResourceCreateInfo& p_desc);
    RenderPassBuilder& Import(std::string_view p_name, ImportFunc&& p_func);

    RenderPassBuilder& Read(ResourceAccess p_access, std::string_view p_name);
    RenderPassBuilder& Write(ResourceAccess p_access, std::string_view p_name);
    RenderPassBuilder& SetExecuteFunc(ExecuteFunc p_func);

    std::string_view GetName() const { return m_name; }

private:
    RenderPassBuilder(std::string_view p_name) : m_name{ p_name } {}

    std::string m_name;
    std::vector<std::pair<std::string, RenderGraphResourceCreateInfo>> m_creates;
    std::vector<std::pair<std::string, ImportFunc>> m_imports;
    std::vector<Resource> m_reads;
    std::vector<Resource> m_writes;
    ExecuteFunc m_func;

    friend class RenderGraphBuilder;
};

}  // namespace my
