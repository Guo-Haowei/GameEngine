#include "core/framework/graphics_manager.h"

namespace my {

class OpenGLGraphicsManager : public GraphicsManager {
public:
    OpenGLGraphicsManager() : GraphicsManager("OpenGLGraphicsManager", Backend::OPENGL) {}

    bool initialize_internal() final;
    void finalize() final;

    void set_render_target(const Subpass* p_subpass, int p_index, int p_mip_level) final;
    void clear(const Subpass* p_subpass, uint32_t p_flags, float* p_clear_color) final;
    void set_viewport(const Viewport& p_vp) final;

    std::shared_ptr<Texture> create_texture(const TextureDesc& p_texture_desc, const SamplerDesc& p_sampler_desc) final;
    std::shared_ptr<Subpass> create_subpass(const SubpassDesc& p_desc) final;

    // @TODO: refactor this
    void fill_material_constant_buffer(const MaterialComponent* material, MaterialConstantBuffer& cb) final;

protected:
    void on_scene_change(const Scene& p_scene) final;
    void set_pipeline_state_impl(PipelineStateName p_name) final;
    void render() final;

    void createGpuResources();
    void destroyGpuResources();
};

}  // namespace my
