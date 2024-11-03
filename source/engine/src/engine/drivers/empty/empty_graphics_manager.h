#pragma once
#include "core/framework/graphics_manager.h"

namespace my {

#pragma warning(push)
#pragma warning(disable : 4100)

class EmptyGraphicsManager : public GraphicsManager {
public:
    EmptyGraphicsManager(Backend p_backend) : GraphicsManager("EmptyGraphicsManager", p_backend) {}

    bool InitializeImpl() override { return true; }
    void Finalize() override {}
    void Render() override {}

    void SetStencilRef(uint32_t p_ref) override {}

    void SetRenderTarget(const DrawPass* p_draw_pass, int p_index, int p_mip_level) override {}
    void UnsetRenderTarget() override {}

    void Clear(const DrawPass* p_draw_pass, uint32_t p_flags, float* p_clear_color, int p_index) override {}
    void SetViewport(const Viewport& p_viewport) override {}

    const MeshBuffers* CreateMesh(const MeshComponent& p_mesh) override { return nullptr; }
    void SetMesh(const MeshBuffers* p_mesh) override {}
    void DrawElements(uint32_t p_count, uint32_t p_offset) override {}
    void DrawElementsInstanced(uint32_t p_instance_count, uint32_t p_count, uint32_t p_offset) override {}

    void Dispatch(uint32_t p_num_groups_x, uint32_t p_num_groups_y, uint32_t p_num_groups_z) override {}
    void SetUnorderedAccessView(uint32_t p_slot, GpuTexture* p_texture) override {}

    std::shared_ptr<GpuStructuredBuffer> CreateStructuredBuffer(const GpuStructuredBufferDesc& p_desc) { return nullptr; }
    void BindStructuredBuffer(const GpuStructuredBuffer* p_buffer, int p_slot) override {}

    std::shared_ptr<ConstantBufferBase> CreateConstantBuffer(int p_slot, size_t p_capacity) override { return nullptr; }
    void UpdateConstantBuffer(const ConstantBufferBase* p_buffer, const void* p_data, size_t p_size) override {}
    void BindConstantBufferRange(const ConstantBufferBase* p_buffer, uint32_t p_size, uint32_t p_offset) override {}

    std::shared_ptr<GpuTexture> CreateTexture(const GpuTextureDesc&, const SamplerDesc&) { return nullptr; }
    void BindTexture(Dimension p_dimension, uint64_t p_handle, int p_slot) override {}
    void UnbindTexture(Dimension p_dimension, int p_slot) override {}

    std::shared_ptr<DrawPass> CreateDrawPass(const DrawPassDesc&) override { return nullptr; }

protected:
    void OnSceneChange(const Scene&) override {}
    void SetPipelineStateImpl(PipelineStateName) override {}
};

#pragma warning(pop)

}  // namespace my
