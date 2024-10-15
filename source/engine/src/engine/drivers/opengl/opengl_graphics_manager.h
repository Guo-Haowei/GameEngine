#include "core/base/rid_owner.h"
#include "core/framework/graphics_manager.h"

// @TODO: fix
struct OpenGLMeshBuffers : public my::MeshBuffers {
    uint32_t vao = 0;
    uint32_t ebo = 0;
    uint32_t vbos[6] = { 0 };
};

namespace my {

class OpenGLGraphicsManager : public GraphicsManager {
public:
    OpenGLGraphicsManager();

    bool initializeImpl() final;
    void finalize() final;

    void setStencilRef(uint32_t p_ref) final;

    void setRenderTarget(const Subpass* p_subpass, int p_index, int p_mip_level) final;
    void clear(const Subpass* p_subpass, uint32_t p_flags, float* p_clear_color) final;
    void setViewport(const Viewport& p_viewport) final;

    const MeshBuffers* createMesh(const MeshComponent& p_mesh) final;
    void setMesh(const MeshBuffers* p_mesh) final;
    void drawElements(uint32_t p_count, uint32_t offset) final;

    std::shared_ptr<UniformBufferBase> createUniform(int p_slot, size_t p_capacity) final;
    void updateUniform(const UniformBufferBase* p_buffer, const void* p_data, size_t p_size) final;
    void bindUniformRange(const UniformBufferBase* p_buffer, uint32_t p_size, uint32_t p_offset) final;

    void bindTexture(Dimension p_dimension, uint64_t p_handle, int p_slot) final;

    std::shared_ptr<Texture> createTexture(const TextureDesc& p_texture_desc, const SamplerDesc& p_sampler_desc) final;
    std::shared_ptr<Subpass> createSubpass(const SubpassDesc& p_desc) final;

protected:
    void onSceneChange(const Scene& p_scene) final;
    void setPipelineStateImpl(PipelineStateName p_name) final;
    void render() final;

    void createGpuResources();

private:
    // @TODO: rename
    RIDAllocator<OpenGLMeshBuffers> m_meshes;

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
