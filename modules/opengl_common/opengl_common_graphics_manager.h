#pragma once

#include "engine/core/base/rid_owner.h"
#include "engine/renderer/base_graphics_manager.h"
#include "opengl_helpers_forward.h"

struct GLFWwindow;

namespace my {

// @TODO: fix
struct OpenGlMeshBuffers : GpuMesh {
    using GpuMesh::GpuMesh;

    uint32_t vao{ 0 };
};

class CommonOpenGLGraphicsManager : public BaseGraphicsManager {
public:
    CommonOpenGLGraphicsManager();

    void FinalizeImpl() override;

    void SetStencilRef(uint32_t p_ref) override;
    void SetBlendState(const BlendDesc& p_desc, const float* p_factor, uint32_t p_mask) override;

    void SetRenderTarget(const Framebuffer* p_framebuffer, int p_index, int p_mip_level) override;
    void UnsetRenderTarget() override;

    void Clear(const Framebuffer* p_framebuffer,
               ClearFlags p_flags,
               const float* p_clear_color,
               float p_clear_depth,
               uint8_t p_clear_stencil,
               int p_index) override;
    void SetViewport(const Viewport& p_viewport) override;

    auto CreateBuffer(const GpuBufferDesc& p_desc) -> Result<std::shared_ptr<GpuBuffer>> override;
    void UpdateBuffer(const GpuBufferDesc& p_desc, GpuBuffer* p_buffer) override;

    auto CreateMeshImpl(const GpuMeshDesc& p_desc,
                        uint32_t p_count,
                        const GpuBufferDesc* p_vb_descs,
                        const GpuBufferDesc* p_ib_desc) -> Result<std::shared_ptr<GpuMesh>> override;

    void SetMesh(const GpuMesh* p_mesh) override;

    void DrawElements(uint32_t p_count, uint32_t p_offset) override;
    void DrawElementsInstanced(uint32_t p_instance_count, uint32_t p_count, uint32_t p_offset) override;
    void DrawArrays(uint32_t p_count, uint32_t p_offset) override;
    void DrawArraysInstanced(uint32_t p_instance_count, uint32_t p_count, uint32_t p_offset) override;

    void Dispatch(uint32_t p_num_groups_x, uint32_t p_num_groups_y, uint32_t p_num_groups_z) override;
    void BindUnorderedAccessView(uint32_t p_slot, GpuTexture* p_texture) override;
    void UnbindUnorderedAccessView(uint32_t p_slot) override;

    void BindStructuredBuffer(int p_slot, const GpuStructuredBuffer* p_buffer) override;
    void UnbindStructuredBuffer(int p_slot) override;
    void BindStructuredBufferSRV(int p_slot, const GpuStructuredBuffer* p_buffer) override;
    void UnbindStructuredBufferSRV(int p_slot) override;

    auto CreateConstantBuffer(const GpuBufferDesc& p_desc) -> Result<std::shared_ptr<GpuConstantBuffer>> override;
    auto CreateStructuredBuffer(const GpuBufferDesc& p_desc) -> Result<std::shared_ptr<GpuStructuredBuffer>> override;
    void UpdateBufferData(const GpuBufferDesc& p_desc, const GpuStructuredBuffer* p_buffer) override;

    void UpdateConstantBuffer(const GpuConstantBuffer* p_buffer, const void* p_data, size_t p_size) override;
    void BindConstantBufferRange(const GpuConstantBuffer* p_buffer, uint32_t p_size, uint32_t p_offset) override;

    void BindTexture(Dimension p_dimension, uint64_t p_handle, int p_slot) override;
    void UnbindTexture(Dimension p_dimension, int p_slot) override;

    void GenerateMipmap(const GpuTexture* p_texture) override;

    std::shared_ptr<Framebuffer> CreateFramebuffer(const FramebufferDesc& p_desc) override;

protected:
    std::shared_ptr<GpuTexture> CreateTextureImpl(const GpuTextureDesc& p_texture_desc, const SamplerDesc& p_sampler_desc) override;

    void Render() override;
    void Present() override;

    void OnWindowResize(int, int) override {}
    void SetPipelineStateImpl(PipelineStateName p_name) override;

    void CreateGpuResources();

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
