#include "engine/core/base/rid_owner.h"
#include "engine/core/framework/graphics_manager.h"
#include "opengl_helpers_forward.h"

struct GLFWwindow;

namespace my {

// @TODO: fix
struct OpenGlMeshBuffers : GpuMesh {
    using GpuMesh::GpuMesh;

    uint32_t vao{ 0 };
};

class OpenGlGraphicsManager : public GraphicsManager {
public:
    OpenGlGraphicsManager();

    void FinalizeImpl() final;

    void SetStencilRef(uint32_t p_ref) final;
    void SetBlendState(const BlendDesc& p_desc, const float* p_factor, uint32_t p_mask) final;

    void SetRenderTarget(const Framebuffer* p_framebuffer, int p_index, int p_mip_level) final;
    void UnsetRenderTarget() final;

    void Clear(const Framebuffer* p_framebuffer, ClearFlags p_flags, const float* p_clear_color, int p_index) final;
    void SetViewport(const Viewport& p_viewport) final;

    auto CreateBuffer(const GpuBufferDesc& p_desc) -> Result<std::shared_ptr<GpuBuffer>> final;
    void UpdateBuffer(const GpuBufferDesc& p_desc, GpuBuffer* p_buffer) final;

    auto CreateMeshImpl(const GpuMeshDesc& p_desc,
                        uint32_t p_count,
                        const GpuBufferDesc* p_vb_descs,
                        const GpuBufferDesc* p_ib_desc) -> Result<std::shared_ptr<GpuMesh>> final;

    void SetMesh(const GpuMesh* p_mesh) final;

    void DrawElements(uint32_t p_count, uint32_t p_offset) final;
    void DrawElementsInstanced(uint32_t p_instance_count, uint32_t p_count, uint32_t p_offset) final;
    void DrawArrays(uint32_t p_count, uint32_t p_offset) final;
    void DrawArraysInstanced(uint32_t p_instance_count, uint32_t p_count, uint32_t p_offset) final;

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

    std::shared_ptr<Framebuffer> CreateDrawPass(const FramebufferDesc& p_desc) final;

protected:
    auto InitializeInternal() -> Result<void> final;
    std::shared_ptr<GpuTexture> CreateTextureImpl(const GpuTextureDesc& p_texture_desc, const SamplerDesc& p_sampler_desc) final;

    void Render() final;
    void Present() final;

    void OnWindowResize(int, int) final {}
    void SetPipelineStateImpl(PipelineStateName p_name) final;

    void CreateGpuResources();

private:
    // @TODO: rename
    RIDAllocator<OpenGlMeshBuffers> m_meshes;

    GLFWwindow* m_window;

    struct {
        CullMode cullMode;
        bool frontCounterClockwise;
        ComparisonFunc depthFunc;
        bool enableDepthTest;
        bool enableStencilTest;
        ComparisonFunc stencilFunc;
        gl::TOPOLOGY topology;
    } m_stateCache;
};

}  // namespace my
