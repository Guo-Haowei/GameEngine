#include "core/framework/graphics_manager.h"
#include "rendering/pipeline_state_manager.h"

namespace my {

class GLGraphicsManager : public GraphicsManager {
public:
    GLGraphicsManager() : GraphicsManager("OpenGLGraphicsManager") {}

    bool initialize() final;
    void finalize() final;
    void render() final;

    std::shared_ptr<RenderTarget> create_resource(const RenderTargetDesc& desc) final;
    std::shared_ptr<RenderTarget> find_resource(const std::string& name) const final;

    std::shared_ptr<Subpass> create_subpass(const SubpassDesc& p_desc) override;

    // @TODO: filter
    void create_texture(ImageHandle* handle) final;

    uint64_t get_final_image() const final;

    // @TODO: refactor this
    void fill_material_constant_buffer(const MaterialComponent* material, MaterialConstantBuffer& cb) final;

protected:
    void on_scene_change(const Scene& p_scene) final;
    void set_pipeline_state_impl(PipelineStateName p_name) final;

    void createGpuResources();
    void destroyGpuResources();

    std::shared_ptr<PipelineStateManager> m_pipeline_state_manager;
};

}  // namespace my
