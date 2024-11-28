#include "engine/core/base/rid_owner.h"
#include "engine/core/framework/graphics_manager.h"

// @TODO: fix
struct OpenGlMeshBuffers : public my::MeshBuffers {
    uint32_t vao = 0;
    uint32_t ebo = 0;
    uint32_t vbos[6] = { 0 };
};

namespace my {

class OpenGlGraphicsManager : public GraphicsManager {
public:
    OpenGlGraphicsManager();

    void Finalize() final;

    void SetStencilRef(uint32_t p_ref) final;
    void SetBlendState(const BlendDesc& p_desc, const float* p_factor, uint32_t p_mask) final;

    void SetRenderTarget(const DrawPass* p_draw_pass, int p_index, int p_mip_level) final;
    void UnsetRenderTarget() final;

    void Clear(const DrawPass* p_draw_pass, ClearFlags p_flags, const float* p_clear_color, int p_index) final;
    void SetViewport(const Viewport& p_viewport) final;

    const MeshBuffers* CreateMesh(const MeshComponent& p_mesh) final;
    void SetMesh(const MeshBuffers* p_mesh) final;
    void DrawElements(uint32_t p_count, uint32_t offset) final;
    void DrawElementsInstanced(uint32_t p_instance_count, uint32_t p_count, uint32_t p_offset) final;

    void Dispatch(uint32_t p_num_groups_x, uint32_t p_num_groups_y, uint32_t p_num_groups_z) final;
    void BindUnorderedAccessView(uint32_t p_slot, GpuTexture* p_texture) final;
    void UnbindUnorderedAccessView(uint32_t p_slot) final;

    auto CreateConstantBuffer(const GpuBufferDesc& p_desc) -> Result<std::shared_ptr<GpuConstantBuffer>> final;
    auto CreateStructuredBuffer(const GpuBufferDesc& p_desc) -> Result<std::shared_ptr<GpuStructuredBuffer>> final;
    void UpdateBufferData(const GpuBufferDesc& p_desc, const GpuStructuredBuffer* p_buffer) final;

    void BindStructuredBuffer(int p_slot, const GpuStructuredBuffer* p_buffer) final;
    void UnbindStructuredBuffer(int p_slot) final;
    void BindStructuredBufferSRV(int p_slot, const GpuStructuredBuffer* p_buffer) final;
    void UnbindStructuredBufferSRV(int p_slot) final;

    void UpdateConstantBuffer(const GpuConstantBuffer* p_buffer, const void* p_data, size_t p_size) final;
    void BindConstantBufferRange(const GpuConstantBuffer* p_buffer, uint32_t p_size, uint32_t p_offset) final;

    void BindTexture(Dimension p_dimension, uint64_t p_handle, int p_slot) final;
    void UnbindTexture(Dimension p_dimension, int p_slot) final;

    void GenerateMipmap(const GpuTexture* p_texture) final;

    std::shared_ptr<DrawPass> CreateDrawPass(const DrawPassDesc& p_desc) final;

protected:
    auto InitializeImpl() -> Result<void> final;
    std::shared_ptr<GpuTexture> CreateTextureImpl(const GpuTextureDesc& p_texture_desc, const SamplerDesc& p_sampler_desc) final;

    void Render() final;
    void Present() final;

    void OnSceneChange(const Scene& p_scene) final;
    void OnWindowResize(int, int) final {}
    void SetPipelineStateImpl(PipelineStateName p_name) final;

    void CreateGpuResources();

private:
    // @TODO: rename
    RIDAllocator<OpenGlMeshBuffers> m_meshes;

    struct {
        CullMode cullMode;
        bool frontCounterClockwise;
        ComparisonFunc depthFunc;
        bool enableDepthTest;
        bool enableStencilTest;
        ComparisonFunc stencilFunc;
    } m_stateCache;
};

}  // namespace my
