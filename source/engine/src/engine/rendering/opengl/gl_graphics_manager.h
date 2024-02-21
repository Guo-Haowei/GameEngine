#include "core/framework/graphics_manager.h"

namespace my {

class OpenGLGraphicsManager : public GraphicsManager {
public:
    OpenGLGraphicsManager() : GraphicsManager("OpenGLGraphicsManager") {}

    bool initialize() override;
    void finalize() override;
    void render();

    // @TODO: filter
    void create_texture(ImageHandle* handle);

    void event_received(std::shared_ptr<Event> event) override;

    uint32_t get_final_image() const;

    std::shared_ptr<RenderData> get_render_data() { return m_render_data; }

    const rg::RenderGraph& get_active_render_graph() { return m_render_graph; }

    std::shared_ptr<RenderTarget> create_resource(const RenderTargetDesc& desc);
    std::shared_ptr<RenderTarget> find_resource(const std::string& name) const;

    // @TODO: refactor this
    void fill_material_constant_buffer(const MaterialComponent* material, MaterialConstantBuffer& cb);

protected:
    void createGpuResources();
    void destroyGpuResources();
};

}  // namespace my
