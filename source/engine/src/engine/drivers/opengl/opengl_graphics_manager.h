#include "core/framework/graphics_manager.h"
#include "rendering/pipeline_state_manager.h"

namespace my {

class OpenGLGraphicsManager : public GraphicsManager {
public:
    OpenGLGraphicsManager() : GraphicsManager("OpenGLGraphicsManager", Backend::OPENGL) {}

    bool initialize() final;
    void finalize() final;

    std::shared_ptr<RenderTarget> create_resource(const RenderTargetDesc& p_desc, const SamplerDesc& p_sampler) final;

    std::shared_ptr<Subpass> create_subpass(const SubpassDesc& p_desc) final;

    std::shared_ptr<Texture> create_texture(const TextureDesc& p_texture_desc, const SamplerDesc& p_sampler_desc) final;

    uint64_t get_final_image() const final;

    // @TODO: refactor this
    void fill_material_constant_buffer(const MaterialComponent* material, MaterialConstantBuffer& cb) final;

protected:
    void on_scene_change(const Scene& p_scene) final;
    void set_pipeline_state_impl(PipelineStateName p_name) final;
    void render() final;

    void createGpuResources();
    void destroyGpuResources();

    std::shared_ptr<PipelineStateManager> m_pipeline_state_manager;
};

}  // namespace my
