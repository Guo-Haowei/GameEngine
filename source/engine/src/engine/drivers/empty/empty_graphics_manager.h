#pragma once
#include "core/framework/graphics_manager.h"

namespace my {

#pragma warning(push)
#pragma warning(disable : 4100)

class EmptyGraphicsManager : public GraphicsManager {
public:
    EmptyGraphicsManager(Backend p_backend) : GraphicsManager("EmptyGraphicsManager", p_backend) {}

    bool initialize_internal() override { return true; }
    void finalize() override {}
    void render() override {}

    void set_render_target(const Subpass* p_subpass, int p_index, int p_mip_level) override {}
    void clear(const Subpass* p_subpass, uint32_t p_flags, float* p_clear_color) override {}
    void set_viewport(const Viewport& p_viewport) override {}

    void set_mesh(const MeshBuffers* p_mesh) override{};
    void draw_elements(uint32_t p_count, uint32_t p_offset) override{};

    std::shared_ptr<Texture> create_texture(const TextureDesc&, const SamplerDesc&) { return nullptr; }
    std::shared_ptr<Subpass> create_subpass(const SubpassDesc&) override { return nullptr; }

    // @TODO: refactor this
    void fill_material_constant_buffer(const MaterialComponent*, MaterialConstantBuffer&) override {}

protected:
    void on_scene_change(const Scene&) override {}
    void set_pipeline_state_impl(PipelineStateName) override {}
};

#pragma warning(pop)

}  // namespace my
