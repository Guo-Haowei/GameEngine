#pragma once
#include "core/framework/graphics_manager.h"

namespace my {

class D3d11GraphicsManager : public GraphicsManager {
public:
    D3d11GraphicsManager() : GraphicsManager("D3d11GraphicsManager") {}

    bool initialize() final;
    void finalize() final;
    void render() final;

    // @TODO: filter
    std::shared_ptr<RenderTarget> create_resource(const RenderTargetDesc&, const SamplerDesc&) final { return nullptr; }
    std::shared_ptr<RenderTarget> find_resource(const std::string&) const final { return nullptr; }

    std::shared_ptr<Subpass> create_subpass(const SubpassDesc&) final { return nullptr; }

    void create_texture(ImageHandle*) final {}

    uint64_t get_final_image() const final { return 0; }

    // @TODO: refactor this
    void fill_material_constant_buffer(const MaterialComponent*, MaterialConstantBuffer&) final {}

protected:
    void on_scene_change(const Scene&) final {}
    void on_window_resize(int p_width, int p_height) final;
    void set_pipeline_state_impl(PipelineStateName) final {}
};

}  // namespace my
