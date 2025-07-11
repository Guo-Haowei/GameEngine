#pragma once
#include "engine/core/base/concurrent_queue.h"
#include "engine/core/base/singleton.h"
#include "engine/math/geomath.h"
#include "engine/render_graph/framebuffer.h"
#include "engine/render_graph/render_graph.h"
#include "engine/renderer/gpu_resource.h"
#include "engine/renderer/pipeline_state.h"
#include "engine/runtime/graphics_manager_interface.h"
#include "engine/runtime/pipeline_state_manager.h"

namespace my {
#include "cbuffer.hlsl.h"
}  // namespace my

// @TODO: refactor
struct MaterialConstantBuffer;
using my::RenderPass;

// clang-format off
namespace my { class RenderGraph; }
// clang-format on

namespace my {

struct ImageAsset;
struct MeshComponent;
struct SamplerDesc;
class Scene;
struct GpuConstantBuffer;

const char* ToString(RenderGraphName p_name);

struct FrameContext {
    std::shared_ptr<GpuConstantBuffer> batchCb;
    std::shared_ptr<GpuConstantBuffer> materialCb;
    std::shared_ptr<GpuConstantBuffer> boneCb;
    std::shared_ptr<GpuConstantBuffer> passCb;
    std::shared_ptr<GpuConstantBuffer> emitterCb;
    std::shared_ptr<GpuConstantBuffer> pointShadowCb;
    std::shared_ptr<GpuConstantBuffer> perFrameCb;
};

class GraphicsManager : public IGraphicsManager {
public:
    // @TODO: rename to RenderTarget

    GraphicsManager(std::string_view p_name, Backend p_backend, int p_frame_count)
        : IGraphicsManager(p_name), m_backend(p_backend), m_frameCount(p_frame_count) {}

    auto InitializeImpl() -> Result<void> final;
    void Update(Scene& p_scene) override;

    // resource
    void UpdateBufferData(const GpuBufferDesc& p_desc, const GpuStructuredBuffer* p_buffer) override;

    void BeginDrawPass(const Framebuffer* p_framebuffer) override;
    void EndDrawPass(const Framebuffer* p_framebuffer) override;

    void UpdateBuffer(const GpuBufferDesc& p_desc, GpuBuffer* p_buffer) override;

    auto CreateMesh(const MeshComponent& p_mesh) -> Result<std::shared_ptr<GpuMesh>> override;

    void SetPipelineState(PipelineStateName p_name) override;

    std::shared_ptr<GpuTexture> CreateTexture(const GpuTextureDesc& p_texture_desc, const SamplerDesc& p_sampler_desc) override;
    std::shared_ptr<GpuTexture> CreateTexture(ImageAsset* p_image) override;
    std::shared_ptr<GpuTexture> FindTexture(std::string_view p_name) const override;

    void RequestTexture(ImageAsset* p_image) override;

    void BeginEvent(std::string_view p_event) override { unused(p_event); }
    void EndEvent() override {}

    // @TODO: move to renderer
    uint64_t GetFinalImage() const override;

    Backend GetBackend() const override { return m_backend; }

    [[nodiscard]] auto SelectRenderGraph() -> Result<void>;
    RenderGraphName GetActiveRenderGraphName() const override { return m_activeRenderGraphName; }
    bool SetActiveRenderGraph(RenderGraphName p_name) override;
    RenderGraph* GetActiveRenderGraph() override;
    FrameContext& GetCurrentFrame() override { return *(m_frameContexts[m_frameIndex].get()); }

    void DrawQuad() override;
    void DrawQuadInstanced(uint32_t p_instance_count) override;
    void DrawSkybox() override;

    void EventReceived(std::shared_ptr<IEvent> p_event) final;

protected:
    virtual auto InitializeInternal() -> Result<void> = 0;
    void BeginFrame() override;
    void EndFrame() override;
    void MoveToNextFrame() override;
    std::shared_ptr<FrameContext> CreateFrameContext() override;

    void OnSceneChange(const Scene& p_scene) override;

    const Backend m_backend;
    RenderGraphName m_activeRenderGraphName{ RenderGraphName::SCENE3D };
    bool m_enableValidationLayer;

    std::array<std::shared_ptr<RenderGraph>, std::to_underlying(RenderGraphName::COUNT)> m_renderGraphs;

    std::unordered_map<std::string_view, std::shared_ptr<GpuTexture>> m_resourceLookup;

    ConcurrentQueue<ImageAsset*> m_loadedImages;

    std::shared_ptr<PipelineStateManager> m_pipelineStateManager;
    std::vector<std::shared_ptr<FrameContext>> m_frameContexts;
    int m_frameIndex{ 0 };
    const int m_frameCount;

    std::shared_ptr<GpuMesh> m_screenQuadBuffers;
    std::shared_ptr<GpuMesh> m_skyboxBuffers;

    // auto InitializeInternal() -> Result<void> final;
    // std::shared_ptr<GpuTexture> CreateTextureImpl(const GpuTextureDesc& p_texture_desc, const SamplerDesc& p_sampler_desc) final;

protected:
    void UpdateEmitters(const Scene& p_scene) override;
};

}  // namespace my
