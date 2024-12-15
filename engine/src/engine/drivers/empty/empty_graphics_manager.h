#pragma once
#include "empty_pipeline_state_manager.h"
#include "engine/core/framework/graphics_manager.h"

namespace my {

WARNING_PUSH()
WARNING_DISABLE(4100, "-Wunused-parameter")

class EmptyGraphicsManager : public GraphicsManager {
public:
    EmptyGraphicsManager(std::string_view p_name,
                         Backend p_backend,
                         int p_frame_count) : GraphicsManager(p_name, p_backend, p_frame_count) {
        m_pipelineStateManager = std::make_shared<EmptyPipelineStateManager>();
    }

    void FinalizeImpl() override {}

    void SetStencilRef(uint32_t p_ref) override {}
    void SetBlendState(const BlendDesc& p_desc, const float* p_factor, uint32_t p_mask) override {}

    void SetRenderTarget(const DrawPass* p_draw_pass, int p_index, int p_mip_level) override {}
    void UnsetRenderTarget() override {}
    void BeginDrawPass(const DrawPass* p_draw_pass) override {}
    void EndDrawPass(const DrawPass* p_draw_pass) override {}

    void Clear(const DrawPass* p_draw_pass, ClearFlags p_flags, const float* p_clear_color, int p_index) override {}
    void SetViewport(const Viewport& p_viewport) override {}

    const GpuMesh* CreateMesh(const MeshComponent& p_mesh) override { return nullptr; }
    void SetMesh(const GpuMesh* p_mesh) override {}
    void DrawElements(uint32_t p_count, uint32_t p_offset) override {}
    void DrawElementsInstanced(uint32_t p_instance_count, uint32_t p_count, uint32_t p_offset) override {}

    void Dispatch(uint32_t p_num_groups_x, uint32_t p_num_groups_y, uint32_t p_num_groups_z) override {}
    void BindUnorderedAccessView(uint32_t p_slot, GpuTexture* p_texture) override {}
    void UnbindUnorderedAccessView(uint32_t p_slot) override {}

    auto CreateConstantBuffer(const GpuBufferDesc& p_desc) -> Result<std::shared_ptr<GpuConstantBuffer>> override { return nullptr; }
    auto CreateStructuredBuffer(const GpuBufferDesc& p_desc) -> Result<std::shared_ptr<GpuStructuredBuffer>> override { return nullptr; }

    void BindStructuredBuffer(int p_slot, const GpuStructuredBuffer* p_buffer) override {}
    void UnbindStructuredBuffer(int p_slot) override {}
    void BindStructuredBufferSRV(int p_slot, const GpuStructuredBuffer* p_buffer) override {}
    void UnbindStructuredBufferSRV(int p_slot) override {}

    void UpdateConstantBuffer(const GpuConstantBuffer* p_buffer, const void* p_data, size_t p_size) override {}
    void BindConstantBufferRange(const GpuConstantBuffer* p_buffer, uint32_t p_size, uint32_t p_offset) override {}

    void BindTexture(Dimension p_dimension, uint64_t p_handle, int p_slot) override {}
    void UnbindTexture(Dimension p_dimension, int p_slot) override {}

    void GenerateMipmap(const GpuTexture* p_texture) override {}

    std::shared_ptr<DrawPass> CreateDrawPass(const DrawPassDesc& p_subpass_desc) override { return nullptr; }

protected:
    auto InitializeInternal() -> Result<void> override { return Result<void>(); }
    std::shared_ptr<GpuTexture> CreateTextureImpl(const GpuTextureDesc& p_texture_desc, const SamplerDesc& p_sampler_desc) override { return nullptr; }

    void Render() override {}
    void Present() override {}

    void OnWindowResize(int p_width, int p_height) override {}
    void SetPipelineStateImpl(PipelineStateName p_name) override {}
};

WARNING_POP()

}  // namespace my
