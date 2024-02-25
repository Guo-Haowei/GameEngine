#pragma once
#include "core/framework/graphics_manager.h"

namespace my {

class EmptyGraphicsManager : public GraphicsManager {
public:
    EmptyGraphicsManager() : GraphicsManager("EmptyGraphicsManager") {}

    bool initialize() override { return true; }
    void finalize() override {}
    void render() override {}

    // @TODO: filter
    std::shared_ptr<RenderTarget> create_resource(const RenderTargetDesc&, const SamplerDesc&) override { return nullptr; }
    std::shared_ptr<RenderTarget> find_resource(const std::string&) const override { return nullptr; }

    std::shared_ptr<Subpass> create_subpass(const SubpassDesc&) override { return nullptr; }

    void create_texture(ImageHandle*) override {}

    uint64_t get_final_image() const override { return 0; }

    // @TODO: refactor this
    void fill_material_constant_buffer(const MaterialComponent*, MaterialConstantBuffer&) override {}

protected:
    void on_scene_change(const Scene&) override {}
    void set_pipeline_state_impl(PipelineStateName) override {}
};

}  // namespace my
