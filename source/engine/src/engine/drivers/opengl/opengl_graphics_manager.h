#include "core/base/rid_owner.h"
#include "core/framework/graphics_manager.h"

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
    void Render() final;

    void SetStencilRef(uint32_t p_ref) final;

    void SetRenderTarget(const DrawPass* p_draw_pass, int p_index, int p_mip_level) final;
    void UnsetRenderTarget() final;

    void Clear(const DrawPass* p_draw_pass, uint32_t p_flags, float* p_clear_color, int p_index) final;
    void SetViewport(const Viewport& p_viewport) final;

    const MeshBuffers* CreateMesh(const MeshComponent& p_mesh) final;
    void SetMesh(const MeshBuffers* p_mesh) final;
    void DrawElements(uint32_t p_count, uint32_t offset) final;
    void DrawElementsInstanced(uint32_t p_instance_count, uint32_t p_count, uint32_t p_offset) final;

    void Dispatch(uint32_t p_num_groups_x, uint32_t p_num_groups_y, uint32_t p_num_groups_z) final;
    void SetUnorderedAccessView(uint32_t p_slot, GpuTexture* p_texture) final;

    std::shared_ptr<GpuStructuredBuffer> CreateStructuredBuffer(const GpuStructuredBufferDesc& p_desc) final;
    void BindStructuredBuffer(int p_slot, const GpuStructuredBuffer* p_buffer) final;
    void UnbindStructuredBuffer(int p_slot) final;
    void BindStructuredBufferSRV(int p_slot, const GpuStructuredBuffer* p_buffer) final;
    void UnbindStructuredBufferSRV(int p_slot) final;

    std::shared_ptr<ConstantBufferBase> CreateConstantBuffer(int p_slot, size_t p_capacity) final;
    void UpdateConstantBuffer(const ConstantBufferBase* p_buffer, const void* p_data, size_t p_size) final;
    void BindConstantBufferRange(const ConstantBufferBase* p_buffer, uint32_t p_size, uint32_t p_offset) final;

    std::shared_ptr<GpuTexture> CreateTexture(const GpuTextureDesc& p_texture_desc, const SamplerDesc& p_sampler_desc) final;
    void BindTexture(Dimension p_dimension, uint64_t p_handle, int p_slot) final;
    void UnbindTexture(Dimension p_dimension, int p_slot) final;

    std::shared_ptr<DrawPass> CreateDrawPass(const DrawPassDesc& p_desc) final;

protected:
    bool InitializeImpl() final;

    void OnSceneChange(const Scene& p_scene) final;
    void OnWindowResize(int, int) final {}
    void SetPipelineStateImpl(PipelineStateName p_name) final;

    void CreateGpuResources();

private:
    // @TODO: rename
    RIDAllocator<OpenGlMeshBuffers> m_meshes;

    struct {
        CullMode cull_mode;
        bool front_counter_clockwise;
        ComparisonFunc depth_func;
        bool enable_depth_test;
        bool enable_stencil_test;
        uint32_t stencil_func;
    } m_state_cache;
};

}  // namespace my
