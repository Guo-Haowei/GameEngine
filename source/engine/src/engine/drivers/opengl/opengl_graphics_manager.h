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

    bool InitializeImpl() final;
    void Finalize() final;

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
    void SetUnorderedAccessView(uint32_t p_slot, Texture* p_texture) final;

    std::shared_ptr<UniformBufferBase> CreateUniform(int p_slot, size_t p_capacity) final;
    void UpdateUniform(const UniformBufferBase* p_buffer, const void* p_data, size_t p_size) final;
    void BindUniformRange(const UniformBufferBase* p_buffer, uint32_t p_size, uint32_t p_offset) final;

    void BindTexture(Dimension p_dimension, uint64_t p_handle, int p_slot) final;
    void UnbindTexture(Dimension p_dimension, int p_slot) final;

    std::shared_ptr<Texture> CreateTexture(const TextureDesc& p_texture_desc, const SamplerDesc& p_sampler_desc) final;
    std::shared_ptr<DrawPass> CreateDrawPass(const DrawPassDesc& p_desc) final;

protected:
    void OnSceneChange(const Scene& p_scene) final;
    void SetPipelineStateImpl(PipelineStateName p_name) final;
    void Render() final;

    void CreateGpuResources();

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
