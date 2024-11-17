#pragma once
#include "core/framework/graphics_manager.h"

namespace my {

WARNING_PUSH()
WARNING_DISABLE(4100, "-Wunused-parameter")

class EmptyGraphicsManager : public GraphicsManager {
public:
    EmptyGraphicsManager(std::string_view p_name, Backend p_backend, int p_frame_count) : GraphicsManager(p_name, p_backend, p_frame_count) {}

    void Finalize() override {}

    void SetStencilRef(uint32_t p_ref) override {}

    void SetRenderTarget(const DrawPass* p_draw_pass, int p_index, int p_mip_level) override {}
    void UnsetRenderTarget() override {}
    void BeginPass(const RenderPass* p_render_pass) override {}
    void EndPass(const RenderPass* p_render_pass) override {}

    void Clear(const DrawPass* p_draw_pass, ClearFlags p_flags, const float* p_clear_color, int p_index) override {}
    void SetViewport(const Viewport& p_viewport) override {}

    const MeshBuffers* CreateMesh(const MeshComponent& p_mesh) override { return nullptr; }
    void SetMesh(const MeshBuffers* p_mesh) override {}
    void DrawElements(uint32_t p_count, uint32_t p_offset) override {}
    void DrawElementsInstanced(uint32_t p_instance_count, uint32_t p_count, uint32_t p_offset) override {}

    void Dispatch(uint32_t p_num_groups_x, uint32_t p_num_groups_y, uint32_t p_num_groups_z) override {}
    void SetUnorderedAccessView(uint32_t p_slot, GpuTexture* p_texture) override {}

    std::shared_ptr<GpuConstantBuffer> CreateConstantBuffer(const GpuBufferDesc& p_desc) override { return nullptr; }
    std::shared_ptr<GpuStructuredBuffer> CreateStructuredBuffer(const GpuBufferDesc& p_desc) override { return nullptr; }

    void BindStructuredBuffer(int p_slot, const GpuStructuredBuffer* p_buffer) override {}
    void UnbindStructuredBuffer(int p_slot) override {}
    void BindStructuredBufferSRV(int p_slot, const GpuStructuredBuffer* p_buffer) override {}
    void UnbindStructuredBufferSRV(int p_slot) override {}

    void UpdateConstantBuffer(const GpuConstantBuffer* p_buffer, const void* p_data, size_t p_size) override {}
    void BindConstantBufferRange(const GpuConstantBuffer* p_buffer, uint32_t p_size, uint32_t p_offset) override {}

    void BindTexture(Dimension p_dimension, uint64_t p_handle, int p_slot) override {}
    void UnbindTexture(Dimension p_dimension, int p_slot) override {}

    std::shared_ptr<DrawPass> CreateDrawPass(const DrawPassDesc& p_subpass_desc) override { return nullptr; }

protected:
    bool InitializeImpl() override { return true; }
    std::shared_ptr<GpuTexture> CreateGpuTextureImpl(const GpuTextureDesc& p_texture_desc, const SamplerDesc& p_sampler_desc) override { return nullptr; }

    void Render() override {}
    void Present() override {}

    void OnSceneChange(const Scene& p_scene) override {}
    void OnWindowResize(int p_width, int p_height) override {}
    void SetPipelineStateImpl(PipelineStateName p_name) override {}
};

WARNING_POP()

}  // namespace my
