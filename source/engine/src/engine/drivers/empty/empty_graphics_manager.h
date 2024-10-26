#pragma once
#include "core/framework/graphics_manager.h"

namespace my {

#pragma warning(push)
#pragma warning(disable : 4100)

class EmptyGraphicsManager : public GraphicsManager {
public:
    EmptyGraphicsManager(Backend p_backend) : GraphicsManager("EmptyGraphicsManager", p_backend) {}

    bool initializeImpl() override { return true; }
    void finalize() override {}
    void render() override {}

    void setStencilRef(uint32_t p_ref) override {}

    void setRenderTarget(const DrawPass* p_draw_pass, int p_index, int p_mip_level) override {}
    void unsetRenderTarget() override {}

    void clear(const DrawPass* p_draw_pass, uint32_t p_flags, float* p_clear_color, int p_index) override {}
    void setViewport(const Viewport& p_viewport) override {}

    const MeshBuffers* createMesh(const MeshComponent& p_mesh) override { return nullptr; }
    void setMesh(const MeshBuffers* p_mesh) override {}
    void drawElements(uint32_t p_count, uint32_t p_offset) override {}

    void dispatch(uint32_t p_num_groups_x, uint32_t p_num_groups_y, uint32_t p_num_groups_z) override {}
    void setUnorderedAccessView(uint32_t p_slot, Texture* p_texture) override {}

    std::shared_ptr<UniformBufferBase> createUniform(int p_slot, size_t p_capacity) override { return nullptr; }
    void updateUniform(const UniformBufferBase* p_buffer, const void* p_data, size_t p_size) override {}
    void bindUniformRange(const UniformBufferBase* p_buffer, uint32_t p_size, uint32_t p_offset) override {}

    void bindTexture(Dimension p_dimension, uint64_t p_handle, int p_slot) override {}
    void unbindTexture(Dimension p_dimension, int p_slot) override {}

    std::shared_ptr<Texture> createTexture(const TextureDesc&, const SamplerDesc&) { return nullptr; }
    std::shared_ptr<DrawPass> createDrawPass(const DrawPassDesc&) override { return nullptr; }

protected:
    void onSceneChange(const Scene&) override {}
    void setPipelineStateImpl(PipelineStateName) override {}
};

#pragma warning(pop)

}  // namespace my
