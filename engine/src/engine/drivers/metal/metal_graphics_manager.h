#pragma once
#include "engine/drivers/empty/empty_graphics_manager.h"

namespace my {

WARNING_PUSH()
WARNING_DISABLE(4100, "-Wunused-parameter")

class MetalGraphicsManager : public EmptyGraphicsManager {
public:
    MetalGraphicsManager();

    void SetStencilRef(uint32_t p_ref) override {}
    void SetBlendState(const BlendDesc& p_desc, const float* p_factor, uint32_t p_mask) override {}

    void SetRenderTarget(const DrawPass* p_draw_pass, int p_index, int p_mip_level) override {}
    void UnsetRenderTarget() override {}
    void BeginDrawPass(const DrawPass* p_draw_pass) override {}
    void EndDrawPass(const DrawPass* p_draw_pass) override {}

    void Clear(const DrawPass* p_draw_pass, ClearFlags p_flags, const float* p_clear_color, int p_index) override {}
    void SetViewport(const Viewport& p_viewport) override {}

    const MeshBuffers* CreateMesh(const MeshComponent& p_mesh) override { return nullptr; }
    void SetMesh(const MeshBuffers* p_mesh) override {}
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
    auto InitializeInternal() -> Result<void> final;
    void FinalizeImpl() final;

    std::shared_ptr<GpuTexture> CreateTextureImpl(const GpuTextureDesc& p_texture_desc, const SamplerDesc& p_sampler_desc) override { return nullptr; }

    void Render() override {}
    void Present() final;

    void OnSceneChange(const Scene& p_scene) override {}
    void OnWindowResize(int p_width, int p_height) final;
    void SetPipelineStateImpl(PipelineStateName p_name) override {}

private:
    void* m_device;
};

WARNING_POP()

}  // namespace my
