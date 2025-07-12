#pragma once
#include "engine/core/base/singleton.h"
#include "engine/runtime/event_queue.h"
#include "engine/runtime/module.h"

// @TODO: refactor
struct MaterialConstantBuffer;

namespace my {
class RenderGraph;
}

namespace my {

enum class Backend : uint8_t;
enum ClearFlags : uint32_t;
enum class Dimension : uint32_t;
enum class RenderGraphName : uint8_t;
enum PipelineStateName : uint8_t;

class Scene;
struct MeshComponent;

struct BlendDesc;
struct Framebuffer;
struct FramebufferDesc;
struct FrameContext;
struct GpuBuffer;
struct GpuBufferDesc;
struct GpuConstantBuffer;
struct GpuMeshDesc;
struct GpuStructuredBuffer;
struct GpuTexture;
struct GpuTextureDesc;
struct ImageAsset;
struct SamplerDesc;
struct Viewport;

struct GpuMesh;

class IGraphicsManager : public Singleton<IGraphicsManager>,
                         public Module,
                         public EventListener,
                         public ModuleCreateRegistry<IGraphicsManager> {
public:
    static constexpr int NUM_FRAMES_IN_FLIGHT = 2;
    static constexpr int NUM_BACK_BUFFERS = 2;
    static constexpr float DEFAULT_CLEAR_COLOR[4] = { 0.0f, 0.0f, 0.0f, 1.0 };

    IGraphicsManager(std::string_view p_name)
        : Module(p_name) {}

    virtual auto InitializeImpl() -> Result<void> = 0;
    virtual void Update(Scene& p_scene) = 0;

    // resource
    virtual auto CreateConstantBuffer(const GpuBufferDesc& p_desc) -> Result<std::shared_ptr<GpuConstantBuffer>> = 0;
    virtual auto CreateStructuredBuffer(const GpuBufferDesc& p_desc) -> Result<std::shared_ptr<GpuStructuredBuffer>> = 0;
    virtual void UpdateBufferData(const GpuBufferDesc& p_desc, const GpuStructuredBuffer* p_buffer) = 0;

    virtual void SetRenderTarget(const Framebuffer* p_framebuffer, int p_index = 0, int p_mip_level = 0) = 0;
    virtual void UnsetRenderTarget() = 0;
    virtual void BeginDrawPass(const Framebuffer* p_framebuffer) = 0;
    virtual void EndDrawPass(const Framebuffer* p_framebuffer) = 0;

    virtual void Clear(const Framebuffer* p_framebuffer,
                       ClearFlags p_flags,
                       const float* p_clear_color = DEFAULT_CLEAR_COLOR,
                       float p_clear_depth = 1.0f,
                       uint8_t p_clear_stencil = 0,
                       int p_index = 0) = 0;

    virtual void SetViewport(const Viewport& p_viewport) = 0;

    virtual auto CreateBuffer(const GpuBufferDesc& p_desc) -> Result<std::shared_ptr<GpuBuffer>> = 0;
    virtual void UpdateBuffer(const GpuBufferDesc& p_desc, GpuBuffer* p_buffer) = 0;

    virtual auto CreateMesh(const MeshComponent& p_mesh) -> Result<std::shared_ptr<GpuMesh>> = 0;

    virtual auto CreateMeshImpl(const GpuMeshDesc& p_desc,
                                uint32_t p_count,
                                const GpuBufferDesc* p_vb_descs,
                                const GpuBufferDesc* p_ib_desc) -> Result<std::shared_ptr<GpuMesh>> = 0;

    virtual void SetMesh(const GpuMesh* p_mesh) = 0;

    virtual void DrawElements(uint32_t p_count, uint32_t p_offset = 0) = 0;
    virtual void DrawElementsInstanced(uint32_t p_instance_count, uint32_t p_count, uint32_t p_offset = 0) = 0;
    virtual void DrawArrays(uint32_t p_count, uint32_t p_offset = 0) = 0;
    virtual void DrawArraysInstanced(uint32_t p_instance_count, uint32_t p_count, uint32_t p_offset = 0) = 0;

    virtual void Dispatch(uint32_t p_num_groups_x, uint32_t p_num_groups_y, uint32_t p_num_groups_z) = 0;
    virtual void BindUnorderedAccessView(uint32_t p_slot, GpuTexture* p_texture) = 0;
    virtual void UnbindUnorderedAccessView(uint32_t p_slot) = 0;

    virtual void SetPipelineState(PipelineStateName p_name) = 0;

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

    virtual std::shared_ptr<Framebuffer> CreateFramebuffer(const FramebufferDesc& p_desc) = 0;

    virtual std::shared_ptr<GpuTexture> CreateTexture(const GpuTextureDesc& p_texture_desc, const SamplerDesc& p_sampler_desc) = 0;
    virtual std::shared_ptr<GpuTexture> CreateTexture(ImageAsset* p_image) = 0;
    virtual std::shared_ptr<GpuTexture> FindTexture(std::string_view p_name) const = 0;
    virtual void BindTexture(Dimension p_dimension, uint64_t p_handle, int p_slot) = 0;
    virtual void UnbindTexture(Dimension p_dimension, int p_slot) = 0;

    virtual void GenerateMipmap(const GpuTexture* p_texture) = 0;

    virtual void BeginEvent(std::string_view p_event) = 0;
    virtual void EndEvent() = 0;

    virtual void RequestTexture(ImageAsset* p_image) = 0;

    // @TODO: move to renderer
    virtual uint64_t GetFinalImage() const = 0;

    // @TODO: thread safety ?
    virtual void EventReceived(std::shared_ptr<IEvent> p_event) = 0;

    // static auto Create() -> Result<GraphicsManager*>;

    virtual Backend GetBackend() const = 0;

    virtual RenderGraphName GetActiveRenderGraphName() const = 0;
    virtual bool SetActiveRenderGraph(RenderGraphName p_name) = 0;
    virtual RenderGraph* GetActiveRenderGraph() = 0;

    virtual FrameContext& GetCurrentFrame() = 0;

    virtual void DrawQuad() = 0;
    virtual void DrawQuadInstanced(uint32_t p_instance_count) = 0;
    virtual void DrawSkybox() = 0;

protected:
    virtual std::shared_ptr<GpuTexture> CreateTextureImpl(const GpuTextureDesc& p_texture_desc, const SamplerDesc& p_sampler_desc) = 0;

    virtual void Render() = 0;
    virtual void Present() = 0;

    virtual void BeginFrame() = 0;
    virtual void EndFrame() = 0;
    virtual void MoveToNextFrame() = 0;
    virtual std::shared_ptr<FrameContext> CreateFrameContext() = 0;

    virtual void OnSceneChange(const Scene& p_scene) = 0;
    virtual void OnWindowResize(int p_width, int p_height) = 0;
    virtual void SetPipelineStateImpl(PipelineStateName p_name) = 0;

public:
    // @TODO: make private
    std::shared_ptr<GpuMesh> m_boxBuffers;
    std::shared_ptr<GpuMesh> m_debugBuffers;

protected:
    virtual void UpdateEmitters(const Scene& p_scene) = 0;
};

}  // namespace my
