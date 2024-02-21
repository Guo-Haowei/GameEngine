#include "core/framework/graphics_manager.h"

namespace my {

class GLGraphicsManager : public GraphicsManager {
public:
    GLGraphicsManager() : GraphicsManager("OpenGLGraphicsManager") {}

    bool initialize() final;
    void finalize() final;
    void render() final;

    // @TODO: filter
    void create_texture(ImageHandle* handle) final;

    uint32_t get_final_image() const final;

    std::shared_ptr<RenderTarget> create_resource(const RenderTargetDesc& desc) final;
    std::shared_ptr<RenderTarget> find_resource(const std::string& name) const final;

    // @TODO: refactor this
    void fill_material_constant_buffer(const MaterialComponent* material, MaterialConstantBuffer& cb) final;

protected:
    void on_scene_change(const Scene& p_scene) final;

    void createGpuResources();
    void destroyGpuResources();
};

}  // namespace my
