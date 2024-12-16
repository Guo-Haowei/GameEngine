#pragma once
#include "engine/core/base/concurrent_queue.h"
#include "engine/core/base/singleton.h"
#include "engine/core/framework/event_queue.h"
#include "engine/core/framework/module.h"
#include "engine/core/framework/pipeline_state_manager.h"
#include "engine/renderer/gpu_resource.h"
#include "engine/renderer/pipeline_state.h"
#include "engine/renderer/render_graph/draw_pass.h"
#include "engine/renderer/render_graph/render_graph.h"
#include "engine/renderer/render_graph/render_pass.h"
#include "engine/renderer/renderer.h"
#include "engine/renderer/sampler.h"
#include "engine/scene/material_component.h"

// @TODO: refactor
#include "cbuffer.hlsl.h"
struct MaterialConstantBuffer;
using my::renderer::RenderPass;

namespace my {

struct MeshComponent;
class Scene;
struct ImageAsset;

// @TODO: refactor
extern ConstantBuffer<PerSceneConstantBuffer> g_constantCache;
extern ConstantBuffer<DebugDrawConstantBuffer> g_debug_draw_cache;

#define RENDER_GRAPH_LIST                              \
    RENDER_GRAPH_DECLARE(DEFAULT, "default")           \
    RENDER_GRAPH_DECLARE(DUMMY, "dummy")               \
    RENDER_GRAPH_DECLARE(EXPERIMENTAL, "experimental") \
    RENDER_GRAPH_DECLARE(PATHTRACER, "pathtracer")

enum class RenderGraphName : uint8_t {
#define RENDER_GRAPH_DECLARE(ENUM, ...) ENUM,
    RENDER_GRAPH_LIST
#undef RENDER_GRAPH_DECLARE
        COUNT,
};

const char* ToString(RenderGraphName p_name);

enum class PathTracerMethod {
    ACCUMULATIVE,
    TILED,
};

struct FrameContext {
    std::shared_ptr<GpuConstantBuffer> batchCb;
    std::shared_ptr<GpuConstantBuffer> materialCb;
    std::shared_ptr<GpuConstantBuffer> boneCb;
    std::shared_ptr<GpuConstantBuffer> passCb;
    std::shared_ptr<GpuConstantBuffer> emitterCb;
    std::shared_ptr<GpuConstantBuffer> pointShadowCb;
    std::shared_ptr<GpuConstantBuffer> perFrameCb;
};

class GraphicsManager : public Singleton<GraphicsManager>, public Module, public EventListener {
public:
    static constexpr int NUM_FRAMES_IN_FLIGHT = 2;
    static constexpr int NUM_BACK_BUFFERS = 2;
    static constexpr float DEFAULT_CLEAR_COLOR[4] = { 0.0f, 0.0f, 0.0f, 1.0 };
    static constexpr PixelFormat DEFAULT_SURFACE_FORMAT = PixelFormat::R8G8B8A8_UNORM;
    static constexpr PixelFormat DEFAULT_DEPTH_STENCIL_FORMAT = PixelFormat::D32_FLOAT;

    GraphicsManager(std::string_view p_name, Backend p_backend, int p_frame_count)
        : Module(p_name),
          m_backend(p_backend),
          m_frameCount(p_frame_count) {}

    auto InitializeImpl() -> Result<void> final;
    void Update(Scene& p_scene);

    // resource
    virtual auto CreateConstantBuffer(const GpuBufferDesc& p_desc) -> Result<std::shared_ptr<GpuConstantBuffer>> = 0;
    virtual auto CreateStructuredBuffer(const GpuBufferDesc& p_desc) -> Result<std::shared_ptr<GpuStructuredBuffer>> = 0;
    virtual void UpdateBufferData(const GpuBufferDesc& p_desc, const GpuStructuredBuffer* p_buffer);

    virtual void SetRenderTarget(const DrawPass* p_draw_pass, int p_index = 0, int p_mip_level = 0) = 0;
    virtual void UnsetRenderTarget() = 0;
    virtual void BeginDrawPass(const DrawPass* p_draw_pass);
    virtual void EndDrawPass(const DrawPass* p_draw_pass);

    virtual void Clear(const DrawPass* p_draw_pass, ClearFlags p_flags, const float* p_clear_color = DEFAULT_CLEAR_COLOR, int p_index = 0) = 0;
    virtual void SetViewport(const Viewport& p_viewport) = 0;

    virtual auto CreateBuffer(const GpuBufferDesc& p_desc) -> Result<std::shared_ptr<GpuBuffer>> = 0;
    virtual void UpdateBuffer(const GpuBufferDesc& p_desc, GpuBuffer* p_buffer);

    auto CreateMesh(const MeshComponent& p_mesh) -> Result<std::shared_ptr<GpuMesh>>;

    virtual auto CreateMeshImpl(const GpuMeshDesc& p_desc,
                                uint32_t p_count,
                                const GpuBufferDesc* p_vb_descs,
                                const GpuBufferDesc* p_ib_desc)
        -> Result<std::shared_ptr<GpuMesh>> = 0;

    virtual void SetMesh(const GpuMesh* p_mesh) = 0;

    virtual LineBuffers* CreateLine(const std::vector<Point>& p_points);
    virtual void SetLine(const LineBuffers* p_buffer);
    virtual void UpdateLine(LineBuffers* p_buffer, const std::vector<Point>& p_points);

    virtual void DrawElements(uint32_t p_count, uint32_t p_offset = 0) = 0;
    virtual void DrawElementsInstanced(uint32_t p_instance_count, uint32_t p_count, uint32_t p_offset = 0) = 0;

    virtual void DrawArrays(uint32_t p_count, uint32_t p_offset = 0);

    virtual void Dispatch(uint32_t p_num_groups_x, uint32_t p_num_groups_y, uint32_t p_num_groups_z) = 0;
    virtual void BindUnorderedAccessView(uint32_t p_slot, GpuTexture* p_texture) = 0;
    virtual void UnbindUnorderedAccessView(uint32_t p_slot) = 0;

    void SetPipelineState(PipelineStateName p_name);

    virtual void SetStencilRef(uint32_t p_ref) = 0;
    virtual void SetBlendState(const BlendDesc& p_desc, const float* p_factor, uint32_t p_mask) = 0;

    virtual void BindStructuredBuffer(int p_slot, const GpuStructuredBuffer* p_buffer) = 0;
    virtual void UnbindStructuredBuffer(int p_slot) = 0;
    virtual void BindStructuredBufferSRV(int p_slot, const GpuStructuredBuffer* p_buffer) = 0;
    virtual void UnbindStructuredBufferSRV(int p_slot) = 0;

    virtual void UpdateConstantBuffer(const GpuConstantBuffer* p_buffer, const void* p_data, size_t p_size) = 0;
    template<typename T>
    void UpdateConstantBuffer(const GpuConstantBuffer* p_buffer, const std::vector<T>& p_vector) {
        UpdateConstantBuffer(p_buffer, p_vector.data(), sizeof(T) * (uint32_t)p_vector.size());
    }
    template<typename T, int N>
    void UpdateConstantBuffer(const GpuConstantBuffer* p_buffer, const std::array<T, N>& p_array) {
        UpdateConstantBuffer(p_buffer, p_array.data(), sizeof(T) * N);
    }

    virtual void BindConstantBufferRange(const GpuConstantBuffer* p_buffer, uint32_t p_size, uint32_t p_offset) = 0;
    template<typename T>
    void BindConstantBufferSlot(const GpuConstantBuffer* p_buffer, int slot) {
        BindConstantBufferRange(p_buffer, sizeof(T), slot * sizeof(T));
    }

    virtual std::shared_ptr<DrawPass> CreateDrawPass(const DrawPassDesc& p_desc) = 0;

    std::shared_ptr<GpuTexture> CreateTexture(const GpuTextureDesc& p_texture_desc, const SamplerDesc& p_sampler_desc);
    std::shared_ptr<GpuTexture> FindTexture(RenderTargetResourceName p_name) const;
    virtual void BindTexture(Dimension p_dimension, uint64_t p_handle, int p_slot) = 0;
    virtual void UnbindTexture(Dimension p_dimension, int p_slot) = 0;

    virtual void GenerateMipmap(const GpuTexture* p_texture) = 0;

    void RequestTexture(ImageAsset* p_image);

    // @TODO: move to renderer
    uint64_t GetFinalImage() const;

    // @TODO: thread safety ?
    void EventReceived(std::shared_ptr<IEvent> p_event) final;

    static auto Create() -> Result<GraphicsManager*>;

    Backend GetBackend() const { return m_backend; }

    [[nodiscard]] auto SelectRenderGraph() -> Result<void>;
    RenderGraphName GetActiveRenderGraphName() const { return m_activeRenderGraphName; }
    bool SetActiveRenderGraph(RenderGraphName p_name);
    renderer::RenderGraph* GetActiveRenderGraph();
    const auto& GetRenderGraphs() const { return m_renderGraphs; }
    bool StartPathTracer(PathTracerMethod p_method);

    FrameContext& GetCurrentFrame() { return *(m_frameContexts[m_frameIndex].get()); }

    void DrawQuad();
    void DrawQuadInstanced(uint32_t p_instance_count);
    void DrawSkybox();

protected:
    virtual auto InitializeInternal() -> Result<void> = 0;
    virtual std::shared_ptr<GpuTexture> CreateTextureImpl(const GpuTextureDesc& p_texture_desc, const SamplerDesc& p_sampler_desc) = 0;

    virtual void Render() = 0;
    virtual void Present() = 0;

    virtual void BeginFrame();
    virtual void EndFrame();
    virtual void MoveToNextFrame();
    virtual std::unique_ptr<FrameContext> CreateFrameContext();

    void OnSceneChange(const Scene& p_scene);
    virtual void OnWindowResize(int p_width, int p_height) = 0;
    virtual void SetPipelineStateImpl(PipelineStateName p_name) = 0;

    const Backend m_backend;
    RenderGraphName m_activeRenderGraphName{ RenderGraphName::DEFAULT };
    bool m_enableValidationLayer;

    std::array<std::unique_ptr<renderer::RenderGraph>, std::to_underlying(RenderGraphName::COUNT)> m_renderGraphs;

    std::map<RenderTargetResourceName, std::shared_ptr<GpuTexture>> m_resourceLookup;

    ConcurrentQueue<ImageAsset*> m_loadedImages;

    std::shared_ptr<PipelineStateManager> m_pipelineStateManager;
    std::vector<std::unique_ptr<FrameContext>> m_frameContexts;
    int m_frameIndex{ 0 };
    const int m_frameCount;

    std::shared_ptr<GpuMesh> m_screenQuadBuffers;
    std::shared_ptr<GpuMesh> m_skyboxBuffers;

public:
    // @TODO: make private
    std::shared_ptr<GpuMesh> m_boxBuffers;

    std::shared_ptr<GpuStructuredBuffer> m_pathTracerBvhBuffer;
    std::shared_ptr<GpuStructuredBuffer> m_pathTracerGeometryBuffer;
    std::shared_ptr<GpuStructuredBuffer> m_pathTracerMaterialBuffer;
    bool m_bufferUpdated = false;

    const ImageAsset* m_brdfImage{ nullptr };

    // @TODO: refactor
    LineBuffers* m_lines{ nullptr };

protected:
    void UpdateEmitters(const Scene& p_scene);
};

}  // namespace my
